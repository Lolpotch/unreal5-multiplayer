// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PKLGameMode.generated.h"

/**
 * Minimal game mode for the LAN demo. Sets the default pawn / controller and
 * spreads spawned players out so they don't stack on the same spot when the
 * map has no PlayerStart actors.
 */
UCLASS()
class PKLMULTIPLAYER_API APKLGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APKLGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	int32 SpawnCount = 0;
};