// Fill out your copyright notice in the Description page of Project Settings.

#include "SpaceRaceMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Brushes/SlateColorBrush.h"
#include "Kismet/KismetSystemLibrary.h"

void URaceLevelButtonHandler::HandleClicked()
{
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->StartRace(LevelInfo);
	}
}

void USpaceRaceMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();
}

void USpaceRaceMainMenuWidget::BuildLayout()
{
	if (!WidgetTree || RootOverlay)
	{
		return;
	}

	// Invisible root filling the whole viewport, so it can center the actual menu panel.
	RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	UBorder* ContentBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ContentBorder"));
	ContentBorder->SetBrush(FSlateColorBrush(FLinearColor(0.02f, 0.02f, 0.05f, 0.9f)));
	ContentBorder->SetPadding(FMargin(48.0f, 36.0f));
	if (UOverlaySlot* BorderSlot = RootOverlay->AddChildToOverlay(ContentBorder))
	{
		BorderSlot->SetHorizontalAlignment(HAlign_Center);
		BorderSlot->SetVerticalAlignment(VAlign_Center);
	}

	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	ContentBorder->SetContent(ContentBox);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("SpaceRace")));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.9f, 1.0f)));
	TitleText->SetFontSize(48);
	if (UVerticalBoxSlot* TitleSlot = ContentBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UTextBlock* WelcomeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WelcomeText"));
	WelcomeText->SetText(FText::FromString(TEXT("Willkommen!")));
	WelcomeText->SetJustification(ETextJustify::Center);
	WelcomeText->SetFontSize(20);
	if (UVerticalBoxSlot* WelcomeSlot = ContentBox->AddChildToVerticalBox(WelcomeText))
	{
		WelcomeSlot->SetHorizontalAlignment(HAlign_Center);
		WelcomeSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	RaceButtonHandlers.Reset();
	if (const USpaceRaceGameInstance* GameInstance = GetGameInstance<USpaceRaceGameInstance>())
	{
		for (const FSpaceRaceLevelInfo& LevelInfo : GameInstance->GetAvailableRaceLevels())
		{
			AddRaceButton(LevelInfo);
		}
	}

	UButton* QuitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuitButton"));
	UTextBlock* QuitLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	QuitLabel->SetText(FText::FromString(TEXT("Spiel beenden")));
	QuitLabel->SetJustification(ETextJustify::Center);
	QuitButton->SetContent(QuitLabel);
	QuitButton->OnClicked.AddDynamic(this, &USpaceRaceMainMenuWidget::HandleQuitClicked);
	if (UVerticalBoxSlot* QuitSlot = ContentBox->AddChildToVerticalBox(QuitButton))
	{
		QuitSlot->SetHorizontalAlignment(HAlign_Center);
		QuitSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));
	}
}

void USpaceRaceMainMenuWidget::AddRaceButton(const FSpaceRaceLevelInfo& LevelInfo)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

	UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ButtonLabel->SetText(FText::FromString(LevelInfo.DisplayName));
	ButtonLabel->SetJustification(ETextJustify::Center);
	Button->SetContent(ButtonLabel);

	URaceLevelButtonHandler* Handler = NewObject<URaceLevelButtonHandler>(this);
	Handler->LevelInfo = LevelInfo;
	Handler->OwnerWidget = this;
	Button->OnClicked.AddDynamic(Handler, &URaceLevelButtonHandler::HandleClicked);
	RaceButtonHandlers.Add(Handler);

	if (UVerticalBoxSlot* ButtonSlot = ContentBox->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetPadding(FMargin(0.0f, 6.0f));
	}
}

void USpaceRaceMainMenuWidget::StartRace(const FSpaceRaceLevelInfo& LevelInfo)
{
	if (USpaceRaceGameInstance* GameInstance = GetGameInstance<USpaceRaceGameInstance>())
	{
		GameInstance->StartRace(LevelInfo);
	}
}

void USpaceRaceMainMenuWidget::HandleQuitClicked()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
	}
}
