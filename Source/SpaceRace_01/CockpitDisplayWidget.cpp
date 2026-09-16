// Fill out your copyright notice in the Description page of Project Settings.

#include "CockpitDisplayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Brushes/SlateColorBrush.h"

void UCockpitDisplayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void UCockpitDisplayWidget::BuildLayout()
{
	if (!WidgetTree || RootBorder)
	{
		return;
	}

	// Invisible root filling the whole viewport, purely so it can anchor the message text and
	// the info panel independently instead of stretching a background over the full screen.
	RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	// --- Top-center checkpoint/status message: word-wrapped, fixed max width so it never runs
	// off-screen and never grows to cover the info panel below.
	USizeBox* MessageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MessageBox"));
	MessageBox->SetWidthOverride(900.0f);

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
	MessageText->SetText(FText::GetEmpty());
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.95f, 0.6f)));
	MessageText->SetFontSize(22);
	MessageText->SetAutoWrapText(true);
	MessageBox->SetContent(MessageText);

	if (UOverlaySlot* MessageSlot = RootOverlay->AddChildToOverlay(MessageBox))
	{
		MessageSlot->SetHorizontalAlignment(HAlign_Center);
		MessageSlot->SetVerticalAlignment(VAlign_Top);
		MessageSlot->SetPadding(FMargin(40.0f, 60.0f, 40.0f, 0.0f));
	}

	// --- Bottom-center info panel: a few stacked rows, each in the existing horizontal-bar style.
	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	// A self-contained color brush (no external style/texture resource lookup) so the background
	// reliably renders regardless of active Slate style.
	RootBorder->SetBrush(FSlateColorBrush(FLinearColor(0.02f, 0.02f, 0.035f, 0.85f)));
	RootBorder->SetPadding(FMargin(18.0f, 10.0f));
	if (UOverlaySlot* BorderSlot = RootOverlay->AddChildToOverlay(RootBorder))
	{
		BorderSlot->SetHorizontalAlignment(HAlign_Center);
		BorderSlot->SetVerticalAlignment(VAlign_Bottom);
		BorderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 40.0f));
	}

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	RootBorder->SetContent(ContentBox);

	// Row: velocity + fuel (unchanged from before, just moved into its own row).
	UHorizontalBox* VelocityRow = CreateRow();
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("VELOCITY")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.9f, 1.0f)));
	TitleText->SetFontSize(16);
	if (UHorizontalBoxSlot* TitleSlot = VelocityRow->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 24.0f, 0.0f));
	}
	ForwardValueText = CreateMetricText(VelocityRow);
	RightValueText = CreateMetricText(VelocityRow);
	UpValueText = CreateMetricText(VelocityRow);
	FuelValueText = CreateMetricText(VelocityRow);

	// Row: gravity status + acceleration + vector.
	UHorizontalBox* GravityRow = CreateRow();
	GravityStatusText = CreateMetricText(GravityRow);
	GravityValueText = CreateMetricText(GravityRow);
	GravityVectorText = CreateMetricText(GravityRow);

	// Row: navigation distances.
	UHorizontalBox* NavigationRow = CreateRow();
	CheckpointDistanceText = CreateMetricText(NavigationRow);
	PlanetCenterDistanceText = CreateMetricText(NavigationRow);
	PlanetSurfaceDistanceText = CreateMetricText(NavigationRow);

	// Row: elapsed exercise time.
	UHorizontalBox* TimeRow = CreateRow();
	TimeText = CreateMetricText(TimeRow);

	UpdateVelocityDisplay(FVector::ZeroVector);
	UpdateFuelDisplay(100.0f);
	UpdateGravityDisplay(FVector::ZeroVector);
	UpdateCheckpointDistance(false, 0.0f);
	UpdatePlanetDistances(false, 0.0f, 0.0f);
	UpdateElapsedTime(0.0f);
}

UHorizontalBox* UCockpitDisplayWidget::CreateRow()
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* RowSlot = ContentBox->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.0f, 2.0f));
	}
	return Row;
}

UTextBlock* UCockpitDisplayWidget::CreateMetricText(UHorizontalBox* Row)
{
	UTextBlock* MetricText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	MetricText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 1.0f, 0.9f)));
	MetricText->SetFontSize(16);
	if (UHorizontalBoxSlot* MetricSlot = Row->AddChildToHorizontalBox(MetricText))
	{
		MetricSlot->SetVerticalAlignment(VAlign_Center);
		MetricSlot->SetPadding(FMargin(0.0f, 0.0f, 20.0f, 0.0f));
	}

	return MetricText;
}

FString UCockpitDisplayWidget::FormatLine(const TCHAR* Label, float SpeedMetersPerSecond)
{
	return FString::Printf(TEXT("%s %+.1f m/s"), Label, SpeedMetersPerSecond);
}

void UCockpitDisplayWidget::UpdateVelocityDisplay(const FVector& LocalVelocity)
{
	if (ForwardValueText)
	{
		ForwardValueText->SetText(FText::FromString(FormatLine(TEXT("FORWARD"), LocalVelocity.X)));
	}
	if (RightValueText)
	{
		RightValueText->SetText(FText::FromString(FormatLine(TEXT("RIGHT"), LocalVelocity.Y)));
	}
	if (UpValueText)
	{
		UpValueText->SetText(FText::FromString(FormatLine(TEXT("UP"), LocalVelocity.Z)));
	}
}

void UCockpitDisplayWidget::UpdateFuelDisplay(float FuelPercent)
{
	if (FuelValueText)
	{
		FuelValueText->SetText(FText::FromString(FString::Printf(TEXT("FUEL %.0f%%"), FuelPercent)));
	}
}

void UCockpitDisplayWidget::OutputMessage(const FText& Message)
{
	if (MessageText)
	{
		MessageText->SetText(Message);
	}
}

void UCockpitDisplayWidget::UpdateGravityDisplay(const FVector& GravityAcceleration)
{
	const bool bGravityActive = !GravityAcceleration.IsZero();

	if (GravityStatusText)
	{
		GravityStatusText->SetText(FText::FromString(bGravityActive ? TEXT("Im Gravitationsfeld") : TEXT("Keine Gravitation")));
	}
	if (GravityValueText)
	{
		GravityValueText->SetText(FText::FromString(FString::Printf(TEXT("Gravity Acceleration: %.2f"), GravityAcceleration.Size())));
	}
	if (GravityVectorText)
	{
		if (bGravityActive)
		{
			GravityVectorText->SetText(FText::FromString(FString::Printf(TEXT("Gravity Vector: X: %.2f  Y: %.2f  Z: %.2f"),
				GravityAcceleration.X, GravityAcceleration.Y, GravityAcceleration.Z)));
			GravityVectorText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			GravityVectorText->SetText(FText::GetEmpty());
			GravityVectorText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCockpitDisplayWidget::UpdateCheckpointDistance(bool bHasActiveCheckpoint, float DistanceToCheckpoint)
{
	if (!CheckpointDistanceText)
	{
		return;
	}

	CheckpointDistanceText->SetText(FText::FromString(bHasActiveCheckpoint
		? FString::Printf(TEXT("Checkpoint Distance: %.0f"), DistanceToCheckpoint)
		: FString(TEXT("Checkpoint Distance: ---"))));
}

void UCockpitDisplayWidget::UpdatePlanetDistances(bool bHasNearestPlanet, float PlanetCenterDistance, float PlanetSurfaceDistance)
{
	if (PlanetCenterDistanceText)
	{
		PlanetCenterDistanceText->SetText(FText::FromString(bHasNearestPlanet
			? FString::Printf(TEXT("Planet Center Distance: %.0f"), PlanetCenterDistance)
			: FString(TEXT("Planet Center Distance: ---"))));
	}
	if (PlanetSurfaceDistanceText)
	{
		PlanetSurfaceDistanceText->SetText(FText::FromString(bHasNearestPlanet
			? FString::Printf(TEXT("Planet Surface Distance: %.0f"), PlanetSurfaceDistance)
			: FString(TEXT("Planet Surface Distance: ---"))));
	}
}

void UCockpitDisplayWidget::UpdateElapsedTime(float ElapsedSeconds)
{
	if (!TimeText)
	{
		return;
	}

	const float ClampedSeconds = FMath::Max(ElapsedSeconds, 0.0f);
	const int32 Minutes = FMath::FloorToInt(ClampedSeconds / 60.0f);
	const float Seconds = ClampedSeconds - static_cast<float>(Minutes) * 60.0f;
	TimeText->SetText(FText::FromString(FString::Printf(TEXT("Time: %02d:%05.2f"), Minutes, Seconds)));
}
