// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SpaceRaceGameInstance.generated.h"

class USpaceRaceMainMenuWidget;

// Describes one available race level: which map it actually is, and what to show for it.
USTRUCT(BlueprintType)
struct FSpaceRaceLevelInfo
{
	GENERATED_BODY()

	// Full content package path of the level, e.g. "/Game/LVL-IndoorTraing01".
	UPROPERTY(BlueprintReadOnly, Category = "SpaceRace")
	FString PackagePath;

	// Menu display name: the map's asset name with the "LVL-" prefix removed.
	UPROPERTY(BlueprintReadOnly, Category = "SpaceRace")
	FString DisplayName;
};

// Top-level controller for the whole SpaceRace game. Persists across level loads.
// Owns: showing the main menu, finding available race levels, starting a race, and
// returning to the main menu. Later: results, stars, progress and unlocks.
UCLASS()
class SPACERACE_01_API USpaceRaceGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// Content directory that is scanned for race level maps.
	UPROPERTY(EditDefaultsOnly, Category = "SpaceRace")
	FString RaceLevelsDirectory = TEXT("/Game");

	// Filename prefix that identifies a map as a race level.
	UPROPERTY(EditDefaultsOnly, Category = "SpaceRace")
	FString RaceLevelPrefix = TEXT("LVL-");

	// Package path of the main menu map.
	UPROPERTY(EditDefaultsOnly, Category = "SpaceRace")
	FString MainMenuLevelPath = TEXT("/Game/MainMenu");

	// Widget class shown as the main menu whenever MainMenuLevelPath finishes loading.
	UPROPERTY(EditDefaultsOnly, Category = "SpaceRace")
	TSubclassOf<USpaceRaceMainMenuWidget> MainMenuWidgetClass;

	// Finds all maps directly in RaceLevelsDirectory whose asset name starts with RaceLevelPrefix.
	UFUNCTION(BlueprintPure, Category = "SpaceRace")
	TArray<FSpaceRaceLevelInfo> GetAvailableRaceLevels() const;

	// Loads the given race level.
	UFUNCTION(BlueprintCallable, Category = "SpaceRace")
	void StartRace(const FSpaceRaceLevelInfo& RaceLevel);

	// Loads the main menu map.
	UFUNCTION(BlueprintCallable, Category = "SpaceRace")
	void ReturnToMainMenu();

protected:
	virtual void Init() override;
	virtual void OnStart() override;

private:
	void HandleWorldLoaded(UWorld* LoadedWorld);
	void ShowMainMenuIfCurrentWorldIsMainMenu();
	void ShowMainMenu();

	UPROPERTY()
	TObjectPtr<USpaceRaceMainMenuWidget> MainMenuWidgetInstance;
};
