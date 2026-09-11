// Copyright Epic Games, Inc. All Rights Reserved.

#include "PKLPlayerController.h"

#include "PKLGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void APKLPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Show the local player how to host / join.
	if (IsLocalController() && GEngine)
	{
		const ENetMode NetMode = GetNetMode();
		const TCHAR* NetRole =
			NetMode == NM_ListenServer ? TEXT("LISTEN SERVER") :
			NetMode == NM_Client       ? TEXT("CLIENT") :
			                             TEXT("STANDALONE");

		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Cyan,
			FString::Printf(TEXT("Role: %s"), NetRole));
		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::White,
			TEXT("Console (~):  HostLAN   |   JoinLAN 127.0.0.1   |   in-game C = change colour"));
	}
}

void APKLPlayerController::HostLAN()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Reload the current level as a listen server so clients can connect.
	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix); // strip PIE prefix if present

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Green,
			FString::Printf(TEXT("Hosting listen server on %s ..."), *MapName));
	}

	UGameplayStatics::OpenLevel(World, FName(*MapName), true, TEXT("listen"));
}

void APKLPlayerController::JoinLAN(const FString& IpAddress)
{
	const FString Address = IpAddress.IsEmpty() ? TEXT("127.0.0.1") : IpAddress;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow,
			FString::Printf(TEXT("Joining %s ..."), *Address));
	}

	ClientTravel(Address, TRAVEL_Absolute);
}

void APKLPlayerController::Host(bool bLAN)
{
	if (UPKLGameInstance* GI = GetGameInstance<UPKLGameInstance>())
	{
		GI->HostSession(bLAN);
	}
}

void APKLPlayerController::Find(bool bLAN)
{
	if (UPKLGameInstance* GI = GetGameInstance<UPKLGameInstance>())
	{
		GI->FindAndJoinSession(bLAN);
	}
}

void APKLPlayerController::Leave()
{
	if (UPKLGameInstance* GI = GetGameInstance<UPKLGameInstance>())
	{
		GI->LeaveSession();
	}
}