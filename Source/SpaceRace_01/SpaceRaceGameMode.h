// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SpaceRaceGameMode.generated.h"

// Central race-logic controller for a single race level (a hand-built LVL-... map).
// It governs race flow only - it does not spawn, generate or assemble any level content
// (planets, checkpoints, space stations, track layout). Those actors already exist in the map.
//
// Finds all Actors carrying a USpaceRaceCheckpointComponent at BeginPlay, sorts them by the
// trailing number in their name (CheckpointXX), and steps through them one at a time as the
// ship reports reaching the currently active one.
UCLASS()
class SPACERACE_01_API ASpaceRaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// Called by the ship when it overlaps the currently active checkpoint.
	void CheckpointReached();

protected:
	virtual void BeginPlay() override;

private:
	void InitializeCheckpoints();
	void ActivateCheckpoint(int32 Index);
	static void SetCheckpointActive(AActor* CheckpointActor, bool bActive);
	void EndRace();

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SortedCheckpoints;

	int32 CurrentCheckpointIndex = INDEX_NONE;

	// Delay between reaching the final checkpoint and ending the race.
	UPROPERTY(EditDefaultsOnly, Category = "Race")
	float FinishDelaySeconds = 10.0f;

	FTimerHandle FinishTimerHandle;
};
