// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PKLCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Simple third-person character used to demonstrate LAN replication.
 * Movement replicates automatically via CharacterMovementComponent.
 * Body colour is a replicated property, changed through a Server RPC,
 * so pressing the colour key on any machine updates the cube on every machine.
 */
UCLASS()
class PKLMULTIPLAYER_API APKLCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APKLCharacter();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// --- Components ---
	UPROPERTY(VisibleAnywhere, Category = "PKL")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "PKL")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "PKL")
	TObjectPtr<UCameraComponent> Camera;

	// --- Replicated state ---
	// Replicated body colour. OnRep applies it to the dynamic material on clients.
	UPROPERTY(ReplicatedUsing = OnRep_BodyColor)
	FLinearColor BodyColor = FLinearColor::Gray;

	UFUNCTION()
	void OnRep_BodyColor();

	// --- Input handlers ---
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void OnChangeColorPressed();

	// Client asks the server to pick a new random colour.
	UFUNCTION(Server, Reliable)
	void ServerRandomizeColor();

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMID;

	void ApplyBodyColor();
};