// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpaceRaceGameInstance.h"
#include "SpaceRaceMainMenuWidget.generated.h"

class UOverlay;
class UVerticalBox;
class UTextBlock;
class UButton;
class USpaceRaceMainMenuWidget;

// Internal helper: routes one dynamically created race button's click back to the owning menu
// together with which level it represents. UButton::OnClicked is a dynamic multicast delegate
// with no parameters, so per-button context needs a small object like this to carry it.
UCLASS()
class URaceLevelButtonHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FSpaceRaceLevelInfo LevelInfo;

	UPROPERTY()
	TWeakObjectPtr<USpaceRaceMainMenuWidget> OwnerWidget;

	UFUNCTION()
	void HandleClicked();
};

// Main menu: title, welcome text, one button per available race level (found via
// USpaceRaceGameInstance), and a Quit button. Entire layout and logic is built in C++;
// the corresponding Widget Blueprint only needs to inherit from this class.
UCLASS()
class SPACERACE_01_API USpaceRaceMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void StartRace(const FSpaceRaceLevelInfo& LevelInfo);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();
	void AddRaceButton(const FSpaceRaceLevelInfo& LevelInfo);

	UFUNCTION()
	void HandleQuitClicked();

	UPROPERTY()
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY()
	TArray<TObjectPtr<URaceLevelButtonHandler>> RaceButtonHandlers;
};
