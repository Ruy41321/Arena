// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "QuickSlotManagerComponent.generated.h"

class URPGSystemInputComponent;
class URPGInputConfig;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UQuickSlotManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UQuickSlotManagerComponent();

	void SetupInputActions(URPGInputConfig* InputConfig, URPGSystemInputComponent* InputComponent);
	
	void AddQuickSlot(const FGameplayTag& QuickSlotTag, int64 ItemID);
	
	void RemoveQuickSlot(int64 ItemID);
	
	void UseQuickSlot(const FGameplayTag& QuickSlotTag);
	
	int64 GetQuickSlotID(const FGameplayTag& QuickSlotTag) const;
	
private:
	
	TMap<FGameplayTag, int64> QuickSlotTagMap;
};
