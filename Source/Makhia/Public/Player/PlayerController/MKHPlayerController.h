// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Interfaces/InventoryInterface.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/EquipmentInterface.h"
#include "Interfaces/QuickSlotInterface.h"
#include "MKHPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UInventoryDashboardWidget;
class UMKHInputConfig;
class UInventoryComponent;
class UInventoryDashboardController;
class UMKHAbilitySystemComponent;
class UEquipmentManagerComponent;
class UQuickSlotManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMappingContextChangedSignature, const UInputMappingContext*, NewMappingContext);
DECLARE_DELEGATE_OneParam(FSkillActivatedSignature, FGameplayTag)

UENUM(BlueprintType)
enum class EFocusOption : uint8
{
	Game	UMETA(DisplayName = "Game Focus"),
	UI		UMETA(DisplayName = "UI Focus")
};

/**
 * 
 */
UCLASS()
class MAKHIA_API AMKHPlayerController : public APlayerController, public IAbilitySystemInterface, public IInventoryInterface, public IEquipmentInterface, public IQuickSlotInterface
{
	GENERATED_BODY()
	
	friend class AMainHUD;
	
public:

	AMKHPlayerController();

	virtual void SetupInputComponent() override;

	/**
	 * Clears any leftover gameplay input block when a new pawn is possessed.
	 * The controller survives a seamless travel, so after a rematch it would otherwise keep the
	 * restricted "dead" mapping context applied by the previous match's death.
	 */
	virtual void AcknowledgePossession(APawn* NewPawn) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/* Implement Equipment Interface*/
	virtual UEquipmentManagerComponent* GetEquipmentManagerComponent_Implementation() const override;
	
	/* Implement Inventory Interface*/
	virtual UInventoryComponent* GetInventoryComponent_Implementation() const override;
	
	/* Implement QuickSlot Interface*/
	virtual UQuickSlotManagerComponent* GetQuickSlotManagerComponent_Implementation() const override;

	UMKHAbilitySystemComponent* GetMKHAbilitySystemComponent();
	
	UPROPERTY(BlueprintAssignable)
	FMappingContextChangedSignature MappingContextChangedDelegate;
	
	FSkillActivatedSignature SkillActivatedDelegate;
	
	void AbilityInputPressed(const FGameplayTag& InputTag);
	void AbilityInputReleased(const FGameplayTag& InputTag);

	/**
	 * Enables or disables gameplay input by swapping the active mapping context.
	 * Disabling applies DeadMappingContext (which should only expose the actions that must survive,
	 * e.g. the game menu); a null DeadMappingContext blocks every input.
	 * The blocked flag is remembered, so a focus change while blocked does not silently restore
	 * the default context.
	 *
	 * @param bEnabled  True to restore the default gameplay context, false to apply the restricted one.
	 */
	UFUNCTION(BlueprintCallable, Category = "Custom Values | Input")
	void SetGameplayInputEnabled(bool bEnabled);

	/** True while gameplay input is blocked (e.g. the character is in the Dead state). */
	UFUNCTION(BlueprintPure, Category = "Custom Values | Input")
	bool IsGameplayInputBlocked() const { return bGameplayInputBlocked; }

	/**
	 * Casts this player's vote on the post-match rematch ballot. The match only restarts once
	 * every participant has voted (see AMKHGameMode::RequestRematch).
	 *
	 * UI widgets must route through here because UUserWidget does not implement RPC callspace
	 * resolution, so a UFUNCTION(Server) declared on a widget would never leave the client.
	 */
	UFUNCTION(Server, Reliable)
	void Server_RequestRematch();

	/**
	 * Restarts the match immediately, with no ballot. Used by the in-game pause menu, which is
	 * deliberately unilateral; the end-of-match panel votes through Server_RequestRematch instead.
	 */
	UFUNCTION(Server, Reliable)
	void Server_RestartMatch();

protected:

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void ChangeMappingContext(const UInputMappingContext* NewMappingContext) const;

	bool EnsureWeaponEquipped(const FGameplayTag& InputTag, bool& bForceQueue);
	
private:
	
	UPROPERTY()
	TObjectPtr<UMKHAbilitySystemComponent> MKHAbilitySystemComponent;
	
	// Input actions and mapping context
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input")
	TObjectPtr<UInputMappingContext> UIOpenMappingContext;

	/** Mapping context applied while gameplay input is blocked. Should only map the actions that must
	 *  stay available (e.g. the game menu). Leave unset to block every input. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input")
	TObjectPtr<UInputMappingContext> DeadMappingContext;

	/** True while gameplay input is blocked; keeps focus changes from restoring the default context. */
	bool bGameplayInputBlocked = false;

	UPROPERTY(EditDefaultsOnly, category = "Custom Values | Input")
	TObjectPtr<UMKHInputConfig> MKHInputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input | Actions")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Custom Values | Input | Actions")
	TObjectPtr<UInputAction> ToggleGameMenuAction;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Replicated)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UEquipmentManagerComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UQuickSlotManagerComponent> QuickSlotManagerComponent;
	
	void BindCallbacksToDependencies() const;

	/**
	 * Returns the mapping context that represents "game focus" right now: the restricted one while
	 * gameplay input is blocked, the default one otherwise.
	 */
	const UInputMappingContext* GetGameplayMappingContext() const;

	UFUNCTION(BlueprintCallable, Category = "Custom Values | Input")
	void ChangeFocus(const EFocusOption NewFocus);
	
	void ToggleInventory();
	void ToggleGameMenu();
	
	void SubscribeToEquipEvents() const;
};
