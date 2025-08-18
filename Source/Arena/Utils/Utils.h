// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Utility class for generic interpolation functions
 */
class ARENA_API FUtils
{
public:
    /**
     * Generic interpolation function that smoothly transitions a value towards a target
     * Similar to FInterpTo but with customizable behavior
     * 
     * @param CurrentValue - The current value being interpolated (modified in place)
     * @param TargetValue - The target value to reach
     * @param Rate - The interpolation rate (higher = faster)
     * @param DeltaTime - Time elapsed since last frame
     * @param Tolerance - Tolerance for considering values "equal"
     * @return true if interpolation is complete, false if still interpolating
     */
    template<typename T>
    static bool HandleGenericInterpolation(T& CurrentValue, const T& TargetValue, const T& Rate, 
                                         float DeltaTime, const T& Tolerance = T(0.01))
    {
        // Check if we're already at target (within tolerance)
        if (FMath::Abs(CurrentValue - TargetValue) <= Tolerance)
        {
            CurrentValue = TargetValue;
            return true; // Interpolation complete
        }
        
        // Calculate interpolation step
        T Step = Rate * DeltaTime;
        
        // Move towards target
        if (CurrentValue < TargetValue)
        {
            CurrentValue += Step;
            if (CurrentValue > TargetValue)
                CurrentValue = TargetValue;
        }
        else if (CurrentValue > TargetValue)
        {
            CurrentValue -= Step;
            if (CurrentValue < TargetValue)
                CurrentValue = TargetValue;
        }
        
        // Check if we reached the target
        return FMath::Abs(CurrentValue - TargetValue) <= Tolerance;
    }
};