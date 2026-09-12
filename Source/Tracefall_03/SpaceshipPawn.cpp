// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceshipPawn.h"

#include "EnhancedInputComponent.h"
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

