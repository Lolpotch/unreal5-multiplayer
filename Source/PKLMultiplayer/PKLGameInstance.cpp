// Copyright Epic Games, Inc. All Rights Reserved.

#include "PKLGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h" // SEARCH_KEYWORDS, SEARCH_PRESENCE

// Fixed local name for our one session (this is the local handle, not a display name).
static const FName PKLSessionName = NAME_GameSession;

void UPKLGameInstance::Init()
{
	Super::Init();

	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		Sessions = OSS->GetSessionInterface();
		Screen(FString::Printf(TEXT("OnlineSubsystem: %s"), *OSS->GetSubsystemName().ToString()), FColor::Green);
	}
	else
	{
		Screen(TEXT("No OnlineSubsystem found!"), FColor::Red);
	}
}

void UPKLGameInstance::HostSession(bool bUseLAN)
{
	if (!Sessions.IsValid())
	{
		return;
	}

	// Drop any stale session left over from a previous run.
	if (Sessions->GetNamedSession(PKLSessionName))
	{
		Sessions->DestroySession(PKLSessionName);
	}

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = bUseLAN;
	Settings.NumPublicConnections = 4;
	Settings.bShouldAdvertise = true;       // show up in searches
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = true;          // Steam friends/presence
	Settings.bAllowJoinViaPresence = true;
	Settings.bUseLobbiesIfAvailable = true; // Steam lobbies
	Settings.Set(SEARCH_KEYWORDS, FString(TEXT("PKLDemo")), EOnlineDataAdvertisementType::ViaOnlineService);

	Sessions->OnCreateSessionCompleteDelegates.Clear();
	Sessions->OnCreateSessionCompleteDelegates.AddUObject(this, &UPKLGameInstance::OnCreateSessionComplete);

	Screen(FString::Printf(TEXT("Creating %s session..."), bUseLAN ? TEXT("LAN") : TEXT("STEAM")), FColor::Yellow);
	Sessions->CreateSession(0, PKLSessionName, Settings);
}

void UPKLGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	Screen(FString::Printf(TEXT("CreateSession: %s"), bWasSuccessful ? TEXT("OK") : TEXT("FAIL")),
		bWasSuccessful ? FColor::Green : FColor::Red);

	if (bWasSuccessful && GetWorld())
	{
		// Become a listen server on the map. Clients join by travelling to us.
		GetWorld()->ServerTravel(TravelMap + TEXT("?listen"));
	}
}

void UPKLGameInstance::FindAndJoinSession(bool bUseLAN)
{
	if (!Sessions.IsValid())
	{
		return;
	}

	SearchSettings = MakeShareable(new FOnlineSessionSearch());
	SearchSettings->bIsLanQuery = bUseLAN;
	SearchSettings->MaxSearchResults = 20;
	// Match the host: it advertises as a lobby (bUseLobbiesIfAvailable). Steam needs this
	// to return the session; harmless on the LAN/Null subsystem.
	SearchSettings->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	// CRITICAL for the shared test app id 480 (Spacewar): thousands of unrelated public
	// lobbies exist worldwide. Without a filter, Steam returns 20 arbitrary ones (none ours)
	// and they get discarded as non-UE sessions -> zero results. Filter server-side on the
	// same keyword the host advertises so Steam returns ONLY our lobby.
	SearchSettings->QuerySettings.Set(SEARCH_KEYWORDS, FString(TEXT("PKLDemo")), EOnlineComparisonOp::Equals);

	Sessions->OnFindSessionsCompleteDelegates.Clear();
	Sessions->OnFindSessionsCompleteDelegates.AddUObject(this, &UPKLGameInstance::OnFindSessionsComplete);

	Screen(FString::Printf(TEXT("Searching %s sessions..."), bUseLAN ? TEXT("LAN") : TEXT("STEAM")), FColor::Yellow);
	Sessions->FindSessions(0, SearchSettings.ToSharedRef());
}

void UPKLGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful || !SearchSettings.IsValid() || SearchSettings->SearchResults.Num() == 0)
	{
		Screen(TEXT("No sessions found."), FColor::Red);
		return;
	}

	Screen(FString::Printf(TEXT("Found %d session(s), joining first."), SearchSettings->SearchResults.Num()), FColor::Green);

	Sessions->OnJoinSessionCompleteDelegates.Clear();
	Sessions->OnJoinSessionCompleteDelegates.AddUObject(this, &UPKLGameInstance::OnJoinSessionComplete);

	Sessions->JoinSession(0, PKLSessionName, SearchSettings->SearchResults[0]);
}

void UPKLGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		Screen(TEXT("Join failed."), FColor::Red);
		return;
	}

	// Resolve the host address the session points at, then client-travel to it.
	FString ConnectString;
	if (Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		if (APlayerController* PC = GetFirstLocalPlayerController())
		{
			Screen(FString::Printf(TEXT("Joining %s"), *ConnectString), FColor::Green);
			PC->ClientTravel(ConnectString, TRAVEL_Absolute);
		}
	}
	else
	{
		Screen(TEXT("Could not resolve connect string."), FColor::Red);
	}
}

void UPKLGameInstance::LeaveSession()
{
	if (Sessions.IsValid() && Sessions->GetNamedSession(PKLSessionName))
	{
		Sessions->OnDestroySessionCompleteDelegates.Clear();
		Sessions->OnDestroySessionCompleteDelegates.AddUObject(this, &UPKLGameInstance::OnDestroySessionComplete);
		Sessions->DestroySession(PKLSessionName);
	}
}

void UPKLGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	Screen(TEXT("Session destroyed."), FColor::White);
}

void UPKLGameInstance::Screen(const FString& Msg, const FColor& Color) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, Color, Msg);
	}
}