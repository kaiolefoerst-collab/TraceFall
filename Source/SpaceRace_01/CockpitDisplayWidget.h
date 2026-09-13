// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CockpitDisplayWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UTextBlock;

// Builds and updates the cockpit system display entirely in C++.
// The corresponding Widget Blueprint (WBP_CockpitDisplay) only needs to inherit from
// this class - it must not contain any Designer content or Event Graph logic.
//
// Laid out as a single flat, wide bar (not a tall panel) so it fits as a bottom-of-screen
// HUD strip when the owning Widget Component is set to Screen space.
UCLASS()
class SPACERACE_01_API UCockpitDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// LocalVelocity is (Forward, Right, Up) speed in m/s, relative to the ship's own orientation.
	UFUNCTION(BlueprintCallable, Category = "Cockpit Display")
	void UpdateVelocityDisplay(const FVector& LocalVelocity);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();
	UTextBlock* CreateMetricText();
	static FString FormatLine(const TCHAR* Label, float SpeedMetersPerSecond);

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
};
