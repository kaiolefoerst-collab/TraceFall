// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CockpitDisplayWidget.generated.h"

class UOverlay;
class UBorder;
class UHorizontalBox;
class UTextBlock;

// Builds and updates the cockpit system display entirely in C++.
// The corresponding Widget Blueprint (WBP_CockpitDisplay / BP_Dashboard) only needs to
// inherit from this class - it must not contain any Designer content or Event Graph logic.
//
// Shown via ASpaceshipPawn as a HUD overlay (CreateWidget + AddToViewport). A single flat,
// wide bar anchored to the bottom-center of the screen, not a tall panel.
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

	// Displays an arbitrary status message (e.g. a checkpoint's SpeechText). Display only -
	// no text-to-speech, audio, or checkpoint logic here.
	UFUNCTION(BlueprintCallable, Category = "Cockpit Display")
	void OutputMessage(const FText& Message);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();
	UTextBlock* CreateMetricText();
	static FString FormatLine(const TCHAR* Label, float SpeedMetersPerSecond);

	// Full-screen, invisible root that positions the visible bar at the bottom-center.
	UPROPERTY()
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY()
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> ContentBox;

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

	// Top-center status line used by OutputMessage(), e.g. for checkpoint SpeechText.
	UPROPERTY()
	TObjectPtr<UTextBlock> MessageText;
};
