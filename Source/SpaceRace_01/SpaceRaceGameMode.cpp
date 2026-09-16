// Fill out your copyright notice in the Description page of Project Settings.

#include "SpaceRaceGameMode.h"

#include "SpaceRaceCheckpointComponent.h"
#include "SpaceRaceGameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSpaceRaceCheckpoints, Log, All);

namespace
{
	// Extracts the trailing run of digits from a name, e.g. "Checkpoint01" -> 1.
	bool TryGetTrailingNumber(const FString& Name, int32& OutNumber)
	{
		int32 EndIndex = Name.Len();
		int32 StartIndex = EndIndex;
		while (StartIndex > 0 && FChar::IsDigit(Name[StartIndex - 1]))
		{
			--StartIndex;
		}

		if (StartIndex == EndIndex)
		{
			return false;
		}

		OutNumber = FCString::Atoi(*Name.Mid(StartIndex, EndIndex - StartIndex));
		return true;
	}
}

void ASpaceRaceGameMode::BeginPlay()
{
	Super::BeginPlay();

	RaceStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	InitializeCheckpoints();
}

AActor* ASpaceRaceGameMode::GetActiveCheckpoint() const
{
	return SortedCheckpoints.IsValidIndex(CurrentCheckpointIndex) ? SortedCheckpoints[CurrentCheckpointIndex] : nullptr;
}

float ASpaceRaceGameMode::GetElapsedRaceTime() const
{
	const UWorld* World = GetWorld();
	return World ? (World->GetTimeSeconds() - RaceStartTimeSeconds) : 0.0f;
}

void ASpaceRaceGameMode::InitializeCheckpoints()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStaticMeshActor::StaticClass(), FoundActors);

	TArray<TPair<int32, AActor*>> NumberedCheckpoints;
	for (AActor* Actor : FoundActors)
	{
		if (!Actor || !Actor->FindComponentByClass<USpaceRaceCheckpointComponent>())
		{
			continue;
		}

		// In-editor, actors are identified by their World Outliner label (e.g. "Checkpoint01"),
		// which can differ from the underlying object name. GetActorLabel() is editor-only, so
		// packaged builds fall back to the object name - level authors must keep those in sync.
#if WITH_EDITOR
		const FString ActorName = Actor->GetActorLabel();
#else
		const FString ActorName = Actor->GetName();
#endif

		int32 Number = 0;
		if (TryGetTrailingNumber(ActorName, Number))
		{
			NumberedCheckpoints.Add(TPair<int32, AActor*>(Number, Actor));
		}
	}

	NumberedCheckpoints.Sort([](const TPair<int32, AActor*>& A, const TPair<int32, AActor*>& B)
	{
		return A.Key < B.Key;
	});

	SortedCheckpoints.Reset();
	for (const TPair<int32, AActor*>& Entry : NumberedCheckpoints)
	{
		SortedCheckpoints.Add(Entry.Value);
	}

	UE_LOG(LogSpaceRaceCheckpoints, Log, TEXT("InitializeCheckpoints: found %d checkpoint(s) among %d StaticMeshActor(s)."), SortedCheckpoints.Num(), FoundActors.Num());

	// Every checkpoint starts deactivated; only the first one is then switched on.
	for (AActor* Checkpoint : SortedCheckpoints)
	{
		SetCheckpointActive(Checkpoint, false);
	}

	CurrentCheckpointIndex = INDEX_NONE;
	if (SortedCheckpoints.Num() > 0)
	{
		ActivateCheckpoint(0);
	}
}

void ASpaceRaceGameMode::ActivateCheckpoint(int32 Index)
{
	if (!SortedCheckpoints.IsValidIndex(Index))
	{
		return;
	}

	CurrentCheckpointIndex = Index;

	// SetCollisionEnabled/SetCollisionResponseToAllChannels each trigger UpdateOverlaps()
	// internally, so if the ship already sits inside this checkpoint (true for Checkpoint00 at
	// level start) the normal BeginOverlap event fires here - no separate start-text handling.
	SetCheckpointActive(SortedCheckpoints[Index], true);
}

void ASpaceRaceGameMode::SetCheckpointActive(AActor* CheckpointActor, bool bActive)
{
	if (!CheckpointActor)
	{
		return;
	}

	CheckpointActor->SetActorHiddenInGame(!bActive);

	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(CheckpointActor->GetRootComponent()))
	{
		if (bActive)
		{
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			PrimitiveComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
			PrimitiveComponent->SetGenerateOverlapEvents(true);
		}
		else
		{
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ASpaceRaceGameMode::CheckpointReached()
{
	if (!SortedCheckpoints.IsValidIndex(CurrentCheckpointIndex))
	{
		// Nothing is currently active to report - ignore (e.g. a duplicate overlap callback
		// arriving in the same frame after the checkpoint was already deactivated below).
		return;
	}

	const int32 ReachedIndex = CurrentCheckpointIndex;
	CurrentCheckpointIndex = INDEX_NONE;

	SetCheckpointActive(SortedCheckpoints[ReachedIndex], false);

	const int32 NextIndex = ReachedIndex + 1;
	if (SortedCheckpoints.IsValidIndex(NextIndex))
	{
		ActivateCheckpoint(NextIndex);
	}
	else
	{
		GetWorldTimerManager().SetTimer(FinishTimerHandle, this, &ASpaceRaceGameMode::EndRace, FinishDelaySeconds, false);
	}
}

void ASpaceRaceGameMode::EndRace()
{
	if (USpaceRaceGameInstance* GameInstance = GetGameInstance<USpaceRaceGameInstance>())
	{
		GameInstance->ReturnToMainMenu();
	}
}
