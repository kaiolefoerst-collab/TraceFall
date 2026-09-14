// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SpaceRaceGameMode.generated.h"

// Central race-logic controller for a single race level (a hand-built LVL-... map).
// It governs race flow only - it does not spawn, generate or assemble any level content
// (planets, checkpoints, space stations, track layout). Those actors already exist in the map.
//
// Planned responsibilities (not yet implemented): initialize race, countdown, start race,
// monitor checkpoints, measure race time, detect the goal, end race, determine result/stars,
// and report the result back to USpaceRaceGameInstance.
UCLASS()
class SPACERACE_01_API ASpaceRaceGameMode : public AGameModeBase
{
	GENERATED_BODY()
};
