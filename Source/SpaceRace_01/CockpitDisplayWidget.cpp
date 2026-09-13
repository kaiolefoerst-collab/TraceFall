// Fill out your copyright notice in the Description page of Project Settings.

#include "CockpitDisplayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
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

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	// A self-contained color brush (no external style/texture resource lookup) so the background
	// reliably renders regardless of active Slate style.
	RootBorder->SetBrush(FSlateColorBrush(FLinearColor(0.02f, 0.02f, 0.035f, 0.85f)));
	RootBorder->SetPadding(FMargin(18.0f, 8.0f));
	WidgetTree->RootWidget = RootBorder;

	ContentBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentBox"));
	RootBorder->SetContent(ContentBox);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("VELOCITY")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.9f, 1.0f)));
	TitleText->SetFontSize(16);
	if (UHorizontalBoxSlot* TitleSlot = ContentBox->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 24.0f, 0.0f));
	}

	ForwardValueText = CreateMetricText();
	RightValueText = CreateMetricText();
	UpValueText = CreateMetricText();

	UpdateVelocityDisplay(FVector::ZeroVector);
}

UTextBlock* UCockpitDisplayWidget::CreateMetricText()
{
	UTextBlock* MetricText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	MetricText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 1.0f, 0.9f)));
	MetricText->SetFontSize(16);
	if (UHorizontalBoxSlot* MetricSlot = ContentBox->AddChildToHorizontalBox(MetricText))
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
