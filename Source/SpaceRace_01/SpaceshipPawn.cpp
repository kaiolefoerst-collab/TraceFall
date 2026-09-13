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

// Sets default values
ASpaceshipPawn::ASpaceshipPawn()
{
	SceneRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	SceneRoot->SetCollisionProfileName(TEXT("BlockAll"));
	SceneRoot->SetBoxExtent(FVector(32.0f, 32.0f, 32.0f));

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

 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASpaceshipPawn::BeginPlay()
{
	Super::BeginPlay();
	AddSpaceshipMappingContext();
	LastSafeTransform = GetActorTransform();

	if (EngineAudioComponent)
	{
		EngineAudioComponent->SetSound(EngineCueSound);
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
	const float ForwardSpeed = FVector::DotProduct(MainEngineVelocity, ForwardDirection);
	const float VerticalSpeed = FVector::DotProduct(ManeuverVelocity, UpDirection);
	const float LateralSpeed = FVector::DotProduct(LateralVelocity, RightDirection);
	float ForwardForce = 0.0f;
	float VerticalForce = 0.0f;
	float LateralForce = 0.0f;

	if (!FMath::IsNearlyZero(ThrustInput))
	{
		const bool bAtForwardLimit = ThrustInput > 0.0f && ForwardSpeed >= MaxSpeed;
		const bool bAtBackwardLimit = ThrustInput < 0.0f && ForwardSpeed <= -MaxSpeed;
		if (!bAtForwardLimit && !bAtBackwardLimit)
		{
			ForwardForce = ThrustInput > 0.0f ? ThrustInput * ThrustForward : ThrustInput * ThrustBackward;
		}
	}

	if (!FMath::IsNearlyZero(VerticalThrustInput))
	{
		const bool bAtUpwardLimit = VerticalThrustInput > 0.0f && VerticalSpeed >= MaxSpeedVertical;
		const bool bAtDownwardLimit = VerticalThrustInput < 0.0f && VerticalSpeed <= -MaxSpeedVertical;
		if (!bAtUpwardLimit && !bAtDownwardLimit)
		{
			VerticalForce = VerticalThrustInput * ThrustVertical;
		}
	}

	if (!FMath::IsNearlyZero(LateralThrustInput))
	{
		const bool bAtRightLimit = LateralThrustInput > 0.0f && LateralSpeed >= MaxSpeedLateral;
		const bool bAtLeftLimit = LateralThrustInput < 0.0f && LateralSpeed <= -MaxSpeedLateral;
		if (!bAtRightLimit && !bAtLeftLimit)
		{
			LateralForce = LateralThrustInput * ThrustLateral;
		}
	}

	if (MassSpaceship > 0.0f)
	{
		MainEngineVelocity += ForwardDirection * (ForwardForce / MassSpaceship) * DeltaTime;
		ManeuverVelocity += UpDirection * (VerticalForce / MassSpaceship) * DeltaTime;
		LateralVelocity += RightDirection * (LateralForce / MassSpaceship) * DeltaTime;
	}

	const bool bTranslationalThrustActive = !FMath::IsNearlyZero(ThrustInput) || !FMath::IsNearlyZero(LateralThrustInput);
	if (!bTranslationalThrustActive)
	{
		MainEngineVelocity *= FMath::Exp(-VelocityDamping * DeltaTime);
		ManeuverVelocity *= FMath::Exp(-ManeuverDamping * DeltaTime);
		LateralVelocity *= FMath::Exp(-ManeuverDamping * DeltaTime);
	}
	FHitResult MovementHitResult;
	AddActorWorldOffset((MainEngineVelocity + ManeuverVelocity + LateralVelocity) * DeltaTime * 100.0f, true, &MovementHitResult);

	if (MovementHitResult.bBlockingHit)
	{
		SetActorTransform(LastSafeTransform);

		MainEngineVelocity *= -TranslationBounceFactor;
		ManeuverVelocity *= -TranslationBounceFactor;
		LateralVelocity *= -TranslationBounceFactor;

		AngularVelocity = FVector::ZeroVector;

		PlayCollisionSound();
	}
	else
	{
		LastSafeTransform = GetActorTransform();
	}

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

	DisplayForwardSpeedDebug(ForwardSpeed);
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
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(105, 2.0f, FColor::Yellow, FString::Printf(TEXT("Lateral Input: %.3f"), LateralThrustInput));
	}
}

void ASpaceshipPawn::HandleSteering(const FInputActionValue& Value)
{
	SteeringInput = Value.Get<FVector2D>();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(104, 2.0f, FColor::Yellow, FString::Printf(TEXT("Steering X: %.3f  Y: %.3f"), SteeringInput.X, SteeringInput.Y));
	}
}

void ASpaceshipPawn::HandleRoll(const FInputActionValue& Value)
{
	RollInput = Value.Get<float>();
}

void ASpaceshipPawn::DisplayForwardSpeedDebug(float ForwardSpeedMS) const
{
	if (GEngine && IsPlayerControlled())
	{
		GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Green, FString::Printf(TEXT("Speed: %.1f m/s"), ForwardSpeedMS), true, FVector2D(1.5f, 1.5f));
	}
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

