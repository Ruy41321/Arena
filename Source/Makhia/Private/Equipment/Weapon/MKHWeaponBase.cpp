// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/Weapon/MKHWeaponBase.h"

#include "MKHLogChannels.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

namespace
{
	/** Lower bound for the scan step frequency to avoid degenerate timer rates. */
	constexpr float MinScanStepFrequency = 0.005f;

	/** Lower bound for the sweep sphere radius so a zero-configured weapon still detects hits. */
	constexpr float MinScanRadius = 10.f;
}

// ============================================================
// Lifecycle
// ============================================================

AMKHWeaponBase::AMKHWeaponBase()
{
	TraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStart"));
	TraceStart->SetupAttachment(GetRootComponent());

	TraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("TraceEnd"));
	TraceEnd->SetupAttachment(GetRootComponent());

	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(GetRootComponent());

	ScanObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
}

// ============================================================
// Public Interface
// ============================================================

void AMKHWeaponBase::StartMeleeScan(float StepFrequency)
{
	if (!IsValid(TraceStart) || !IsValid(TraceEnd))
	{
		UE_LOG(LogMKHAbility, Warning, TEXT("StartMeleeScan aborted on %s: missing TraceStart/TraceEnd points."), *GetNameSafe(this));
		return;
	}

	// Restarting an active scan begins a new window (new swing): drop the previous hit set.
	HitActors.Reset();
	PreviousBladeStart = TraceStart->GetComponentLocation();
	PreviousBladeEnd = TraceEnd->GetComponentLocation();
	bMeleeScanActive = true;

	const float ClampedFrequency = FMath::Max(StepFrequency, MinScanStepFrequency);
	GetWorldTimerManager().SetTimer(HitScanTimer, this, &AMKHWeaponBase::PerformMeleeScanStep, ClampedFrequency, true);

	// Immediate first step catches targets already overlapping the blade at swing start.
	PerformMeleeScanStep();
}

void AMKHWeaponBase::StopMeleeScan()
{
	GetWorldTimerManager().ClearTimer(HitScanTimer);
	bMeleeScanActive = false;
}

bool AMKHWeaponBase::IsMeleeScanActive() const
{
	return bMeleeScanActive;
}

void AMKHWeaponBase::SetWeaponDamage(float InDamage)
{
	WeaponDamage = InDamage;
}

void AMKHWeaponBase::SetBlockStabilityPercent(float InBlockStabilityPercent)
{
	BlockStabilityPercent = InBlockStabilityPercent;
}

float AMKHWeaponBase::GetBlockStabilityPercent() const
{
	return BlockStabilityPercent;
}

float AMKHWeaponBase::GetWeaponDamage() const
{
	return WeaponDamage;
}

FVector AMKHWeaponBase::GetProjectileSpawnLocation() const
{
	return ProjectileSpawnPoint ? ProjectileSpawnPoint->GetComponentLocation() : GetActorLocation();
}

// ============================================================
// Protected / Internal Logic
// ============================================================

void AMKHWeaponBase::PerformMeleeScanStep()
{
	if (!bMeleeScanActive || !IsValid(TraceStart) || !IsValid(TraceEnd))
	{
		StopMeleeScan();
		return;
	}

	const FVector CurrentBladeStart = TraceStart->GetComponentLocation();
	const FVector CurrentBladeEnd = TraceEnd->GetComponentLocation();

	// Static coverage: the blade segment as it stands this step.
	SweepAndReportHits(CurrentBladeStart, CurrentBladeEnd);

	// Motion coverage: sweep each blade sample from its previous-step position to the current
	// one, so targets crossed between two steps of a fast swing are still detected.
	for (int32 SampleIndex = 0; SampleIndex < NumBladeSamples; ++SampleIndex)
	{
		const float Alpha = NumBladeSamples > 1 ? static_cast<float>(SampleIndex) / static_cast<float>(NumBladeSamples - 1) : 0.5f;
		const FVector PreviousSample = FMath::Lerp(PreviousBladeStart, PreviousBladeEnd, Alpha);
		const FVector CurrentSample = FMath::Lerp(CurrentBladeStart, CurrentBladeEnd, Alpha);

		if (!PreviousSample.Equals(CurrentSample, KINDA_SMALL_NUMBER))
		{
			SweepAndReportHits(PreviousSample, CurrentSample);
		}
	}

	PreviousBladeStart = CurrentBladeStart;
	PreviousBladeEnd = CurrentBladeEnd;
}

void AMKHWeaponBase::SweepAndReportHits(const FVector& SweepStart, const FVector& SweepEnd)
{
	TArray<AActor*> IgnoredActors;
	BuildScanIgnoreList(IgnoredActors);

	TArray<FHitResult> SweepHits;
	UKismetSystemLibrary::SphereTraceMultiForObjects(
		this,
		SweepStart,
		SweepEnd,
		FMath::Max(HitScanRadius, MinScanRadius),
		ScanObjectTypes,
		false,
		IgnoredActors,
		bDrawDebugScan ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		SweepHits,
		true
	);

	for (const FHitResult& Hit : SweepHits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!IsValid(HitActor) || HitActors.Contains(HitActor))
		{
			continue;
		}

		HitActors.Add(HitActor);
		OnMeleeHitDetected.Broadcast(Hit);
	}
}

void AMKHWeaponBase::BuildScanIgnoreList(TArray<AActor*>& OutIgnoredActors) const
{
	OutIgnoredActors.Reset();
	OutIgnoredActors.Add(const_cast<AMKHWeaponBase*>(this));

	if (AActor* OwnerActor = GetOwner())
	{
		OutIgnoredActors.AddUnique(OwnerActor);
	}
	if (APawn* InstigatorPawn = GetInstigator())
	{
		OutIgnoredActors.AddUnique(InstigatorPawn);
	}
	if (AActor* AttachParent = GetAttachParentActor())
	{
		OutIgnoredActors.AddUnique(AttachParent);
	}
}
