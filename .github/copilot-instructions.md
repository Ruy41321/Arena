# Project context for GitHub Copilot (Visual Studio)

## Domain
- Unreal Engine 5 C++ game project.
- Primary gameplay class: APlayerCharacter with components:
  - UDodgeSystemComponent, UCrouchSystemComponent, UBasicMovementComponent, UJumpSystemComponent, USprintSystemComponent
  - UMovementStateMachine with EMovementState, MovementStateTypes
- Uses Enhanced Input (UInputMappingContext, UInputAction).
- Camera via UCameraComponent + USpringArmComponent.
- Target C++ standard: C++20.

## Coding conventions
- Follow Unreal naming and types: A for actors, U for UObject-derived, F for structs, E for enums, TArray/TObjectPtr, etc.
- Prefer TObjectPtr for UPROPERTY pointers. Use UPROPERTY with appropriate visibility and Category metadata.
- Keep includes minimal; forward-declare where possible. Use #pragma once.
- Do not break existing public APIs unless explicitly asked. Preserve BlueprintCallable/BlueprintPure metadata.
- Match existing formatting (brace style, spacing) seen in Source/Arena files.

## Behavior expectations
- Movement State Machine:
  - Expose getters for current/previous state.
  - TransitionToMovementState(NewState, bForceTransition=false) should validate transitions and fire state change events.
- Input:
  - Bind Enhanced Input actions via SetupPlayerInputComponent with the DefaultMappingContext.
- Camera:
  - CameraBoom + Camera setup; avoid direct tick cost unless needed.

## Response preferences
- Generate code that compiles for UE5 with C++20.
- Favor small, focused methods; avoid placeholders.
- When modifying existing files, preserve surrounding code and comments.
- Do not create documentation files .md or .txt unless requested.