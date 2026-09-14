// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpaceRaceCheckpointComponent.generated.h"

// Per-instance checkpoint data. Attach via Add Component to an existing checkpoint Actor
// (e.g. a StaticMeshActor named Checkpoint00, Checkpoint01, ...) placed by hand in a race
// level. Provides checkpoint data plus a purely cosmetic spin - no overlap, ordering, or race
// logic here (that lives in ASpaceRaceGameMode).
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPACERACE_01_API USpaceRaceCheckpointComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpaceRaceCheckpointComponent();

	// Line spoken/shown when the player reaches this checkpoint. Set individually per instance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	FText SpeechText;

	// Purely cosmetic spin around the owning Actor's own vertical axis. 180 deg/s = 1 revolution every 2 seconds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	float RotationSpeedDegreesPerSecond = 180.0f;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Forces tick back on for component instances saved before rotation was added to this
	// class (their serialized PrimaryComponentTick.bCanEverTick=false would otherwise stick).
	virtual void PostLoad() override;

	// Checkpoint Actors are placed as StaticMeshActors, which default to Mobility=Static; that
	// silently ignores any runtime rotation. Force Movable so the spin actually takes effect.
	virtual void BeginPlay() override;
};
