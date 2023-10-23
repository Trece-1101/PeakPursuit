// Copyright 2020-2023 NiceBug Games All Rights Reserved.


#include "Components/ClimbMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "PeakPursuit/DebugHelper.h"
#include "PeakPursuit/PeakPursuitCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "PeakPursuit/PeakPursuitCharacter.h"
#include "MotionWarpingComponent.h"


void UClimbMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UClimbMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    OwningPlayerAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();

    if (OwningPlayerAnimInstance)
    {
        //DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMontageEndedMCDelegate, UAnimMontage*, Montage, bool, bInterrupted);
        OwningPlayerAnimInstance->OnMontageEnded.AddDynamic(this, &UClimbMovementComponent::OnClimbMontageEnded);
        OwningPlayerAnimInstance->OnMontageBlendingOut.AddDynamic(this, &UClimbMovementComponent::OnClimbMontageEnded);
    }
}

void UClimbMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

    float CapsuleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

    if (IsClimbing())
    {
        bOrientRotationToMovement = false;
        CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(CapsuleHalfHeight * 0.5f);

        OnEnterClimbState.ExecuteIfBound();
    }

    if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
    {
        bOrientRotationToMovement = true;
        CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(CapsuleHalfHeight * 2.0f);

        const FRotator DirtyRotation = UpdatedComponent->GetComponentRotation();
        const FRotator CleanStandRotation = FRotator(0.0f, DirtyRotation.Yaw, 0.0f);
        UpdatedComponent->SetRelativeRotation(CleanStandRotation);

        StopMovementImmediately();

        OnExitClimbState.ExecuteIfBound();
    }

}

FVector UClimbMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const
{

    if (IsFalling() && OwningPlayerAnimInstance->IsAnyMontagePlaying())
    {
        return RootMotionVelocity;
    }

    return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);
}


void UClimbMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
    Super::PhysCustom(deltaTime, Iterations);

    if (IsClimbing())
    {
        PhysClimb(deltaTime, Iterations);
    }
}


float UClimbMovementComponent::GetMaxSpeed() const
{
    if (IsClimbing())
    {
        return MaxClimbAcceleration;
    }

    return Super::GetMaxSpeed();
}


float UClimbMovementComponent::GetMaxAcceleration() const
{
    if (IsClimbing())
    {
        return MaxClimbSpeed;
    }

    return Super::GetMaxAcceleration();
}



void UClimbMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
    if (deltaTime < MIN_TICK_TIME)
    {
        return;
    }

    //Process climbable surfaces
    GetClimbableSurfaces();
    ProcessClimbableSurfaceInfo();

    //Check if character should stop climbing
    if (ShouldStopClimbing() || HasReachFloor())
    {
        StopClimbing();
    }

    RestorePreAdditiveRootMotionVelocity();

    if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
    {
        //Define max climb speed and acceleration
        CalcVelocity(deltaTime, 0.0f, true, MaxBrakingDeceleration);
    }

    ApplyRootMotionToVelocity(deltaTime);

    FVector OldLocation = UpdatedComponent->GetComponentLocation();
    const FVector Adjusted = Velocity * deltaTime;
    FHitResult Hit(1.f);

    //Handle Climb rotation
    SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(deltaTime), true, Hit);

    if (Hit.Time < 1.f)
    {      
        HandleImpact(Hit, deltaTime, Adjusted);
        SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
    }

    if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
    {
        Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
    }

    //Snap movement to climbable surfaces
    SnapMovementToClimbableSurfaces(deltaTime);

    if (HasReachLedge())
    {
        PlayClimbMontage(ClimbToTopMontage);
    }
}


void UClimbMovementComponent::ProcessClimbableSurfaceInfo()
{
    CurrentClimbableSurfaceLocation = FVector::ZeroVector;
    CurrentClimbableSurfaceNormal = FVector::ZeroVector;

    if (ClimbTraceResults.IsEmpty()) { return; }

    for (const FHitResult& HitResult : ClimbTraceResults)
    {
        CurrentClimbableSurfaceLocation += HitResult.ImpactPoint;
        CurrentClimbableSurfaceNormal += HitResult.ImpactNormal;
    }

    CurrentClimbableSurfaceLocation /= ClimbTraceResults.Num();
    CurrentClimbableSurfaceNormal = CurrentClimbableSurfaceNormal.GetSafeNormal();
}

bool UClimbMovementComponent::ShouldStopClimbing()
{
    if (ClimbTraceResults.IsEmpty())
    {
        return true;
    }

    const float DotResult = FVector::DotProduct(CurrentClimbableSurfaceNormal, FVector::UpVector);
    const float DegreeDiff = FMath::RadiansToDegrees(FMath::Acos(DotResult));

    if (DegreeDiff <= DegreesSurfaceClimbingThreshold)
    {
        return true;
    }

    return false;
}


bool UClimbMovementComponent::HasReachFloor()
{
    const FVector DownVector = -UpdatedComponent->GetUpVector();
    const FVector StartOffset = DownVector * 50.0f;

    const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
    const FVector End = Start - FVector(0.0f, 0.0f, FloorReachedDetector);
    
    //GetClimbCapsuleTraces(Start, End);
    FHitResult HitResult = GetClimbLineTraces(Start, End);

    if (!HitResult.bBlockingHit) { return false; }

    const bool bFloorReached = FVector::Parallel(-HitResult.ImpactNormal, FVector::UpVector) && GetUnrotatedClimbVelocity().Z < -10.0f;

    if (!bFloorReached) { return false; }

    return true;    
}


bool UClimbMovementComponent::HasReachLedge()
{
    FHitResult LedgeHitResult;
    TraceFromLedgeHeight(LedgeHitResult);

    if (!LedgeHitResult.bBlockingHit)
    {
        const FVector WalkableSurfaceTraceStart = LedgeHitResult.TraceEnd;
        const FVector DownVector = -UpdatedComponent->GetUpVector();
        const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.0f;

        FHitResult FloorHitResult = GetClimbLineTraces(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd);
                
        if (FloorHitResult.bBlockingHit && GetUnrotatedClimbVelocity().Z > 10.0f) { return true; }
    }

    return false;
}


FQuat UClimbMovementComponent::GetClimbRotation(float DeltaTime)
{
    const FQuat CurrentQuat = UpdatedComponent->GetComponentQuat();

    if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
    {
        return CurrentQuat;
    }

    const FQuat TargetQuat = FRotationMatrix::MakeFromX(-CurrentClimbableSurfaceNormal).ToQuat();
    return FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, ClimbRotInterpSpeed);

}


void UClimbMovementComponent::SnapMovementToClimbableSurfaces(float DeltaTime)
{
    const FVector ComponentForward = UpdatedComponent->GetForwardVector();
    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
    
    const FVector ProjectedCharacterToSurface = (CurrentClimbableSurfaceLocation - ComponentLocation).ProjectOnTo(ComponentForward);

    const FVector SnapVector = -CurrentClimbableSurfaceNormal * ProjectedCharacterToSurface.Length();

    UpdatedComponent->MoveComponent(
        SnapVector * DeltaTime * MaxClimbSpeed,
        UpdatedComponent->GetComponentQuat(),
        true
    );
}


TArray<FHitResult> UClimbMovementComponent::GetClimbCapsuleTraces(const FVector& Start, const FVector& End)
{
    TArray<FHitResult> OutHitResults;

    UKismetSystemLibrary::CapsuleTraceMultiForObjects(
        this,
        Start,
        End,
        ClimbCapsuleTraceRadius,
        ClimbCapsuleTraceHeight,
        ClimbableSurfaceTypes,
        false,
        TArray<AActor*>(),
        bShowDebugShape ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
        OutHitResults,
        false,
        FLinearColor::Red,
        FLinearColor::Green,
        ShowDebugDuration
    );

    return OutHitResults;
}


bool UClimbMovementComponent::GetClimbableSurfaces()
{
    //UpdatedComponent es el Capsule Component del Character, que es la raiz
    const FVector StartOffset = UpdatedComponent->GetForwardVector() * (ClimbCapsuleTraceRadius * 0.5f);
    const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
    const FVector End = Start + UpdatedComponent->GetForwardVector();

    ClimbTraceResults = GetClimbCapsuleTraces(Start, End);

    // If not empty return true, if empty return false
    return !ClimbTraceResults.IsEmpty();
}


FHitResult UClimbMovementComponent::GetClimbLineTraces(const FVector& Start, const FVector& End)
{
    FHitResult OutHitResult;

    UKismetSystemLibrary::LineTraceSingleForObjects(
        this,
        Start,
        End,
        ClimbableSurfaceTypes,
        false,
        TArray<AActor*>(),
        bShowDebugShape ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
        OutHitResult,
        false,
        FLinearColor::Blue,
        FLinearColor::Green, 
        ShowDebugDuration
    );

    return OutHitResult;
}


bool UClimbMovementComponent::TraceFromEyeHeight()
{
    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
    const FVector EyesHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + EyesTraceStartOffset);
    const FVector Start = ComponentLocation + EyesHeightOffset;
    const FVector End = Start + UpdatedComponent->GetForwardVector() * EyesTraceDist;

    EyesTraceResult = GetClimbLineTraces(Start, End);

    // If blocking hit return true, if not return false
    return EyesTraceResult.bBlockingHit;
}


FHitResult UClimbMovementComponent::TraceFromHeight(float TraceDistance, float StartOffset)
{
    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
    const FVector EyesHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + StartOffset);
    const FVector Start = ComponentLocation + EyesHeightOffset;
    const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

    return GetClimbLineTraces(Start, End);
}


void UClimbMovementComponent::TraceFromLedgeHeight(FHitResult& OutHitResult)
{
    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
    const FVector LedgeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight + LedgeTraceStartOffset);

    const FVector Start = ComponentLocation + LedgeHeightOffset;
    const FVector End = Start + UpdatedComponent->GetForwardVector() * EyesTraceDist;

    OutHitResult = GetClimbLineTraces(Start, End);
}


void UClimbMovementComponent::TryStartVaulting()
{
    FVector VaultStartPosition;
    FVector VaultLandPosition;

    if (CanStartVaulting(VaultStartPosition, VaultLandPosition))
    {
        //Start Vaulting
        SetMotionWarpTarget(FName("VaultStartPoint"), VaultStartPosition);
        SetMotionWarpTarget(FName("VaultLandPoint"), VaultLandPosition);

        StartClimbing();
        PlayClimbMontage(VaultMontage);
    }
}


void UClimbMovementComponent::SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition)
{
    APeakPursuitCharacter* MyOwner = CastChecked<APeakPursuitCharacter>(GetOwner());
    
    MyOwner->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocation(
        InWarpTargetName, InTargetPosition
    );
}


void UClimbMovementComponent::HandleHopUp()
{
    FVector HopUpTargetPoint;
    if (CanHopUp(HopUpTargetPoint))
    {
        SetMotionWarpTarget(FName("HopUpTargetPoint"), HopUpTargetPoint);
        PlayClimbMontage(HopUpMontage);
    }

}


bool UClimbMovementComponent::CanHopUp(FVector& OutHopUpTargetPos)
{
    FHitResult HopUpHit = TraceFromHeight(100.0f, -20.0f);
    FHitResult LedgeHit = TraceFromHeight(100.0f, 150.0f);

    if (HopUpHit.bBlockingHit && LedgeHit.bBlockingHit)
    {
        OutHopUpTargetPos = HopUpHit.ImpactPoint;
        return true;
    }

    return false;
}


void UClimbMovementComponent::HandleHopDown()
{
    FVector HopDownTargetPoint;
    if (CanHopDown(HopDownTargetPoint))
    {
        SetMotionWarpTarget(FName("HopDownTargetPoint"), HopDownTargetPoint);
        PlayClimbMontage(HopDownMontage);
    }
}


bool UClimbMovementComponent::CanHopDown(FVector& OutHopDownTargetPos)
{
    FHitResult HopDownHit = TraceFromHeight(100.0f, -300.0f);

    if (HopDownHit.bBlockingHit)
    {
        OutHopDownTargetPos = HopDownHit.ImpactPoint;
        return true;
    }

    return false;
}


void UClimbMovementComponent::RequestHoping()
{    
    const FVector UnrotatedLastInputVector = UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), GetLastInputVector());

    const float DotResult = FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), FVector::UpVector);

    if (DotResult >= 0.9f)
    {
        HandleHopUp();
    }
    else if (DotResult <= -0.9f)
    {
        HandleHopDown();
    }
}


bool UClimbMovementComponent::CanStartVaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition)
{
    if (IsFalling()) { return false; }

    OutVaultStartPosition = FVector::ZeroVector;
    OutVaultLandPosition = FVector::ZeroVector;

    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
    const FVector ComponentForward = UpdatedComponent->GetForwardVector();
    const FVector UpVector = UpdatedComponent->GetUpVector();
    const FVector DownVector = -UpdatedComponent->GetUpVector();

    int32 lines = 5;
    for (int32 i = 0; i < lines; i++)
    {
        float lenghtLine = 80.0f * (i + 1);
        const FVector Start = ComponentLocation + UpVector * 100.0f + (ComponentForward * lenghtLine);
        const FVector End = Start + DownVector * lenghtLine;

        FHitResult VaultTracehit = GetClimbLineTraces(Start, End);

        if (i == 0 && VaultTracehit.bBlockingHit)
        {
            OutVaultStartPosition = VaultTracehit.ImpactPoint;
        }

        if (i == (lines - 2) && VaultTracehit.bBlockingHit)
        {
            OutVaultLandPosition = VaultTracehit.ImpactPoint;
        }
    }

    if (OutVaultStartPosition != FVector::ZeroVector && OutVaultLandPosition != FVector::ZeroVector)
    {
        return true;
    }

    return false;
}

void UClimbMovementComponent::ToggleClimbing()
{
    if (IsClimbing())
    {
        // Stop Climbing
        StopClimbing();
    }
    else
    {
        if (CanStartClimbing())
        {
            //StartClimbing();
            PlayClimbMontage(IdleToClimbMontage);
        }
        else if(CanClimbDownLedge())
        {
            PlayClimbMontage(ClimbDownLedgeMontage);
        }
        else
        {
            TryStartVaulting();
        }
    }
}


bool UClimbMovementComponent::IsClimbing() const
{
    return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::MOVE_Climb;
}

bool UClimbMovementComponent::CanStartClimbing()
{
    if (IsFalling() || !TraceFromEyeHeight() || !GetClimbableSurfaces())
    {
        // Si esta cayendo o el trace desde la vista no esta hiteando o no encuentra superficies para escalar no puede escalar
        return false;
    }

    return true;
}


void UClimbMovementComponent::StartClimbing()
{
    SetMovementMode(MOVE_Custom, ECustomMovementMode::MOVE_Climb);
    //Debug::Print("Start climbing");
}


void UClimbMovementComponent::StopClimbing()
{
    SetMovementMode(MOVE_Falling);
    //Debug::Print("Stop climbing");
}


bool UClimbMovementComponent::CanClimbDownLedge()
{
    if (IsFalling()) { return false; }

    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
    const FVector ComponentForward = UpdatedComponent->GetForwardVector();
    const FVector DownVector = -(UpdatedComponent->GetUpVector());


    const FVector WalkableSurfaceTraceStart = ComponentLocation + ComponentForward * ClimbDownWalkableSurfaceTraceOffset;
    const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.0f;

    FHitResult WalkableSurfaceHit = GetClimbLineTraces(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd);

    if (!WalkableSurfaceHit.bBlockingHit) { return false; }

    const FVector LedgeTraceStart = WalkableSurfaceHit.TraceStart + ComponentForward * ClimbDownLedgeTraceOffset;
    const FVector LedgeTraceEnd = LedgeTraceStart + DownVector * ClimbDownWalkableSurfaceTraceDistance;

    FHitResult LedgeTraceHit = GetClimbLineTraces(LedgeTraceStart, LedgeTraceEnd);

    if (LedgeTraceHit.bBlockingHit) { return false; }

    return true;
}


void UClimbMovementComponent::PlayClimbMontage(UAnimMontage* MontageToPlay)
{
    if (!MontageToPlay) { return; }

    if (OwningPlayerAnimInstance->IsAnyMontagePlaying()) { return; }

    OwningPlayerAnimInstance->Montage_Play(MontageToPlay);
}


void UClimbMovementComponent::OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == IdleToClimbMontage || Montage == ClimbDownLedgeMontage)
    {
        StartClimbing();
        StopMovementImmediately();
    }

    if (Montage == ClimbToTopMontage || Montage == VaultMontage)
    {
        SetMovementMode(MOVE_Walking);
    }

    //Debug::Print(*Montage->GetName());
}


FVector UClimbMovementComponent::GetUnrotatedClimbVelocity() const
{
    return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), Velocity);
}
