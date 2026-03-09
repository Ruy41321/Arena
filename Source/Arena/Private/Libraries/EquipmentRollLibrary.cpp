// Fill out your copyright notice in the Description page of Project Settings.

#include "Libraries/EquipmentRollLibrary.h"

#include "Equipment/Rarity/RarityDefinition.h"
#include "Equipment/EquipmentDefinition.h"
#include "Equipment/EquipmentTypes.h"
#include "Data/EquipmentStatEffects.h"
#include "Libraries/RPGAbilitySystemLibrary.h"
#include "Engine/DataTable.h"

int32 UEquipmentRollLibrary::WeightedRandomSelect(const TArray<float>& Weights)
{
	if (Weights.Num() == 0)
	{
		return -1;
	}

	float TotalWeight = 0.f;
	for (const float W : Weights)
	{
		TotalWeight += W;
	}

	if (TotalWeight <= 0.f)
	{
		return -1;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	float Accumulator = 0.f;

	for (int32 i = 0; i < Weights.Num(); ++i)
	{
		Accumulator += Weights[i];
		if (Roll <= Accumulator)
		{
			return i;
		}
	}

	return Weights.Num() - 1;
}

const FRarityDefinition* UEquipmentRollLibrary::RollRarity(const UDataTable* RarityTable)
{
	if (!RarityTable)
	{
		return nullptr;
	}

	TArray<FRarityDefinition*> Rows;
	RarityTable->GetAllRows<FRarityDefinition>(TEXT("RollRarity"), Rows);

	float TotalWeight = 0.f;
	for (const FRarityDefinition* Row : Rows)
	{
		TotalWeight += Row->RollProbability;
	}

	if (TotalWeight <= 0.f)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	float Accumulator = 0.f;

	for (const FRarityDefinition* Row : Rows)
	{
		Accumulator += Row->RollProbability;
		if (Roll <= Accumulator)
		{
			return Row;
		}
	}

	// Fallback: return the last row
	return Rows.Last();
}

TArray<FEquipmentStatEffectDefinition> UEquipmentRollLibrary::RollPassiveStats(
	const UEquipmentDefinition* EquipmentCDO,
	const UEquipmentStatEffects* StatData,
	int32 NumStats)
{
	TArray<FEquipmentStatEffectDefinition> Result;

	if (!EquipmentCDO || !StatData || NumStats <= 0)
	{
		return Result;
	}

	// Build the weighted candidate pool from PossibleStatRolls
	TArray<const FEquipmentStatEffectDefinition*> CandidatePool;
	TArray<float> CandidateWeights;

	const FGameplayTagContainer& PossibleTags = EquipmentCDO->PossibleStatRolls;

	for (int32 i = 0; i < PossibleTags.Num(); ++i)
	{
		const FGameplayTag& Tag = PossibleTags.GetByIndex(i);

		for (const auto& Pair : StatData->MasterStatMap)
		{
			if (!Tag.MatchesTag(Pair.Key))
			{
				continue;
			}

			const FEquipmentStatEffectDefinition* PossibleStat =
				URPGAbilitySystemLibrary::GetDataTableRowByTag<FEquipmentStatEffectDefinition>(Pair.Value, Tag);

			if (PossibleStat && PossibleStat->ProbabilityToSelect > 0.f)
			{
				CandidatePool.Add(PossibleStat);
				CandidateWeights.Add(PossibleStat->ProbabilityToSelect);
			}
			break;
		}
	}

	if (CandidatePool.Num() == 0)
	{
		return Result;
	}

	// Roll exactly NumStats stats using weighted selection
	for (int32 i = 0; i < NumStats; ++i)
	{
		const int32 SelectedIndex = WeightedRandomSelect(CandidateWeights);
		if (SelectedIndex < 0)
		{
			break;
		}

		const FEquipmentStatEffectDefinition* Selected = CandidatePool[SelectedIndex];

		FEquipmentStatEffectDefinition NewStat = *Selected;
		NewStat.CurrentValue = Selected->bFractionalStat
			? FMath::FRandRange(Selected->MinStatLevel, Selected->MaxStatLevel)
			: static_cast<float>(FMath::TruncToInt(FMath::RandRange(Selected->MinStatLevel, Selected->MaxStatLevel)));

		Result.Add(NewStat);
	}

	return Result;
}

TArray<FEquipmentAbilityDefinition> UEquipmentRollLibrary::RollActiveAbilities(
	const UEquipmentDefinition* EquipmentCDO,
	const UEquipmentStatEffects* StatData,
	int32 NumAbilities)
{
	TArray<FEquipmentAbilityDefinition> Result;

	if (!EquipmentCDO || !StatData || NumAbilities <= 0)
	{
		return Result;
	}

	// Build the weighted candidate pool from PossibleAbilityRolls
	TArray<const FEquipmentAbilityDefinition*> CandidatePool;
	TArray<float> CandidateWeights;

	const FGameplayTagContainer& PossibleTags = EquipmentCDO->PossibleAbilityRolls;

	for (int32 i = 0; i < PossibleTags.Num(); ++i)
	{
		const FGameplayTag& Tag = PossibleTags.GetByIndex(i);

		for (const auto& Pair : StatData->MasterStatMap)
		{
			if (!Tag.MatchesTag(Pair.Key))
			{
				continue;
			}

			const FEquipmentAbilityDefinition* PossibleAbility =
				URPGAbilitySystemLibrary::GetDataTableRowByTag<FEquipmentAbilityDefinition>(Pair.Value, Tag);

			if (PossibleAbility && PossibleAbility->ProbabilityToSelect > 0.f)
			{
				CandidatePool.Add(PossibleAbility);
				CandidateWeights.Add(PossibleAbility->ProbabilityToSelect);
			}
			break;
		}
	}

	if (CandidatePool.Num() == 0)
	{
		return Result;
	}

	// Roll exactly NumAbilities abilities using weighted selection
	for (int32 i = 0; i < NumAbilities; ++i)
	{
		const int32 SelectedIndex = WeightedRandomSelect(CandidateWeights);
		if (SelectedIndex < 0)
		{
			break;
		}

		Result.Add(*CandidatePool[SelectedIndex]);
	}

	return Result;
}

