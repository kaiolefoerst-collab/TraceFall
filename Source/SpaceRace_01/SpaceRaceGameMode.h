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

	// The checkpoint the ship should currently be heading to, or nullptr if none is active
	// (e.g. before initialization, or after the race has finished). Exposed read-only so other
	// systems (e.g. the cockpit display) can query it without keeping a second checkpoint list.
	AActor* GetActiveCheckpoint() const;

	// Seconds elapsed since the race/exercise actually started (BeginPlay), the single
	// authoritative time base for this race - not tied to any display widget's lifetime.
	float GetElapsedRaceTime() const;

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

	float RaceStartTimeSeconds = 0.0f;
};
