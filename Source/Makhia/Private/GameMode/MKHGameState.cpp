// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/MKHGameState.h"

#include "Net/UnrealNetwork.h"
#include "MKHLogChannels.h"
#include "Player/MKHPlayerCharacter.h"

// ============================================================
// Lifecycle
// ============================================================

AMKHGameState::AMKHGameState()
{
	// Two fixed scoreboard slots for the 1v1 participants.
	PlayerDatas.SetNum(2);
	PreviousPlayerDatas = PlayerDatas;
}

void AMKHGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMKHGameState, DuelState);
	DOREPLIFETIME(AMKHGameState, PlayerDatas);
	DOREPLIFETIME(AMKHGameState, RematchVotes);
}

// ============================================================
// Public Interface (Server authority)
// ============================================================

void AMKHGameState::SetMatchPhase(EMKHMatchPhase NewPhase, float PhaseDurationSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	DuelState.Phase = NewPhase;
	DuelState.PhaseDuration = FMath::Max(0.f, PhaseDurationSeconds);
	DuelState.PhaseEndServerTime = (DuelState.PhaseDuration > 0.f)
		? GetServerWorldTimeSeconds() + DuelState.PhaseDuration
		: 0.f;

	// Push the change to clients immediately instead of waiting for the next net cadence.
	ForceNetUpdate();

	// OnRep does not run on the authority, so broadcast the phase events locally here.
	HandlePhaseChanged();
}

void AMKHGameState::RegisterParticipant(int32 SlotIndex, AMKHPlayerCharacter* Character)
{
	if (!HasAuthority() || !PlayerDatas.IsValidIndex(SlotIndex))
	{
		return;
	}

	PlayerDatas[SlotIndex].PlayerCharacter = Character;
	PlayerDatas[SlotIndex].Score = 0;

	ForceNetUpdate();
	OnParticipantRegistered.Broadcast(SlotIndex, Character);
}

void AMKHGameState::BroadcastExistingPlayerDatas()
{
	for (int32 SlotIndex = 0; SlotIndex < PlayerDatas.Num(); ++SlotIndex)
	{
		const FMKHDuelPlayerData& Data = PlayerDatas[SlotIndex];
		if (!IsValid(Data.PlayerCharacter))
		{
			continue;
		}

		OnParticipantRegistered.Broadcast(SlotIndex, Data.PlayerCharacter.Get());
		OnScoreChanged.Broadcast(SlotIndex, Data.Score);
	}
}

void AMKHGameState::BroadcastCurrentPhase()
{
	// Only the generic notification is replayed: the granular per-phase events (OnInitialPreparationStarted,
	// etc.) carry one-shot semantics and must not be re-fired to already-bound listeners.
	OnMatchPhaseChanged.Broadcast(DuelState.Phase);
}

void AMKHGameState::SetScore(int32 SlotIndex, int32 NewScore)
{
	if (!HasAuthority() || !PlayerDatas.IsValidIndex(SlotIndex))
	{
		return;
	}

	PlayerDatas[SlotIndex].Score = NewScore;

	ForceNetUpdate();
	OnScoreChanged.Broadcast(SlotIndex, NewScore);
}

void AMKHGameState::Multicast_PlayerDied_Implementation(AMKHPlayerCharacter* DeadPlayer, AMKHPlayerCharacter* ScoringPlayer, int32 ScoringScore)
{
	OnPlayerDied.Broadcast(DeadPlayer, ScoringPlayer, ScoringScore);
}

void AMKHGameState::Multicast_MatchOver_Implementation(AMKHPlayerCharacter* WinningPlayer, bool bIsDraw)
{
	OnMatchOver.Broadcast(WinningPlayer, bIsDraw);
}

void AMKHGameState::SetRematchVotes(int32 CurrentVotes, int32 RequiredVotes)
{
	if (!HasAuthority())
	{
		return;
	}

	RematchVotes.CurrentVotes = CurrentVotes;
	RematchVotes.RequiredVotes = RequiredVotes;

	// Push the change immediately: the panel is waiting on it to update the button label.
	ForceNetUpdate();

	// OnRep does not run on the authority, so broadcast locally here.
	OnRematchVotesChanged.Broadcast(CurrentVotes, RequiredVotes);
}

// ============================================================
// Public Interface (Read-only queries)
// ============================================================

bool AMKHGameState::IsPhaseTimerActive() const
{
	return DuelState.PhaseEndServerTime > 0.f;
}

float AMKHGameState::GetPhaseTimeRemaining() const
{
	if (!IsPhaseTimerActive())
	{
		return 0.f;
	}

	return FMath::Max(0.f, DuelState.PhaseEndServerTime - GetServerWorldTimeSeconds());
}

float AMKHGameState::GetPhaseElapsedFraction() const
{
	if (!IsPhaseTimerActive() || DuelState.PhaseDuration <= 0.f)
	{
		return 0.f;
	}

	const float Elapsed = DuelState.PhaseDuration - GetPhaseTimeRemaining();
	return FMath::Clamp(Elapsed / DuelState.PhaseDuration, 0.f, 1.f);
}

// ============================================================
// Protected / Internal Logic
// ============================================================

void AMKHGameState::HandlePhaseChanged()
{
	OnMatchPhaseChanged.Broadcast(DuelState.Phase);

	// Fan the generic change out to the granular per-phase events UI can bind to directly.
	switch (DuelState.Phase)
	{
	case EMKHMatchPhase::InitialPreparation:
		OnInitialPreparationStarted.Broadcast();
		break;
	case EMKHMatchPhase::Duel:
		OnDuelStarted.Broadcast();
		break;
	case EMKHMatchPhase::SuddenDeath:
		OnDuelTimeOver.Broadcast();
		break;
	case EMKHMatchPhase::IntermissionPreparation:
		OnIntermissionPreparationStarted.Broadcast();
		break;
	default:
		break;
	}
}

void AMKHGameState::OnRep_DuelState()
{
	HandlePhaseChanged();
}

void AMKHGameState::OnRep_PlayerDatas()
{
	// A single OnRep covers any change to the array, so diff against the previous snapshot
	// to fire the event matching what actually changed in each slot.
	for (int32 SlotIndex = 0; SlotIndex < PlayerDatas.Num(); ++SlotIndex)
	{
		const FMKHDuelPlayerData& Current = PlayerDatas[SlotIndex];
		const FMKHDuelPlayerData Previous = PreviousPlayerDatas.IsValidIndex(SlotIndex)
			? PreviousPlayerDatas[SlotIndex]
			: FMKHDuelPlayerData();

		if (Current.PlayerCharacter != Previous.PlayerCharacter && IsValid(Current.PlayerCharacter))
		{
			OnParticipantRegistered.Broadcast(SlotIndex, Current.PlayerCharacter.Get());
		}

		if (Current.Score != Previous.Score)
		{
			OnScoreChanged.Broadcast(SlotIndex, Current.Score);
		}
	}

	PreviousPlayerDatas = PlayerDatas;
}

void AMKHGameState::OnRep_RematchVotes()
{
	OnRematchVotesChanged.Broadcast(RematchVotes.CurrentVotes, RematchVotes.RequiredVotes);
}
