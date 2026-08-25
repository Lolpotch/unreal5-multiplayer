// Copyright Epic Games, Inc. All Rights Reserved.

#include "PKLGameMode.h"

#include "PKLCharacter.h"
#include "PKLPlayerController.h"
#include "GameFramework/Pawn.h"

APKLGameMode::APKLGameMode()
{
	DefaultPawnClass = APKLCharacter::StaticClass();
	PlayerControllerClass = APKLPlayerController::StaticClass();
}

void APKLGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// If the map has PlayerStarts the pawn is already placed well; this only
	// nudges players apart so an empty map doesn't stack everyone at the origin.
	if (NewPlayer && NewPlayer->GetPawn())
	{
		const FVector Offset(SpawnCount * 200.f, 0.f, 100.f);
		NewPlayer->GetPawn()->SetActorLocation(
			NewPlayer->GetPawn()->GetActorLocation() + Offset,
			false, nullptr, ETeleportType::TeleportPhysics);
	}
	++SpawnCount;
}