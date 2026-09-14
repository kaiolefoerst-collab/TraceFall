// Fill out your copyright notice in the Description page of Project Settings.

#include "SpaceRaceCheckpointComponent.h"

#include "Components/SceneComponent.h"

USpaceRaceCheckpointComponent::USpaceRaceCheckpointComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USpaceRaceCheckpointComponent::PostLoad()
{
	Super::PostLoad();
	PrimaryComponentTick.bCanEverTick = true;
}

void USpaceRaceCheckpointComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		if (USceneComponent* OwnerRoot = Owner->GetRootComponent())
		{
			OwnerRoot->SetMobility(EComponentMobility::Movable);
		}
	}
}

void USpaceRaceCheckpointComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (AActor* Owner = GetOwner())
	{
		Owner->AddActorLocalRotation(FRotator(0.0f, RotationSpeedDegreesPerSecond * DeltaTime, 0.0f));
	}
}
