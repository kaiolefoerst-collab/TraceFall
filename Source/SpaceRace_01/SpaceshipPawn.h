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
class UCockpitDisplayWidget;

// Governs how much automatic translation assistance is applied on top of raw thrust + gravity.
// Further modes (e.g. velocity-hold, docking assist, ...) are meant to be added here later
// without changing this enum's basic shape.
UENUM(BlueprintType)
enum class EFlightAssistantMode : uint8
{
	// Fully manual, physically direct translation: no damping, no speed clamping, no automatic
	// re-alignment of the existing velocity vector to the ship's orientation.
	None,

	// Same underlying physics as None (no damping, no clamping, no directly zeroing/setting any
	// velocity component), but additionally uses the existing lateral/vertical thrusters to
	// actively counter-accelerate RightVelocity and UpVelocity toward zero, keeping the velocity
	// vector aligned with the ship's local Forward axis. ForwardVelocity is left untouched.
	PureForward
};

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

	// None = fully manual translation (no damping/clamping). See EFlightAssistantMode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assistant")
	EFlightAssistantMode FlightAssistantMode = EFlightAssistantMode::None;

	// Gameplay gravitational constant for a = GravityConstant * PlanetMass / DistanceSquared.
	// Tune experimentally to get the desired gravity strength.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Gravity")
	float GravityConstant = 4000000000.0f;

	// Maximum amount of fuel the tank can hold.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Fuel")
	float FuelTankCapacity = 1000.0f;

	// Current fuel level, clamped to [0, FuelTankCapacity].
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Fuel")
	float CurrentFuel = 1000.0f;

	// Shared efficiency divisor for all nozzles: consumption = |Thrust| * MassSpaceship / EngineEfficiency.
	// Higher value = more fuel-efficient engines (less consumption for the same thrust).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Fuel")
	float EngineEfficiency = 40000000000.0f;

	UFUNCTION(BlueprintPure, Category = "Flight|Fuel")
	float GetFuelPercentage() const;

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

	// Current translational velocity relative to the ship's own orientation: X=Forward, Y=Right, Z=Up, in m/s.
	UFUNCTION(BlueprintPure, Category = "Flight")
	FVector GetLocalVelocity() const;

	// Widget class shown as a HUD overlay (via CreateWidget + AddToViewport) for the local pilot.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cockpit Display")
	TSubclassOf<UCockpitDisplayWidget> CockpitDisplayWidgetClass;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;

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

	UFUNCTION()
	void HandleCheckpointOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void AddSpaceshipMappingContext();
	void UpdateEngineSound(bool bAnyThrustActive);
	void PlayCollisionSound();
	void UpdateCockpitDisplay();
	void ConsumeFuel(float ForwardForce, float VerticalForce, float LateralForce, float DeltaTime);
	void FindGravityPlanets();
	FVector ComputeGravityAcceleration() const;
	AActor* FindNearestGravityPlanet() const;

	float ThrustInput = 0.0f;
	float VerticalThrustInput = 0.0f;
	float LateralThrustInput = 0.0f;
	FVector Velocity = FVector::ZeroVector;

	// Actors tagged "Planet", found once at BeginPlay - not re-searched every Tick.
	TArray<TWeakObjectPtr<AActor>> GravityPlanets;

	// Velocity accumulated purely from planetary gravity. Kept separate from Velocity above so
	// VelocityDamping/ManeuverDamping (which only ever apply to the thrust-controlled Velocity)
	// never bleed off gravity-induced motion.
	FVector GravityVelocity = FVector::ZeroVector;

	// Last computed total gravitational acceleration, cached for the cockpit display.
	FVector TotalGravityAcceleration = FVector::ZeroVector;
	FVector2D SteeringInput = FVector2D::ZeroVector;
	float RollInput = 0.0f;
	FVector AngularVelocity = FVector::ZeroVector;
	FQuat HorizonRotation = FQuat::Identity;
	float RollAngle = 0.0f;
	bool bRotationInitialized = false;
	FTransform LastSafeTransform;
	bool bEngineSoundActive = false;
	float LastCollisionSoundTime = -1000.0f;

	UPROPERTY()
	TObjectPtr<UCockpitDisplayWidget> CockpitDisplayWidgetInstance;

};
