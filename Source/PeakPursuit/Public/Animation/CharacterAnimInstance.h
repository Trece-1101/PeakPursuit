// Copyright 2020-2023 NiceBug Games All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class PEAKPURSUIT_API UCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

private:
	UPROPERTY()
	class APeakPursuitCharacter* MyCharacter;

	UPROPERTY()
	class UClimbMovementComponent* ClimbMovementComponent;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference)
	float GroundSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference)
	float AirSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference)
	bool bShouldMove;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference)
	bool bIsFalling;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference)
	bool bIsClimbing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference)
	FVector ClimbVelocity;

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	void GetGroundSpeed();
	void GetAirSpeed();
	void GetShouldMove();
	void GetIsFalling();
	void GetIsClimbing();
	void GetClimbVelocity();
};
