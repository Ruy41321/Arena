// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WidgetController.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UWidgetController : public UObject
{
	GENERATED_BODY()
	
public:
	
	void SetOwningActor(AActor* InOwner);

	virtual void BindCallbacksToDependencies();

	virtual void BroadcastInitialValues();
		
	/**
	 * Binds the delegates of this controller to the appropriate events on the HUD widget.
	 * Model notify View.
	 */
	UFUNCTION(BlueprintNativeEvent)
	void BindDelegatesToWidget();
	virtual void BindDelegatesToWidget_Implementation();
	
	virtual void UnbindAllEventsFromDelegates();
	
protected:
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<AActor> OwningActor;
	
};
