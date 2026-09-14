// Fill out your copyright notice in the Description page of Project Settings.

#include "SpaceRaceGameInstance.h"

#include "SpaceRaceMainMenuWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

void USpaceRaceGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USpaceRaceGameInstance::HandleWorldLoaded);
}

void USpaceRaceGameInstance::OnStart()
{
	Super::OnStart();

	// The very first world (PIE or standalone) is not necessarily reached via a LoadMap call
	// (PIE duplicates the already-open editor world instead), so PostLoadMapWithWorld may never
	// fire for it. OnStart() reliably fires once the initial world is ready, covering that case.
	ShowMainMenuIfCurrentWorldIsMainMenu();
}

TArray<FSpaceRaceLevelInfo> USpaceRaceGameInstance::GetAvailableRaceLevels() const
{
	TArray<FSpaceRaceLevelInfo> Levels;

	IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();

	TArray<FAssetData> AssetsInDirectory;
	AssetRegistry.GetAssetsByPath(FName(*RaceLevelsDirectory), AssetsInDirectory, /*bRecursive=*/false);

	for (const FAssetData& Asset : AssetsInDirectory)
	{
		if (Asset.AssetClassPath != UWorld::StaticClass()->GetClassPathName())
		{
			continue;
		}

		const FString AssetName = Asset.AssetName.ToString();
		if (!AssetName.StartsWith(RaceLevelPrefix))
		{
			continue;
		}

		FSpaceRaceLevelInfo LevelInfo;
		LevelInfo.PackagePath = Asset.PackageName.ToString();
		LevelInfo.DisplayName = AssetName.RightChop(RaceLevelPrefix.Len());
		Levels.Add(LevelInfo);
	}

	return Levels;
}

void USpaceRaceGameInstance::StartRace(const FSpaceRaceLevelInfo& RaceLevel)
{
	if (!RaceLevel.PackagePath.IsEmpty())
	{
		UGameplayStatics::OpenLevel(this, FName(*RaceLevel.PackagePath));
	}
}

void USpaceRaceGameInstance::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(*MainMenuLevelPath));
}

void USpaceRaceGameInstance::HandleWorldLoaded(UWorld* LoadedWorld)
{
	ShowMainMenuIfCurrentWorldIsMainMenu();
}

void USpaceRaceGameInstance::ShowMainMenuIfCurrentWorldIsMainMenu()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// PIE renames the loaded package (e.g. "/Game/UEDPIE_0_MainMenu"), so strip that prefix
	// before comparing against the configured MainMenuLevelPath.
	const FString CurrentPackagePath = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	if (CurrentPackagePath == MainMenuLevelPath)
	{
		ShowMainMenu();
	}
}

void USpaceRaceGameInstance::ShowMainMenu()
{
	if (!MainMenuWidgetClass)
	{
		return;
	}

	if (MainMenuWidgetInstance)
	{
		MainMenuWidgetInstance->RemoveFromParent();
		MainMenuWidgetInstance = nullptr;
	}

	MainMenuWidgetInstance = CreateWidget<USpaceRaceMainMenuWidget>(this, MainMenuWidgetClass);
	if (MainMenuWidgetInstance)
	{
		MainMenuWidgetInstance->AddToViewport();
	}
}
