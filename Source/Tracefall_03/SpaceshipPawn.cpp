// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceshipPawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedActionKeyMapping.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ASpaceshipPawn::ASpaceshipPawn()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	SpaceshipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipMesh"));
	SpaceshipMesh->SetupAttachment(SceneRoot);
	CockpitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CockpitCamera"));
	CockpitCamera->SetupAttachment(SceneRoot);
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextFinder(TEXT("/Game/Input/IMC_Spaceship.IMC_Spaceship"));
	if (MappingContextFinder.Succeeded())
	{
		SpaceshipMappingContext = MappingContextFinder.Object;
	}

 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASpaceshipPawn::BeginPlay()
{
	Super::BeginPlay();
	AddSpaceshipMappingContext();
}

// Called every frame
void ASpaceshipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector ForwardDirection = GetActorForwardVector();
	const FVector UpDirection = GetActorUpVector();
	const float ForwardSpeed = FVector::DotProduct(MainEngineVelocity, ForwardDirection);
	const float VerticalSpeed = FVector::DotProduct(ManeuverVelocity, UpDirection);
	float ForwardForce = 0.0f;
	float VerticalForce = 0.0f;

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

	if (MassSpaceship > 0.0f)
	{
		MainEngineVelocity += ForwardDirection * (ForwardForce / MassSpaceship) * DeltaTime;
		ManeuverVelocity += UpDirection * (VerticalForce / MassSpaceship) * DeltaTime;
	}

	MainEngineVelocity *= FMath::Exp(-VelocityDamping * DeltaTime);
	ManeuverVelocity *= FMath::Exp(-ManeuverDamping * DeltaTime);
	AddActorWorldOffset((MainEngineVelocity + ManeuverVelocity) * DeltaTime * 100.0f);

	const FVector AngularAcceleration(
		RollInput * RollTorque,
		SteeringInput.Y * PitchTorque,
		SteeringInput.X * YawTorque);
	AngularVelocity += AngularAcceleration * DeltaTime;
	AngularVelocity *= FMath::Exp(-RotationDamping * DeltaTime);
	AngularVelocity.X = FMath::Clamp(AngularVelocity.X, -MaxRollRate, MaxRollRate);
	AngularVelocity.Y = FMath::Clamp(AngularVelocity.Y, -MaxPitchRate, MaxPitchRate);
	AngularVelocity.Z = FMath::Clamp(AngularVelocity.Z, -MaxYawRate, MaxYawRate);

	const float RadiansPerDegree = UE_PI / 180.0f;
	const FQuat RollRotation(FVector::ForwardVector, AngularVelocity.X * RadiansPerDegree * DeltaTime);
	const FQuat PitchRotation(FVector::RightVector, AngularVelocity.Y * RadiansPerDegree * DeltaTime);
	const FQuat YawRotation(FVector::UpVector, AngularVelocity.Z * RadiansPerDegree * DeltaTime);
	AddActorLocalRotation(RollRotation * PitchRotation * YawRotation);
}

// Called to bind functionality to input
void ASpaceshipPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		const UInputAction* ThrustAction = nullptr;
		const UInputAction* VerticalThrustAction = nullptr;
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

