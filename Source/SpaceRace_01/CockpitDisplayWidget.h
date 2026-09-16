// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CockpitDisplayWidget.generated.h"

class UOverlay;
class USizeBox;
class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;

// Builds and updates the cockpit system display entirely in C++.
// The corresponding Widget Blueprint (WBP_CockpitDisplay / BP_Dashboard) only needs to
// inherit from this class - it must not contain any Designer content or Event Graph logic.
//
// Shown via ASpaceshipPawn as a HUD overlay (CreateWidget + AddToViewport): a word-wrapped
// checkpoint/status message top-center, and a small multi-row info panel bottom-center.
UCLASS()
class SPACERACE_01_API UCockpitDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// LocalVelocity is (Forward, Right, Up) speed in m/s, relative to the ship's own orientation.
	UFUNCTION(BlueprintCallable, Category = "Cockpit Display")
	void UpdateVelocityDisplay(const FVector& LocalVelocity);

	// FuelPercent is in the range [0, 100].
	UFUNCTION(BlueprintCallable, Category = "Cockpit Display")
	void UpdateFuelDisplay(float FuelPercent);

	// Displays an arbitrary status message (e.g. a checkpoint's SpeechText). Word-wraps and
	// supports explicit "\n" line breaks. Display only - no text-to-speech, audio, or checkpoint
	// logic here.
	UFUNCTION(BlueprintCallable, Category = "Cockpit Display")
	void OutputMessage(const FText& Message);

	// The ship's currently computed total gravitational acceleration (already thresholded to
	// exactly FVector::ZeroVector by the caller when negligible). The widget does not compute
	// gravity itself - it only displays the value it is given.
	UFUNCTION(BlueprintCallable, Category = "Cockpit Display")
	void UpdateGravityDisplay(const FVector& GravityAcceleration);

	// bHasActiveCheckpoint false means no current target checkpoint (shows "---").
	UFUNCTION(BlueprintCallable, Category = "Cockpit Display")
	void UpdateCheckpointDistance(bool bHasActiveCheckpoint, float DistanceToCheckpoint);

	// bHasNearestPlanet false means no planet in the world (shows "---" for both values).
	UFUNCTION(BlueprintCallable, Category = "Cockpit Display")
	void UpdatePlanetDistances(bool bHasNearestPlanet, float PlanetCenterDistance, float PlanetSurfaceDistance);

	// Seconds elapsed since the race/exercise started, shown as "Time: MM:SS.ss".
	UFUNCTION(BlueprintCallable, Category = "Cockpit Display")
	void UpdateElapsedTime(float ElapsedSeconds);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();
	UHorizontalBox* CreateRow();
	UTextBlock* CreateMetricText(UHorizontalBox* Row);
	static FString FormatLine(const TCHAR* Label, float SpeedMetersPerSecond);

	// Full-screen, invisible root that positions the message text and the info panel.
	UPROPERTY()
	TObjectPtr<UOverlay> RootOverlay;

	// Top-center checkpoint/status message, word-wrapped.
	UPROPERTY()
	TObjectPtr<UTextBlock> MessageText;

	// Bottom-center info panel.
	UPROPERTY()
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ForwardValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> RightValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> UpValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> FuelValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> GravityStatusText;

	UPROPERTY()
	TObjectPtr<UTextBlock> GravityValueText;

	UPROPERTY()
	TObjectPtr<UTextBlock> GravityVectorText;

	UPROPERTY()
	TObjectPtr<UTextBlock> CheckpointDistanceText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PlanetCenterDistanceText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PlanetSurfaceDistanceText;

	UPROPERTY()
	TObjectPtr<UTextBlock> TimeText;
};
