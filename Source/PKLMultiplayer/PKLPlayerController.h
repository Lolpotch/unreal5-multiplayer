// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PKLPlayerController.generated.h"

/**
 * Player controller exposing simple LAN host/join commands.
 * Open the console (~) and type:
 *   HostLAN            - become a listen server on the current level
 *   JoinLAN 127.0.0.1  - connect to a host by IP (default 127.0.0.1)
 */
UCLASS()
class PKLMULTIPLAYER_API APKLPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	// Host a listen server on the currently loaded level.
	UFUNCTION(Exec)
	void HostLAN();

	// Join a host by IP address (defaults to localhost for same-PC testing).
	UFUNCTION(Exec)
	void JoinLAN(const FString& IpAddress = TEXT("127.0.0.1"));
};