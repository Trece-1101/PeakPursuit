// Copyright 2020-2023 NiceBug Games All Rights Reserved.


#include "Animation/CharacterAnimInstance.h"
#include "PeakPursuit/PeakPursuitCharacter.h"
#include "Components/ClimbMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UCharacterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    MyCharacter = Cast<APeakPursuitCharacter>(TryGetPawnOwner());

    if (IsValid(MyCharacter))
    {
        ClimbMovementComponent = MyCharacter->GetClimbMovementComponent();
    }
}


void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!IsValid(MyCharacter) || !ClimbMovementComponent) { return; }

    GetGroundSpeed();
    GetAirSpeed();
    GetShouldMove();
    GetIsFalling();
    GetIsClimbing();
    GetClimbVelocity();
}


void UCharacterAnimInstance::GetGroundSpeed()
{
    GroundSpeed = UKismetMathLibrary::VSizeXY(MyCharacter->GetVelocity());
}


void UCharacterAnimInstance::GetAirSpeed()
{
    AirSpeed = MyCharacter->GetVelocity().Z;
}


void UCharacterAnimInstance::GetShouldMove()
{
    bool IsAccelerating = ClimbMovementComponent->GetCurrentAcceleration().Size() > 0.0;

    bShouldMove = IsAccelerating && GroundSpeed > 5.0f && !bIsFalling;
}


void UCharacterAnimInstance::GetIsFalling()
{
    bIsFalling = ClimbMovementComponent->IsFalling();
}


void UCharacterAnimInstance::GetIsClimbing()
{
    bIsClimbing = ClimbMovementComponent->IsClimbing();
}

void UCharacterAnimInstance::GetClimbVelocity()
{
    ClimbVelocity = ClimbMovementComponent->GetUnrotatedClimbVelocity();
}
