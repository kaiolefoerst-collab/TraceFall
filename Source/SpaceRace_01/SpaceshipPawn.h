// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "InputActionValue.h"
#include "SpaceshipPawn.generated.h"

class UInputMappingContext;
class USoundBase;

UCLASS()
class SPACERACE_01_API ASpaceshipPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASpaceshipPawn();

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* SpaceshipMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCameraComponent* CockpitCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float MaxSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float MaxSpeedVertical = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float MassSpaceship = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float ThrustForward = 4000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float ThrustBackward = 4000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float ThrustVertical = 4000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float ThrustLateral = 4000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float MaxSpeedLateral = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float VelocityDamping = 0.46f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float ManeuverDamping = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision")
	float TranslationBounceFactor = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation")
	float PitchTorque = -30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation")
	float YawTorque = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation")
	float RollTorque = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation")
	float MaxPitchRate = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation")
	float MaxYawRate = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation")
	float MaxRollRate = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Rotation")
	float RotationDamping = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> SpaceshipMappingContext;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sound")
	UAudioComponent* EngineAudioComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> EngineCueSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> EngineStartSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> CollisionSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float CollisionSoundCooldown = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float EngineStartVolumeMultiplier = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float CollisionVolumeMultiplier = 10.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void HandleThrust(const FInputActionValue& Value);
	void HandleVerticalThrust(const FInputActionValue& Value);
	void HandleLateralThrust(const FInputActionValue& Value);
	void HandleSteering(const FInputActionValue& Value);
	void HandleRoll(const FInputActionValue& Value);
	void AddSpaceshipMappingContext();
	void DisplayForwardSpeedDebug(float ForwardSpeedMS) const;
	void UpdateEngineSound(bool bAnyThrustActive);
	void PlayCollisionSound();

	float ThrustInput = 0.0f;
	float VerticalThrustInput = 0.0f;
	float LateralThrustInput = 0.0f;
	FVector MainEngineVelocity = FVector::ZeroVector;
	FVector ManeuverVelocity = FVector::ZeroVector;
	FVector LateralVelocity = FVector::ZeroVector;
	FVector2D SteeringInput = FVector2D::ZeroVector;
	float RollInput = 0.0f;
	FVector AngularVelocity = FVector::ZeroVector;
	FQuat HorizonRotation = FQuat::Identity;
	float RollAngle = 0.0f;
	bool bRotationInitialized = false;
	FTransform LastSafeTransform;
	bool bEngineSoundActive = false;
	float LastCollisionSoundTime = -1000.0f;

};
