// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/MKHGameMode.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/MKHGameplayTags.h"
#include "GameMode/MKHGameState.h"
#include "MKHLogChannels.h"
#include "Player/MKHPlayerCharacter.h"

// ============================================================
// Lifecycle
// ============================================================

AMKHGameMode::AMKHGameMode()
{
	// Two fixed slots for the 1v1 participants.
	Players.SetNum(2);

	// Drive the replicated duel state / UI events through our custom game state.
	GameStateClass = AMKHGameState::StaticClass();
}

void AMKHGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	// After Super the pawn is spawned and possessed, so the controller's pawn is guaranteed valid.
	RegisterParticipant(NewPlayer);
	TryStartMatch();
}

void AMKHGameMode::Logout(AController* Exiting)
{
	// Without this the ballot would keep counting a controller that is on its way out, and the
	// remaining player could never reach the (now lower) vote requirement.
	if (APlayerController* ExitingPlayer = Cast<APlayerController>(Exiting))
	{
		if (RematchVoters.Remove(ExitingPlayer) > 0)
		{
			PublishRematchVotes();
		}
	}

	Super::Logout(Exiting);
}

// ============================================================
// Public Interface (Class default data accessors)
// ============================================================

UCharacterClassInfo* AMKHGameMode::GetCharacterClassDefaultInfo() const
{
	return ClassDefaults;
}

UProjectileInfo* AMKHGameMode::GetProjectileInfo() const
{
	return ProjectileInfo;
}

// ============================================================
// Public Interface (Rematch ballot)
// ============================================================

void AMKHGameMode::RequestRematch(APlayerController* Voter)
{
	if (!IsValid(Voter))
	{
		return;
	}

	// The ballot only exists between the end of the match and the restart.
	if (MatchPhase != EMKHMatchPhase::MatchOver)
	{
		UE_LOG(LogMKHAbility, Warning, TEXT("MKHGameMode: %s asked for a rematch outside the MatchOver phase, ignoring."), *GetNameSafe(Voter));
		return;
	}

	bool bAlreadyVoted = false;
	RematchVoters.Add(Voter, &bAlreadyVoted);
	if (bAlreadyVoted)
	{
		return;
	}

	const int32 VotesCast = RematchVoters.Num();
	const int32 VotesNeeded = CountRematchVotersNeeded();

	UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: %s voted for a rematch (%d/%d)."), *GetNameSafe(Voter), VotesCast, VotesNeeded);

	PublishRematchVotes();

	if (VotesCast < VotesNeeded)
	{
		return;
	}

	UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: every participant voted for a rematch, restarting the match."));
	RestartMatchInPlace();
}

// ============================================================
// Protected / Internal Logic (Match flow)
// ============================================================

void AMKHGameMode::RegisterParticipant(AController* NewPlayer)
{
	AMKHPlayerCharacter* Character = NewPlayer ? Cast<AMKHPlayerCharacter>(NewPlayer->GetPawn()) : nullptr;
	if (!IsValid(Character))
	{
		return;
	}

	// Ignore characters already registered (OnPostLogin + BeginPlay can both see the host).
	if (FindParticipantIndex(Character) != INDEX_NONE)
	{
		return;
	}

	// Find the first free slot; a 1v1 match only accepts two participants.
	const int32 FreeIndex = Players.IndexOfByPredicate([](const FMKHPlayerData& Data) { return !Data.IsValidPlayer(); });
	if (FreeIndex == INDEX_NONE)
	{
		UE_LOG(LogMKHAbility, Warning, TEXT("MKHGameMode: a third participant tried to join a 1v1 match, ignoring %s."), *GetNameSafe(Character));
		return;
	}

	Players[FreeIndex].PlayerCharacter = Character;
	Players[FreeIndex].Score = 0;
	Players[FreeIndex].SpawnTransform = Character->GetActorTransform();

	if (AMKHGameState* GS = GetDuelGameState())
	{
		GS->RegisterParticipant(FreeIndex, Character);
	}

	UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: registered %s in slot %d."), *GetNameSafe(Character), FreeIndex);

	BindDeathEvent(Character);
}

void AMKHGameMode::TryStartMatch()
{
	// Only start once both participants are present and the match has not begun yet.
	if (MatchPhase != EMKHMatchPhase::None)
	{
		return;
	}

	const bool bBothReady = Players[0].IsValidPlayer() && Players[1].IsValidPlayer();
	if (!bBothReady)
	{
		return;
	}

	StartInitialPreparationPhase();
}

void AMKHGameMode::StartInitialPreparationPhase()
{
	SetPhase(EMKHMatchPhase::InitialPreparation, InitialPreparationDuration);

	ClearPhaseTimer();
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AMKHGameMode::StartDuelPhase, InitialPreparationDuration, false);

	UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: initial preparation phase started (%.0fs)."), InitialPreparationDuration);
}

void AMKHGameMode::StartDuelPhase()
{
	SetPhase(EMKHMatchPhase::Duel, DuelDuration);

	ClearPhaseTimer();
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AMKHGameMode::HandleDuelTimeOver, DuelDuration, false);

	UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: duel phase started (max %.0fs)."), DuelDuration);
}

void AMKHGameMode::StartIntermissionPhase()
{
	SetPhase(EMKHMatchPhase::IntermissionPreparation, IntermissionPreparationDuration);

	ClearPhaseTimer();
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AMKHGameMode::StartDuelPhase, IntermissionPreparationDuration, false);

	UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: intermission preparation started (%.0fs)."), IntermissionPreparationDuration);
}

void AMKHGameMode::HandleDuelTimeOver()
{
	// Sudden death has no countdown: the drain effect resolves the duel.
	SetPhase(EMKHMatchPhase::SuddenDeath, 0.f);
	ClearPhaseTimer();

	// Apply the lethal drain to both participants; whoever dies first hands the point to the survivor.
	for (FMKHPlayerData& Data : Players)
	{
		ApplySuddenDeathDrain(Data);
	}

	UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: duel time over, sudden-death drain applied."));
}

void AMKHGameMode::HandlePlayerDeath(const FGameplayEventData* Payload, AMKHPlayerCharacter* DeadCharacter)
{
	// Only deaths during an active duel matter.
	if (MatchPhase != EMKHMatchPhase::Duel && MatchPhase != EMKHMatchPhase::SuddenDeath)
	{
		return;
	}

	const int32 DeadIndex = FindParticipantIndex(DeadCharacter);
	if (DeadIndex == INDEX_NONE || Players[DeadIndex].bDiedThisRound)
	{
		return;
	}

	// Each death awards the opposite slot; on a double KO both events pass through here.
	const int32 ScoringIndex = (DeadIndex == 0) ? 1 : 0;
	FMKHPlayerData& ScoringData = Players[ScoringIndex];
	if (!ScoringData.IsValidPlayer())
	{
		return;
	}

	Players[DeadIndex].bDiedThisRound = true;

	// First death of the round: stop the phase timer and schedule the resolution. The phase stays
	// unchanged so a same-frame second death still counts.
	if (!bRoundResolving)
	{
		bRoundResolving = true;
		ClearPhaseTimer();

		// Deferred by one tick so both drains can run their pending periodic execution in this frame:
		// two players on identical health then die together instead of the first one winning the race.
		GetWorldTimerManager().SetTimerForNextTick(this, &AMKHGameMode::RemoveSuddenDeathDrainFromParticipants);
		GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AMKHGameMode::ResolveRound, DeathResolveDelay, false);
	}

	ScoringData.Score++;

	UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: %s died, point awarded to %s (score %d)."),
		*GetNameSafe(DeadCharacter), *GetNameSafe(ScoringData.PlayerCharacter), ScoringData.Score);

	if (AMKHGameState* GS = GetDuelGameState())
	{
		GS->SetScore(ScoringIndex, ScoringData.Score);
		GS->Multicast_PlayerDied(DeadCharacter, ScoringData.PlayerCharacter, ScoringData.Score);
	}
}

void AMKHGameMode::ResolveRound()
{
	bRoundResolving = false;

	const int32 Score0 = Players[0].Score;
	const int32 Score1 = Players[1].Score;

	// Match over once either player reaches ScoreToWin; equal scores at that point mean a draw.
	if (Score0 >= ScoreToWin || Score1 >= ScoreToWin)
	{
		const bool bIsDraw = (Score0 == Score1);
		AMKHPlayerCharacter* Winner = bIsDraw ? nullptr
			: (Score0 > Score1 ? Players[0].PlayerCharacter : Players[1].PlayerCharacter).Get();

		SetPhase(EMKHMatchPhase::MatchOver, 0.f);
		if (AMKHGameState* GS = GetDuelGameState())
		{
			GS->Multicast_MatchOver(Winner, bIsDraw);
		}
		UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: match over (%d - %d), %s."),
			Score0, Score1, bIsDraw ? TEXT("draw") : *GetNameSafe(Winner));
		return;
	}

	ResetParticipantsForNewRound();
	StartIntermissionPhase();
}

void AMKHGameMode::RestartMatchInPlace()
{
	// Close the ballot first so a vote arriving mid-restart cannot carry over into the new match.
	RematchVoters.Reset();
	PublishRematchVotes();

	// Wipe the authoritative scoreboard and mirror it, otherwise the replicated scores would still
	// show the result of the match that just ended.
	AMKHGameState* GS = GetDuelGameState();
	for (int32 SlotIndex = 0; SlotIndex < Players.Num(); ++SlotIndex)
	{
		Players[SlotIndex].Score = 0;

		if (IsValid(GS))
		{
			GS->SetScore(SlotIndex, 0);
		}
	}

	// Drop whatever the previous match left running before rebuilding the world state.
	bRoundResolving = false;
	ClearPhaseTimer();
	RemoveSuddenDeathDrainFromParticipants();

	// Teleports both participants back to their spawns and forces them out of the Dead state, which
	// is also what hands gameplay input back to a player that lost the last round.
	ResetParticipantsForNewRound();

	UE_LOG(LogMKHAbility, Log, TEXT("MKHGameMode: match restarted in place, scores reset."));

	StartInitialPreparationPhase();
}

// ============================================================
// Protected / Internal Logic (Helpers)
// ============================================================

void AMKHGameMode::SetPhase(EMKHMatchPhase NewPhase, float PhaseDurationSeconds)
{
	MatchPhase = NewPhase;

	if (AMKHGameState* GS = GetDuelGameState())
	{
		GS->SetMatchPhase(NewPhase, PhaseDurationSeconds);
	}
}

AMKHGameState* AMKHGameMode::GetDuelGameState() const
{
	return GetGameState<AMKHGameState>();
}

int32 AMKHGameMode::CountRematchVotersNeeded() const
{
	// Counted from the connected player states, not from the participant pawns: at match over one
	// participant is dead, and whether its pawn is still possessed must not change the vote target.
	const int32 ConnectedPlayers = IsValid(GameState) ? GameState->PlayerArray.Num() : 0;

	// Never zero: a ballot needing no votes would restart the match on the very first press.
	return FMath::Max(ConnectedPlayers, 1);
}

void AMKHGameMode::PublishRematchVotes() const
{
	if (AMKHGameState* GS = GetDuelGameState())
	{
		GS->SetRematchVotes(RematchVoters.Num(), CountRematchVotersNeeded());
	}
}

void AMKHGameMode::BindDeathEvent(AMKHPlayerCharacter* Character)
{
	if (!IsValid(Character))
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character);
	if (!IsValid(ASC))
	{
		UE_LOG(LogMKHAbility, Warning, TEXT("MKHGameMode: cannot bind death event, %s has no ability system component."), *GetNameSafe(Character));
		return;
	}

	// The death event carries the dying actor implicitly; we pass the participant as a payload so
	// the handler resolves the correct slot without re-casting the event instigator.
	ASC->GenericGameplayEventCallbacks.FindOrAdd(MKHGameplayTags::Event::Death)
		.AddUObject(this, &AMKHGameMode::HandlePlayerDeath, Character);
}

void AMKHGameMode::ClearPhaseTimer()
{
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
}

void AMKHGameMode::ResetParticipantsForNewRound()
{
	for (FMKHPlayerData& Data : Players)
	{
		Data.bDiedThisRound = false;

		if (!IsValid(Data.PlayerCharacter))
		{
			continue;
		}

		// Restore the initial spawn transform, top the character back up and revive it to Idle.
		// The reset is multicast because a server-only transform change never restores the rotation
		// on the owning client: movement corrections replicate location and velocity, not orientation.
		// See p.UseLastGoodRotationDuringCorrection in DefaultEngine.ini for the other half of the fix.
		Data.PlayerCharacter->Multicast_ResetToSpawnTransform(Data.SpawnTransform);
		ApplyRoundResetEffect(Data.PlayerCharacter);
		Data.PlayerCharacter->Multicast_TransitionToMovementState(EMovementStateValue::Idle, true);
	}
}

void AMKHGameMode::ApplyRoundResetEffect(AMKHPlayerCharacter* Character) const
{
	if (!RoundResetEffect || !IsValid(Character))
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character);
	if (!IsValid(ASC))
	{
		return;
	}

	const FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(RoundResetEffect, 1.f, Context);
	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void AMKHGameMode::ApplySuddenDeathDrain(FMKHPlayerData& PlayerData) const
{
	if (!SuddenDeathDrainEffect || !IsValid(PlayerData.PlayerCharacter))
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerData.PlayerCharacter);
	if (!IsValid(ASC))
	{
		return;
	}

	const FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(SuddenDeathDrainEffect, 1.f, Context);
	if (Spec.IsValid())
	{
		// Keep the handle so the drain can be surgically removed when the round is decided.
		PlayerData.SuddenDeathDrainHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void AMKHGameMode::RemoveSuddenDeathDrainFromParticipants()
{
	for (FMKHPlayerData& Data : Players)
	{
		if (!Data.SuddenDeathDrainHandle.IsValid())
		{
			continue;
		}

		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.PlayerCharacter);
		if (IsValid(ASC))
		{
			ASC->RemoveActiveGameplayEffect(Data.SuddenDeathDrainHandle);
		}

		Data.SuddenDeathDrainHandle.Invalidate();
	}
}

int32 AMKHGameMode::FindParticipantIndex(const AMKHPlayerCharacter* Character) const
{
	if (!Character)
	{
		return INDEX_NONE;
	}

	return Players.IndexOfByPredicate([Character](const FMKHPlayerData& Data)
	{
		return Data.PlayerCharacter == Character;
	});
}
