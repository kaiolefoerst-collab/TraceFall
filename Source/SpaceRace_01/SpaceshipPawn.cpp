// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceshipPawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedActionKeyMapping.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Kismet/GameplayStatics.h"
#include "CockpitDisplayWidget.h"
#include "Blueprint/UserWidget.h"
#include "SpaceRaceCheckpointComponent.h"
#include "SpaceRaceGameMode.h"
#include "PhysicsEngine/BodySetup.h"

DEFINE_LOG_CATEGORY_STATIC(LogSpaceRaceGravity, Log, All);

// Sets default values
ASpaceshipPawn::ASpaceshipPawn()
{
	SceneRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	SceneRoot->SetCollisionProfileName(TEXT("BlockAll"));
	SceneRoot->SetBoxExtent(FVector(32.0f, 32.0f, 32.0f));
	SceneRoot->SetGenerateOverlapEvents(true);
	SceneRoot->OnComponentBeginOverlap.AddDynamic(this, &ASpaceshipPawn::HandleCheckpointOverlap);

	SpaceshipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipMesh"));
	SpaceshipMesh->SetupAttachment(SceneRoot);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SpaceshipMeshFinder(TEXT("/Game/MyGraphics/SM_SpaceShip01.SM_SpaceShip01"));
	if (SpaceshipMeshFinder.Succeeded())
	{
		SpaceshipMesh->SetStaticMesh(SpaceshipMeshFinder.Object);
		// SpaceshipMesh is rotated -90 deg yaw relative to SceneRoot, so swap X/Y extents to fit the collision box.
		const FVector MeshExtent = SpaceshipMeshFinder.Object->GetBounds().BoxExtent;
		SceneRoot->SetBoxExtent(FVector(MeshExtent.Y, MeshExtent.X, MeshExtent.Z));
	}
	SpaceshipMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	SpaceshipMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	SpaceshipMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	CockpitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CockpitCamera"));
	CockpitCamera->SetupAttachment(SceneRoot);
	CockpitCamera->SetRelativeLocation(FVector(1210.0f, 0.0f, 230.0f));
	CockpitCamera->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	CockpitCamera->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	CockpitCamera->FieldOfView = 90.0f;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextFinder(TEXT("/Game/Input/IMC_Spaceship.IMC_Spaceship"));
	if (MappingContextFinder.Succeeded())
	{
		SpaceshipMappingContext = MappingContextFinder.Object;
	}

	EngineAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudioComponent"));
	EngineAudioComponent->SetupAttachment(SceneRoot);
	EngineAudioComponent->bAutoActivate = false;

	static ConstructorHelpers::FObjectFinder<USoundBase> EngineCueFinder(TEXT("/Game/MySounds/Engine.Engine"));
	if (EngineCueFinder.Succeeded())
	{
		EngineCueSound = EngineCueFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> EngineStartSoundFinder(TEXT("/Game/MySounds/Whoosh.Whoosh"));
	if (EngineStartSoundFinder.Succeeded())
	{
		EngineStartSound = EngineStartSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> CollisionSoundFinder(TEXT("/Game/MySounds/CollisioninSpace.CollisioninSpace"));
	if (CollisionSoundFinder.Succeeded())
	{
		CollisionSound = CollisionSoundFinder.Object;
	}

	// Visual-only "flying through a particle field" effect. No asset loaded here - the concrete
	// Niagara System (NS_SpaceSpeedParticles) is assigned manually in BP_SpaceshipPawn.
	// bAutoActivate is off: UpdateSpaceSpeedEffect() switches it on only once the ship is
	// actually above SpaceSpeedEffectMinSpeed, so it starts deactivated.
	SpaceSpeedNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SpaceSpeedNiagaraComponent"));
	SpaceSpeedNiagaraComponent->SetupAttachment(SceneRoot);
	SpaceSpeedNiagaraComponent->bAutoActivate = false;

 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASpaceshipPawn::BeginPlay()
{
	Super::BeginPlay();
	AddSpaceshipMappingContext();
	LastSafeTransform = GetActorTransform();
	FindGravityPlanets();

	if (EngineAudioComponent)
	{
		EngineAudioComponent->SetSound(EngineCueSound);
	}
}

void ASpaceshipPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!CockpitDisplayWidgetInstance && CockpitDisplayWidgetClass && NewController && NewController->IsLocalController())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(NewController))
		{
			CockpitDisplayWidgetInstance = CreateWidget<UCockpitDisplayWidget>(PlayerController, CockpitDisplayWidgetClass);
			if (CockpitDisplayWidgetInstance)
			{
				CockpitDisplayWidgetInstance->AddToViewport();
			}
		}
	}
}

// Called every frame
void ASpaceshipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const bool bAnyThrustActive = !FMath::IsNearlyZero(ThrustInput) || !FMath::IsNearlyZero(VerticalThrustInput) || !FMath::IsNearlyZero(LateralThrustInput);
	UpdateEngineSound(bAnyThrustActive);

	const FVector ForwardDirection = GetActorForwardVector();
	const FVector UpDirection = GetActorUpVector();
	const FVector RightDirection = GetActorRightVector();

	// Single velocity vector for all thrusters. Decompose into local ship axes for this frame
	// so force/damping/limits always act relative to the ship's current orientation. With no
	// force/damping applied, decomposing and recomposing with the same basis is a no-op, so this
	// also stays correct (identity) for FlightAssistantMode::None further below.
	const FTransform ActorTransform = GetActorTransform();
	const FVector LocalVelocity = ActorTransform.InverseTransformVectorNoScale(Velocity);
	float ForwardSpeed = LocalVelocity.X;
	float LateralSpeed = LocalVelocity.Y;
	float VerticalSpeed = LocalVelocity.Z;
	float ForwardForce = 0.0f;
	float VerticalForce = 0.0f;
	float LateralForce = 0.0f;

	// None and PureForward both mean fully manual, physically direct translation: no speed
	// clamping and no damping may act on the linear Velocity in either of them. Kept as a single
	// flag (rather than removing the underlying systems) so later modes can re-enable them.
	const bool bApplyDampingAndSpeedLimits = (FlightAssistantMode != EFlightAssistantMode::None
		&& FlightAssistantMode != EFlightAssistantMode::PureForward);

	if (!FMath::IsNearlyZero(ThrustInput))
	{
		const bool bAtForwardLimit = bApplyDampingAndSpeedLimits && ThrustInput > 0.0f && ForwardSpeed >= MaxSpeed;
		const bool bAtBackwardLimit = bApplyDampingAndSpeedLimits && ThrustInput < 0.0f && ForwardSpeed <= -MaxSpeed;
		if (!bAtForwardLimit && !bAtBackwardLimit)
		{
			ForwardForce = ThrustInput > 0.0f ? ThrustInput * ThrustForward : ThrustInput * ThrustBackward;
		}
	}

	if (!FMath::IsNearlyZero(VerticalThrustInput))
	{
		const bool bAtUpwardLimit = bApplyDampingAndSpeedLimits && VerticalThrustInput > 0.0f && VerticalSpeed >= MaxSpeedVertical;
		const bool bAtDownwardLimit = bApplyDampingAndSpeedLimits && VerticalThrustInput < 0.0f && VerticalSpeed <= -MaxSpeedVertical;
		if (!bAtUpwardLimit && !bAtDownwardLimit)
		{
			VerticalForce = VerticalThrustInput * ThrustVertical;
		}
	}

	if (!FMath::IsNearlyZero(LateralThrustInput))
	{
		const bool bAtRightLimit = bApplyDampingAndSpeedLimits && LateralThrustInput > 0.0f && LateralSpeed >= MaxSpeedLateral;
		const bool bAtLeftLimit = bApplyDampingAndSpeedLimits && LateralThrustInput < 0.0f && LateralSpeed <= -MaxSpeedLateral;
		if (!bAtRightLimit && !bAtLeftLimit)
		{
			LateralForce = LateralThrustInput * ThrustLateral;
		}
	}

	// PureForward only engages while the player is actively giving forward/backward thrust
	// (W/S); without it, PureForward is translationally identical to None (verified below since
	// every Assist*Force stays exactly 0.0 and nothing else in this function checks the mode).
	const bool bPureForwardAssistEligible = (FlightAssistantMode == EFlightAssistantMode::PureForward)
		&& !FMath::IsNearlyZero(ThrustInput) && DeltaTime > 0.0f && MassSpaceship > 0.0f;

	// Actively counter-accelerate RightVelocity/UpVelocity toward (but never past) zero using the
	// same lateral/vertical thruster power available to the player, reacting to the ship's total
	// current velocity (thrust + gravity) so planetary drift is countered too. Each axis is only
	// corrected while the player isn't manually commanding that same axis (A/D, Space/Ctrl take
	// precedence and fully disable the assistant on their respective axis).
	// Kept as separate Assist*Force variables (not merged into LateralForce/VerticalForce above,
	// which represent player-commanded thrust) so a future fuel-consumption pass can attribute
	// the assistant's own thruster usage distinctly from the player's.
	float AssistLateralForce = 0.0f;
	float AssistVerticalForce = 0.0f;
	if (bPureForwardAssistEligible)
	{
		const FVector LocalTotalVelocity = ActorTransform.InverseTransformVectorNoScale(Velocity + GravityVelocity);

		if (FMath::IsNearlyZero(LateralThrustInput))
		{
			const float MaxLateralAssistAccel = ThrustLateral / MassSpaceship;
			const float NeededRightAccel = FMath::Clamp(-LocalTotalVelocity.Y / DeltaTime, -MaxLateralAssistAccel, MaxLateralAssistAccel);
			AssistLateralForce = NeededRightAccel * MassSpaceship;
		}

		if (FMath::IsNearlyZero(VerticalThrustInput))
		{
			const float MaxVerticalAssistAccel = ThrustVertical / MassSpaceship;
			const float NeededUpAccel = FMath::Clamp(-LocalTotalVelocity.Z / DeltaTime, -MaxVerticalAssistAccel, MaxVerticalAssistAccel);
			AssistVerticalForce = NeededUpAccel * MassSpaceship;
		}
	}

	// Fuel accounting currently only covers player-commanded thrust; the assistant's counter-
	// thrust force is deliberately excluded here until it is wired into fuel consumption.
	ConsumeFuel(ForwardForce, VerticalForce, LateralForce, DeltaTime);

	if (MassSpaceship > 0.0f)
	{
		ForwardSpeed += (ForwardForce / MassSpaceship) * DeltaTime;
		VerticalSpeed += ((VerticalForce + AssistVerticalForce) / MassSpaceship) * DeltaTime;
		LateralSpeed += ((LateralForce + AssistLateralForce) / MassSpaceship) * DeltaTime;
	}

	const bool bTranslationalThrustActive = !FMath::IsNearlyZero(ThrustInput) || !FMath::IsNearlyZero(LateralThrustInput);
	if (bApplyDampingAndSpeedLimits && !bTranslationalThrustActive)
	{
		ForwardSpeed *= FMath::Exp(-VelocityDamping * DeltaTime);
		LateralSpeed *= FMath::Exp(-ManeuverDamping * DeltaTime);
	}
	if (bApplyDampingAndSpeedLimits && FMath::IsNearlyZero(VerticalThrustInput))
	{
		VerticalSpeed *= FMath::Exp(-ManeuverDamping * DeltaTime);
	}

	Velocity = ForwardDirection * ForwardSpeed + RightDirection * LateralSpeed + UpDirection * VerticalSpeed;

	// Planetary gravity is a persistent external acceleration. It is accumulated into its own
	// GravityVelocity instead of being folded into Velocity above, so it never gets decomposed
	// into local Forward/Lateral/Vertical speed and incorrectly removed by VelocityDamping /
	// ManeuverDamping whenever the corresponding thrust axis happens to be inactive.
	TotalGravityAcceleration = ComputeGravityAcceleration();
	GravityVelocity += TotalGravityAcceleration * DeltaTime;

	FHitResult MovementHitResult;
	AddActorWorldOffset((Velocity + GravityVelocity) * DeltaTime * 100.0f, true, &MovementHitResult);

	if (MovementHitResult.bBlockingHit)
	{
		SetActorTransform(LastSafeTransform);

		Velocity *= -TranslationBounceFactor;
		GravityVelocity *= -TranslationBounceFactor;

		AngularVelocity = FVector::ZeroVector;

		PlayCollisionSound();
	}
	else
	{
		LastSafeTransform = GetActorTransform();
	}

	// Visual-only: reads the same World-Velocity already used for movement above, does not
	// affect flight physics in any way.
	UpdateSpaceSpeedEffect(Velocity + GravityVelocity);

	const FVector AngularAcceleration(
		RollInput * RollTorque,
		SteeringInput.Y * PitchTorque,
		SteeringInput.X * YawTorque);
	AngularVelocity += AngularAcceleration * DeltaTime;
	const float RotationDampingFactor = FMath::Exp(-RotationDamping * DeltaTime);
	if (FMath::IsNearlyZero(RollInput))
	{
		AngularVelocity.X *= RotationDampingFactor;
	}
	AngularVelocity.Y *= RotationDampingFactor;
	AngularVelocity.Z *= RotationDampingFactor;
	AngularVelocity.X = FMath::Clamp(AngularVelocity.X, -MaxRollRate, MaxRollRate);
	AngularVelocity.Y = FMath::Clamp(AngularVelocity.Y, -MaxPitchRate, MaxPitchRate);
	AngularVelocity.Z = FMath::Clamp(AngularVelocity.Z, -MaxYawRate, MaxYawRate);

	const float RadiansPerDegree = UE_PI / 180.0f;
	if (!bRotationInitialized)
	{
		const FVector InitialForward = GetActorForwardVector().GetSafeNormal();
		const FVector InitialRight = FVector::CrossProduct(FVector::UpVector, InitialForward).GetSafeNormal(UE_SMALL_NUMBER, GetActorRightVector());
		const FVector InitialUp = FVector::CrossProduct(InitialForward, InitialRight).GetSafeNormal();
		HorizonRotation = FRotationMatrix::MakeFromXZ(InitialForward, InitialUp).ToQuat();
		RollAngle = FMath::Atan2(
			FVector::DotProduct(GetActorUpVector(), InitialRight),
			FVector::DotProduct(GetActorUpVector(), InitialUp));
		bRotationInitialized = true;
	}

	RollAngle += AngularVelocity.X * RadiansPerDegree * DeltaTime;
	const FQuat LocalRollRotation(FVector::ForwardVector, RollAngle);
	const FQuat CurrentRolledRotation = HorizonRotation * LocalRollRotation;
	const FVector YawAxis = CurrentRolledRotation.RotateVector(FVector::UpVector);
	const FQuat YawRotation(YawAxis, AngularVelocity.Z * RadiansPerDegree * DeltaTime);
	const FQuat YawAppliedRotation = YawRotation * CurrentRolledRotation;
	const FVector PitchAxis = YawAppliedRotation.RotateVector(FVector::RightVector);
	const FQuat PitchRotation(PitchAxis, AngularVelocity.Y * RadiansPerDegree * DeltaTime);
	const FQuat RotatedOrientation = PitchRotation * YawAppliedRotation;
	const FVector NewForward = RotatedOrientation.RotateVector(FVector::ForwardVector).GetSafeNormal();
	const FVector PreviousRight = HorizonRotation.RotateVector(FVector::RightVector);
	const FVector NewRight = FVector::CrossProduct(FVector::UpVector, NewForward).GetSafeNormal(UE_SMALL_NUMBER, PreviousRight);
	const FVector NewUp = FVector::CrossProduct(NewForward, NewRight).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	HorizonRotation = FRotationMatrix::MakeFromXZ(NewForward, NewUp).ToQuat();
	HorizonRotation.Normalize();
	FQuat NewRotation = HorizonRotation * LocalRollRotation;
	NewRotation.Normalize();
	SetActorRotation(NewRotation);

	UpdateCockpitDisplay();
}

// Called to bind functionality to input
void ASpaceshipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		const UInputAction* ThrustAction = nullptr;
		const UInputAction* VerticalThrustAction = nullptr;
		const UInputAction* LateralThrustAction = nullptr;
		if (SpaceshipMappingContext)
		{
			for (const FEnhancedActionKeyMapping& Mapping : SpaceshipMappingContext->GetMappings())
			{
				if (!Mapping.Action)
				{
					continue;
				}

				if (Mapping.Action->GetName() == TEXT("IA_Spaceship_Thrust"))
				{
					ThrustAction = Mapping.Action.Get();
				}
				else if (Mapping.Action->GetName() == TEXT("IA_Spaceship_VerticalThrust"))
				{
					VerticalThrustAction = Mapping.Action.Get();
				}
				else if (Mapping.Action->GetName() == TEXT("IA_Spaceship_LateralThrust"))
				{
					LateralThrustAction = Mapping.Action.Get();
				}
			}
		}

		if (ThrustAction)
		{
			EnhancedInputComponent->BindAction(ThrustAction, ETriggerEvent::Triggered, this, &ASpaceshipPawn::HandleThrust);
			EnhancedInputComponent->BindAction(ThrustAction, ETriggerEvent::Completed, this, &ASpaceshipPawn::HandleThrust);
		}
		if (VerticalThrustAction)
		{
			EnhancedInputComponent->BindAction(VerticalThrustAction, ETriggerEvent::Triggered, this, &ASpaceshipPawn::HandleVerticalThrust);
			EnhancedInputComponent->BindAction(VerticalThrustAction, ETriggerEvent::Completed, this, &ASpaceshipPawn::HandleVerticalThrust);
		}
		if (LateralThrustAction)
		{
			EnhancedInputComponent->BindAction(LateralThrustAction, ETriggerEvent::Triggered, this, &ASpaceshipPawn::HandleLateralThrust);
			EnhancedInputComponent->BindAction(LateralThrustAction, ETriggerEvent::Completed, this, &ASpaceshipPawn::HandleLateralThrust);
		}

		const UInputAction* SteeringAction = nullptr;
		const UInputAction* RollAction = nullptr;
		if (SpaceshipMappingContext)
		{
			for (const FEnhancedActionKeyMapping& Mapping : SpaceshipMappingContext->GetMappings())
			{
				if (!Mapping.Action)
				{
					continue;
				}

				if (Mapping.Action->GetName() == TEXT("IA_Spaceship_Steering"))
				{
					SteeringAction = Mapping.Action.Get();
				}
				else if (Mapping.Action->GetName() == TEXT("IA_Spaceship_Roll"))
				{
					RollAction = Mapping.Action.Get();
				}
			}
		}

		if (SteeringAction)
		{
			EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ASpaceshipPawn::HandleSteering);
			EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ASpaceshipPawn::HandleSteering);
			EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Canceled, this, &ASpaceshipPawn::HandleSteering);
		}
		if (RollAction)
		{
			EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Triggered, this, &ASpaceshipPawn::HandleRoll);
			EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Completed, this, &ASpaceshipPawn::HandleRoll);
			EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Canceled, this, &ASpaceshipPawn::HandleRoll);
		}
	}
}

void ASpaceshipPawn::HandleThrust(const FInputActionValue& Value)
{
	ThrustInput = Value.Get<float>();
}

void ASpaceshipPawn::HandleVerticalThrust(const FInputActionValue& Value)
{
	VerticalThrustInput = Value.Get<float>();
}

void ASpaceshipPawn::HandleLateralThrust(const FInputActionValue& Value)
{
	LateralThrustInput = Value.Get<float>();
}

void ASpaceshipPawn::HandleSteering(const FInputActionValue& Value)
{
	SteeringInput = Value.Get<FVector2D>();
}

void ASpaceshipPawn::HandleRoll(const FInputActionValue& Value)
{
	RollInput = Value.Get<float>();
}

void ASpaceshipPawn::HandleCheckpointOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	const USpaceRaceCheckpointComponent* CheckpointComponent = OtherActor->FindComponentByClass<USpaceRaceCheckpointComponent>();
	if (!CheckpointComponent)
	{
		return;
	}

	if (CockpitDisplayWidgetInstance)
	{
		CockpitDisplayWidgetInstance->OutputMessage(CheckpointComponent->SpeechText);
	}

	if (ASpaceRaceGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpaceRaceGameMode>())
	{
		GameMode->CheckpointReached();
	}
}

FVector ASpaceshipPawn::GetLocalVelocity() const
{
	// The true world velocity is Velocity + GravityVelocity (see Tick()'s AddActorWorldOffset
	// call) - both must be included so the cockpit reflects gravity drift and, while
	// FlightAssistantMode::PureForward is correcting it, the resulting approach toward 0.
	return GetActorTransform().InverseTransformVectorNoScale(Velocity + GravityVelocity);
}

float ASpaceshipPawn::GetFuelPercentage() const
{
	return FuelTankCapacity > 0.0f ? FMath::Clamp(CurrentFuel / FuelTankCapacity, 0.0f, 1.0f) * 100.0f : 0.0f;
}

void ASpaceshipPawn::ConsumeFuel(float ForwardForce, float VerticalForce, float LateralForce, float DeltaTime)
{
	if (CurrentFuel <= 0.0f || EngineEfficiency <= 0.0f)
	{
		CurrentFuel = FMath::Max(CurrentFuel, 0.0f);
		return;
	}

	// All nozzles share the same efficiency factor. Consumption is proportional to the thrust
	// each active nozzle is actually producing this frame (zero once a nozzle is at its speed limit).
	const float TotalThrustMagnitude = FMath::Abs(ForwardForce) + FMath::Abs(VerticalForce) + FMath::Abs(LateralForce);
	const float FuelConsumptionRate = TotalThrustMagnitude * MassSpaceship / EngineEfficiency;
	CurrentFuel = FMath::Max(0.0f, CurrentFuel - FuelConsumptionRate * DeltaTime);
}

void ASpaceshipPawn::UpdateCockpitDisplay()
{
	if (!CockpitDisplayWidgetInstance)
	{
		return;
	}

	CockpitDisplayWidgetInstance->UpdateVelocityDisplay(GetLocalVelocity());
	CockpitDisplayWidgetInstance->UpdateFuelDisplay(GetFuelPercentage());
	CockpitDisplayWidgetInstance->UpdateGravityDisplay(TotalGravityAcceleration);

	// Reuses the existing GameMode checkpoint management - no second checkpoint tracking here.
	ASpaceRaceGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASpaceRaceGameMode>() : nullptr;

	if (AActor* ActiveCheckpoint = GameMode ? GameMode->GetActiveCheckpoint() : nullptr)
	{
		const float CheckpointDistance = FVector::Dist(GetActorLocation(), ActiveCheckpoint->GetActorLocation());
		CockpitDisplayWidgetInstance->UpdateCheckpointDistance(true, CheckpointDistance);
	}
	else
	{
		CockpitDisplayWidgetInstance->UpdateCheckpointDistance(false, 0.0f);
	}

	// Reuses the existing cached GravityPlanets list - no new per-frame Actor search.
	if (AActor* NearestPlanet = FindNearestGravityPlanet())
	{
		const float CenterDistance = FVector::Dist(GetActorLocation(), NearestPlanet->GetActorLocation());
		float SurfaceDistance = CenterDistance;
		if (const UStaticMeshComponent* PlanetMesh = NearestPlanet->FindComponentByClass<UStaticMeshComponent>())
		{
			// World-space bounds already account for the planet's World Scale.
			SurfaceDistance = CenterDistance - PlanetMesh->Bounds.SphereRadius;
		}
		CockpitDisplayWidgetInstance->UpdatePlanetDistances(true, CenterDistance, SurfaceDistance);
	}
	else
	{
		CockpitDisplayWidgetInstance->UpdatePlanetDistances(false, 0.0f, 0.0f);
	}

	CockpitDisplayWidgetInstance->UpdateElapsedTime(GameMode ? GameMode->GetElapsedRaceTime() : 0.0f);
}

AActor* ASpaceshipPawn::FindNearestGravityPlanet() const
{
	AActor* NearestPlanet = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	const FVector ShipLocation = GetActorLocation();

	for (const TWeakObjectPtr<AActor>& PlanetPtr : GravityPlanets)
	{
		AActor* Planet = PlanetPtr.Get();
		if (!Planet)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(ShipLocation, Planet->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestPlanet = Planet;
		}
	}

	return NearestPlanet;
}

void ASpaceshipPawn::UpdateSpaceSpeedEffect(const FVector& WorldVelocity)
{
	if (!SpaceSpeedNiagaraComponent)
	{
		return;
	}

	const bool bShouldBeActive = WorldVelocity.Size() > SpaceSpeedEffectMinSpeed;

	// Only call Activate()/Deactivate() on an actual state change, never every Tick.
	if (bShouldBeActive && !bSpaceSpeedEffectActive)
	{
		SpaceSpeedNiagaraComponent->Activate();
	}
	else if (!bShouldBeActive && bSpaceSpeedEffectActive)
	{
		SpaceSpeedNiagaraComponent->Deactivate();
	}
	bSpaceSpeedEffectActive = bShouldBeActive;

	if (!bSpaceSpeedEffectActive)
	{
		return;
	}

	// The emitter now runs in Local Space (Local Space = ON) and simply moves/rotates with the
	// ship as a normal attached component - no manual world position/rotation overrides needed.
	// Niagara's Add Velocity module and Shape Location module both operate in the component's
	// local space, so WorldVelocity has to be converted into that local space here. No sign flip
	// is applied - the Niagara System itself applies the direction reversal via its own
	// "Velocity Speed Scale = -1.0" setting on the Add Velocity module.
	const FVector LocalVelocity = SpaceSpeedNiagaraComponent->GetComponentTransform().InverseTransformVectorNoScale(WorldVelocity);
	SpaceSpeedNiagaraComponent->SetVectorParameter(FName(TEXT("User.ShipVelocity")), LocalVelocity);
}

void ASpaceshipPawn::FindGravityPlanets()
{
	GravityPlanets.Reset();

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("Planet")), FoundActors);

	UE_LOG(LogSpaceRaceGravity, Log, TEXT("FindGravityPlanets: %d actor(s) tagged \"Planet\"."), FoundActors.Num());

	for (AActor* Actor : FoundActors)
	{
		if (!Actor)
		{
			continue;
		}

		GravityPlanets.Add(Actor);

		UStaticMeshComponent* PlanetMesh = Actor->FindComponentByClass<UStaticMeshComponent>();
		if (!PlanetMesh)
		{
			UE_LOG(LogSpaceRaceGravity, Warning, TEXT("  %s: no UStaticMeshComponent found - will not contribute gravity."), *Actor->GetName());
			continue;
		}

		UBodySetup* PlanetBodySetup = PlanetMesh->GetBodySetup();
		const float PlanetMass = PlanetBodySetup ? PlanetBodySetup->CalculateMass(PlanetMesh) : 0.0f;

		UE_LOG(LogSpaceRaceGravity, Log, TEXT("  %s: StaticMeshComponent=%s Mass=%.1f BodySetup=%s Location=%s"),
			*Actor->GetName(),
			*PlanetMesh->GetName(),
			PlanetMass,
			PlanetBodySetup ? TEXT("valid") : TEXT("NULL"),
			*Actor->GetActorLocation().ToString());
	}
}

FVector ASpaceshipPawn::ComputeGravityAcceleration() const
{
	FVector Total = FVector::ZeroVector;
	const FVector ShipLocation = GetActorLocation();

	// Safety floor against division by (near) zero / runaway forces at extreme close range.
	constexpr float MinDistanceSquared = 100.0f * 100.0f;

	for (const TWeakObjectPtr<AActor>& PlanetPtr : GravityPlanets)
	{
		AActor* Planet = PlanetPtr.Get();
		if (!Planet)
		{
			continue;
		}

		UStaticMeshComponent* PlanetMesh = Planet->FindComponentByClass<UStaticMeshComponent>();
		if (!PlanetMesh)
		{
			continue;
		}

		// GetMass() only works while physics is actually simulating (it reads the live physics
		// body). Planets are typically static, so read the configured/calculated mass instead -
		// this honors a Mass override and otherwise computes it from density * volume, without
		// requiring Simulate Physics to be enabled.
		UBodySetup* PlanetBodySetup = PlanetMesh->GetBodySetup();
		const float PlanetMass = PlanetBodySetup ? PlanetBodySetup->CalculateMass(PlanetMesh) : 0.0f;
		if (PlanetMass <= 0.0f)
		{
			continue;
		}

		const FVector ToPlanet = Planet->GetActorLocation() - ShipLocation;
		const float DistanceSquared = FMath::Max(ToPlanet.SizeSquared(), MinDistanceSquared);
		const FVector DirectionToPlanet = ToPlanet.GetSafeNormal();

		Total += DirectionToPlanet * (GravityConstant * PlanetMass / DistanceSquared);
	}

	// Treat negligible combined gravity (from all planets together) as exactly none, rather than
	// showing/applying a near-zero residual.
	if (Total.Size() < 1.0f)
	{
		Total = FVector::ZeroVector;
	}

	return Total;
}

void ASpaceshipPawn::AddSpaceshipMappingContext()
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (SpaceshipMappingContext && !InputSubsystem->HasMappingContext(SpaceshipMappingContext))
				{
					InputSubsystem->AddMappingContext(SpaceshipMappingContext, 0);
				}
			}
		}
	}
}

void ASpaceshipPawn::UpdateEngineSound(bool bAnyThrustActive)
{
	if (bAnyThrustActive && !bEngineSoundActive)
	{
		if (EngineAudioComponent && EngineCueSound)
		{
			float LoopDuration = EngineCueSound->GetDuration();
			if (LoopDuration <= 0.0f || LoopDuration >= INDEFINITELY_LOOPING_DURATION)
			{
				// GetDuration() returns the sentinel value for sounds set to loop (e.g. a looping SoundWave),
				// so fall back to the wave's raw clip length to get a usable range for the random start time.
				if (const USoundWave* SoundWave = Cast<USoundWave>(EngineCueSound))
				{
					LoopDuration = SoundWave->Duration;
				}
				else
				{
					LoopDuration = 0.0f;
				}
			}

			const float RandomStartTime = LoopDuration > 0.0f ? FMath::FRandRange(0.0f, LoopDuration) : 0.0f;
			EngineAudioComponent->Play(RandomStartTime);
		}

		if (EngineStartSound)
		{
			UGameplayStatics::PlaySound2D(this, EngineStartSound, EngineStartVolumeMultiplier);
		}
	}
	else if (!bAnyThrustActive && bEngineSoundActive)
	{
		if (EngineAudioComponent)
		{
			EngineAudioComponent->Stop();
		}
	}

	bEngineSoundActive = bAnyThrustActive;
}

void ASpaceshipPawn::PlayCollisionSound()
{
	if (!CollisionSound)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastCollisionSoundTime >= CollisionSoundCooldown)
	{
		UGameplayStatics::PlaySound2D(this, CollisionSound, CollisionVolumeMultiplier);
		LastCollisionSoundTime = CurrentTime;
	}
}

