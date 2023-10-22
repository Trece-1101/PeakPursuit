// Copyright 2020-2023 NiceBug Games All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ClimbMovementComponent.generated.h"

DECLARE_DELEGATE(FOnEnterClimbState)
DECLARE_DELEGATE(FOnExitClimbState)

UENUM(BlueprintType)
namespace ECustomMovementMode {
	enum Type {
		MOVE_Climb UMETA(DisplayName = "Climb Mode")
	};
}

/**
 * 
 */
UCLASS()
class PEAKPURSUIT_API UClimbMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	FOnEnterClimbState OnEnterClimbState;
	FOnExitClimbState OnExitClimbState;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** Called after MovementMode has changed. Base implementation does special handling for starting certain modes, then notifies the CharacterOwner. */
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const;

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;

#pragma region Attributes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	TArray<TEnumAsByte<EObjectTypeQuery> > ClimbableSurfaceTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float ClimbCapsuleTraceRadius = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float ClimbCapsuleTraceHeight = 72.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float EyesTraceDist = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float EyesTraceStartOffset = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float LedgeTraceStartOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float MaxBrakingDeceleration = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float MaxClimbSpeed = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float MaxClimbAcceleration = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float ClimbRotInterpSpeed = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float DegreesSurfaceClimbingThreshold = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float FloorReachedDetector = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float ClimbDownWalkableSurfaceTraceOffset = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float ClimbDownLedgeTraceOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing")
	float ClimbDownWalkableSurfaceTraceDistance = 200.0f;


	UPROPERTY()
	class UAnimInstance* OwningPlayerAnimInstance;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Animations")
	class UAnimMontage* IdleToClimbMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Animations")
	class UAnimMontage* ClimbToTopMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Animations")
	class UAnimMontage* ClimbDownLedgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Animations")
	class UAnimMontage* VaultMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Animations")
	class UAnimMontage* HopUpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Animations")
	class UAnimMontage* HopDownMontage;


	TArray<FHitResult> ClimbTraceResults;
	FVector CurrentClimbableSurfaceLocation;
	FVector CurrentClimbableSurfaceNormal;
	FHitResult EyesTraceResult;
	FHitResult LedgeTraceResult;

	//Debug
	UPROPERTY(EditAnywhere, Category = "Character Movement: Debug")
	bool bShowDebugShape = false;

	UPROPERTY(EditAnywhere, Category = "Character Movement: Debug")
	float ShowDebugDuration = 0.1f;
#pragma endregion

#pragma region Methods
private:
	TArray<FHitResult> GetClimbCapsuleTraces(const FVector& Start, const FVector& End);
	FHitResult GetClimbLineTraces(const FVector& Start, const FVector& End);
	bool GetClimbableSurfaces();
	bool TraceFromEyeHeight();
	FHitResult TraceFromHeight(float TraceDistance, float StartOffset);
	void TraceFromLedgeHeight(FHitResult& OutHitResult);
	bool CanStartClimbing();
	void StartClimbing();
	void StopClimbing();
	bool CanClimbDownLedge();
	void PhysClimb(float deltaTime, int32 Iterations);
	void ProcessClimbableSurfaceInfo();
	bool ShouldStopClimbing();
	bool HasReachFloor();
	bool HasReachLedge();
	void TryStartVaulting();
	bool CanStartVaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition);
	FQuat GetClimbRotation(float DeltaTime);
	void SnapMovementToClimbableSurfaces(float DeltaTime);
	void PlayClimbMontage(class UAnimMontage* MontageToPlay);
	void SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition);
	void HandleHopUp();
	bool CanHopUp(FVector& OutHopUpTargetPos);
	void HandleHopDown();
	bool CanHopDown(FVector& OutHopDownTargetPos);


	UFUNCTION()
	void OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);


public:
	FORCEINLINE FVector GetClimbableSurfaceNormal() const { return CurrentClimbableSurfaceNormal; }
	FVector GetUnrotatedClimbVelocity() const;

	bool IsClimbing() const;
	void ToggleClimbing();
	void RequestHoping();
#pragma endregion


	
};
