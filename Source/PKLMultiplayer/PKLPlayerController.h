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

	// --- Direct IP (no session layer) ---

	// Host a listen server on the currently loaded level.
	UFUNCTION(Exec)
	void HostLAN();

	// Join a host by IP address (defaults to localhost for same-PC testing).
	UFUNCTION(Exec)
	void JoinLAN(const FString& IpAddress = TEXT("127.0.0.1"));

	// --- Session layer (Steam by default; pass 1 for LAN sessions) ---

	// Create a session and host. `bLAN`: 0 = Steam, 1 = LAN broadcast.
	UFUNCTION(Exec)
	void Host(bool bLAN = false);

	// Search for a session and join the first found. `bLAN`: 0 = Steam, 1 = LAN.
	UFUNCTION(Exec)
	void Find(bool bLAN = false);

	// Destroy the current session.
	UFUNCTION(Exec)
	void Leave();
};