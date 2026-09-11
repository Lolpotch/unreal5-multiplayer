// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "PKLGameInstance.generated.h"

/**
 * Session layer that sits on top of the (unchanged) replicated gameplay.
 * Hosts and joins through the Online Subsystem, so the same code works on LAN
 * or Steam depending on the bUseLAN flag:
 *   - bUseLAN = true  -> Null subsystem / LAN broadcast (no Steam needed)
 *   - bUseLAN = false -> Steam sessions (needs Steam running, app id 480 for testing)
 */
UCLASS()
class PKLMULTIPLAYER_API UPKLGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// Create a session and, on success, ServerTravel to the level as a listen server.
	UFUNCTION(BlueprintCallable, Category = "PKL|Session")
	void HostSession(bool bUseLAN = false);

	// Search for a session and join the first result found.
	UFUNCTION(BlueprintCallable, Category = "PKL|Session")
	void FindAndJoinSession(bool bUseLAN = false);

	// Destroy the current session (call before quitting / returning to menu).
	UFUNCTION(BlueprintCallable, Category = "PKL|Session")
	void LeaveSession();

private:
	IOnlineSessionPtr Sessions;
	TSharedPtr<FOnlineSessionSearch> SearchSettings;

	// Level travelled to when hosting.
	FString TravelMap = TEXT("/Game/Level1");

	// Delegate handlers.
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	void Screen(const FString& Msg, const FColor& Color) const;
};