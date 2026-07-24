// Copyright (c) 2025 Luigi Pennisi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/MovementStateMachine/MovementStateTypes.h"
#include "Interfaces/MKHAbilitySystemInterface.h"
#include "Character/CharacterBase.h"
#include "MKHPlayerCharacter.generated.h"

// Forward declarations for Enhanced Input
class UInputMappingContext;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;

class UDodgeSystemComponent;
class UCrouchSystemComponent;
class UBlockingSystemComponent;
class UBasicMovementComponent;
class UJumpSystemComponent;
class USprintSystemComponent;
class UMovementStateMachine;
class UAnimMontage;
struct FGameplayTag;

/**
 * Broadcast on every peer whenever the replicated username changes.
 * The username reaches remote peers asynchronously (owning client -> server RPC -> replication back),
 * so UI must react to this event rather than reading the name once at registration time.
 *
 * @param NewUsername  The username now in effect on this machine.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMKHOnUsernameChangedSignature, const FString&, NewUsername);


UCLASS()
class MAKHIA_API AMKHPlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	AMKHPlayerCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** Sets the character's maximum walk speed */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMaxWalkSpeed(float NewSpeed);

	// Movement State Machine helper functions
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	EMovementStateValue GetCurrentMovementState() const;

	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	EMovementStateValue GetPreviousMovementState() const;

	UFUNCTION(BlueprintCallable, Category = "Movement State Machine")
	bool TransitionToMovementState(EMovementStateValue NewState, bool bForceTransition = false);
	
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Movement State Machine")
	void Multicast_TransitionToMovementState(EMovementStateValue NewState, bool bForceTransition = false);

	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	FString GetCurrentMovementStateAsString() const;

	/** Get the movement state machine component */
	UFUNCTION(BlueprintPure, Category = "Movement State Machine")
	UMovementStateMachine* GetMovementStateMachine() const { return MovementStateMachine.Get(); }

	/** Unsubscribe from movement state change events */
	UFUNCTION(BlueprintCallable, Category = "Movement State Machine")
	void UnsubscribeFromMovementStateChanges(UObject* Subscriber);

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	/**
	 * Re-initializes the ability actor info when the controller replicates after the player
	 * state. PlayerState and Controller replication order is not guaranteed; if the actor
	 * info was built with a null controller, LocalPredicted abilities stay non-local forever
	 * unless this re-runs the init once the controller arrives.
	 */
	virtual void OnRep_Controller() override;
	
	/**
	 * Stops the given montage on every machine (server, owner, and simulated proxies).
	 * Needed because GAS does not reliably replicate a montage stop to simulated proxies
	 * when the owning ability is cancelled externally (e.g. a stun interrupted by an attack).
	 *
	 * @param MontageToStop    The specific montage to stop; other montages are left untouched.
	 * @param InBlendOutTime   Blend-out time used when stopping the montage.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopMontage(UAnimMontage* MontageToStop, const float InBlendOutTime);

	/**
	 * Snaps the character back to its spawn transform on every machine (server, owner and simulated proxies).
	 * A server-side SetActorTransform is not enough: the movement correction sent to the owning client
	 * (ClientAdjustPosition) carries location and velocity but no rotation, so the owner would keep the
	 * orientation it had before the reset. This also restores the control rotation locally, which drives
	 * the camera boom and, through bOrientRotationToMovement, where the character turns on the next input.
	 *
	 * Depends on p.UseLastGoodRotationDuringCorrection being disabled (see DefaultEngine.ini): with the
	 * engine default the next correction would restore the client's own pre-reset rotation on top of this.
	 *
	 * @param SpawnTransform  Transform captured when the participant was registered, at its first spawn.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ResetToSpawnTransform(const FTransform& SpawnTransform);

	/** Returns the player's display username (replicated to every peer). */
	UFUNCTION(BlueprintPure, Category = "Player|Identity")
	const FString& GetUsername() const { return Username; }

	/**
	 * Sets the player's display username. Callable from the owning client: the value is routed to
	 * the server and then replicated back to every peer, so all machines observe the same username.
	 *
	 * @param NewUsername  The username to assign.
	 */
	UFUNCTION(BlueprintCallable, Category = "Player|Identity")
	void SetUsername(const FString& NewUsername);

	/**
	 * Fired on this machine whenever the username changes: on the server when it is committed, and on
	 * each client when the new value replicates in. Bind UI here instead of reading the name from the
	 * game state's player data, which is populated as soon as the character reference is registered —
	 * several frames before the username round-trip completes.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Player|Identity")
	FMKHOnUsernameChangedSignature OnUsernameChanged;

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo() override;
	virtual void Landed(const FHitResult& Hit) override;

	virtual void BindCallbacksToDependencies() override;
	virtual void InitClassDefaults() override;
	virtual void BroadcastInitialValues() override;

	/**
	 * Reacts to a crowd-control gameplay tag (Stunned / Staggered) being added or removed.
	 * When the tag becomes active it cancels ongoing movement actions by forcing an uncrouch
	 * and interrupting any in-progress jump, so crowd control fully suppresses movement.
	 *
	 * @param CallbackTag  The crowd-control tag whose count changed.
	 * @param NewCount     The new stack count for the tag (greater than zero means active).
	 */
	void HandleCrowdControlTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Dodge System")
	TObjectPtr<UDodgeSystemComponent> DodgeSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Crouch System")
	TObjectPtr<UCrouchSystemComponent> CrouchSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Blocking System")
	TObjectPtr<UBlockingSystemComponent> BlockingSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Basic Movement")
	TObjectPtr<UBasicMovementComponent> BasicMovementSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Jump System")
	TObjectPtr<UJumpSystemComponent> JumpSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Sprint System")
	TObjectPtr<USprintSystemComponent> SprintSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement State Machine")
	TObjectPtr<UMovementStateMachine> MovementStateMachine;
		
	// Property to indicates whenever the character is holding a weapon to switch locomotion blendspace
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Animation")
	bool bIsHoldingWeapon = false;
	
protected:
	// Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, category = "Camera")
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

private:
	/**
	 * Seeds the username from the local game instance for the locally controlled pawn, once.
	 * Called from the possession hooks (PossessedBy on the host, OnRep_Controller on the owning
	 * client) where the controller — and therefore ownership required to route the server RPC — is
	 * guaranteed to be in place. Idempotent and a no-op on non-locally-controlled pawns.
	 */
	void TrySeedUsernameFromLocalGameInstance();

	/**
	 * Realigns the controller view rotation with the given spawn orientation.
	 * Only applied on the machines that actually own the view (the locally controlled client and the
	 * authority): anywhere else the control rotation is unused or overwritten by the owner's updates.
	 *
	 * @param SpawnRotation  Actor rotation the character spawned with; only its yaw is used.
	 */
	void ApplySpawnViewRotation(const FRotator& SpawnRotation);

	/** Server RPC that commits the username on the authority so it replicates to every peer. */
	UFUNCTION(Server, Reliable)
	void Server_SetUsername(const FString& NewUsername);

	/** Replication callback: notifies this client that the username arrived from the server. */
	UFUNCTION()
	void OnRep_Username();

	/** Player's display username, replicated to every peer. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, ReplicatedUsing = OnRep_Username, meta=(AllowPrivateAccess = "true"), Category = "MKHPlayerCharacter")
	FString Username = "Player";

	/** Guards TrySeedUsernameFromLocalGameInstance so the initial seed runs only once per machine. */
	bool bUsernameSeeded = false;

};