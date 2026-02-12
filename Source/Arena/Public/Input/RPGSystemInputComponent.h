// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/RPGInputConfig.h"
#include "RPGSystemInputComponent.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API URPGSystemInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()
	
public:

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(URPGInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc);

};

template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void URPGSystemInputComponent::BindAbilityActions(URPGInputConfig* InputConfig, 
	UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc)
{
	check(InputConfig);

	for (const FRPGInputAction& Action : InputConfig->RPGInputActions)
	{
		if (IsValid(Action.InputAction) && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				FGameplayTag Tag = Action.InputTag;
				BindActionValueLambda(Action.InputAction, ETriggerEvent::Started,
					[Object, PressedFunc, Tag](const FInputActionValue&)
					{
						(Object->*PressedFunc)(Tag);
					});
			}
			if (ReleasedFunc)
			{
				FGameplayTag Tag = Action.InputTag;
				BindActionValueLambda(Action.InputAction, ETriggerEvent::Completed,
					[Object, ReleasedFunc, Tag](const FInputActionValue&)
					{
						(Object->*ReleasedFunc)(Tag);
					});
			}
		}
	}
}
