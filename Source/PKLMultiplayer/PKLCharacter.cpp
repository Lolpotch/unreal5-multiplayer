// Copyright Epic Games, Inc. All Rights Reserved.

#include "PKLCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

APKLCharacter::APKLCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Actor + movement replication (movement is handled by CharacterMovementComponent).
	bReplicates = true;
	SetReplicateMovement(true);

	// Rotate the character towards movement instead of towards the controller.
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
		Move->RotationRate = FRotator(0.f, 540.f, 0.f);
		Move->JumpZVelocity = 500.f;
		Move->AirControl = 0.35f;
	}

	// Visible body: an engine cube parented to the capsule so we can see each player.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	// Fill the capsule (half-height ~88, radius ~34): cube is 100 units, centred on the actor.
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	BodyMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.76f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeMesh.Object);
	}

	// The Cube's default material is the checkered WorldGridMaterial, which has no
	// colour parameter. Swap in BasicShapeMaterial, which exposes a "Color" vector param.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BodyMat.Succeeded())
	{
		BodyMesh->SetMaterial(0, BodyMat.Object);
	}

	// Third-person camera.
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetCapsuleComponent());
	SpringArm->TargetArmLength = 400.f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 60.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

void APKLCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Create a dynamic material instance so we can recolour the cube at runtime.
	if (BodyMesh && BodyMesh->GetMaterial(0))
	{
		BodyMID = BodyMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	// Server hands each player a distinct starting colour based on player index.
	if (HasAuthority())
	{
		static const FLinearColor Palette[] = {
			FLinearColor(0.9f, 0.1f, 0.1f), // red
			FLinearColor(0.1f, 0.3f, 0.9f), // blue
			FLinearColor(0.1f, 0.8f, 0.2f), // green
			FLinearColor(0.9f, 0.8f, 0.1f), // yellow
			FLinearColor(0.7f, 0.1f, 0.9f), // purple
		};
		const int32 Index = static_cast<int32>(GetUniqueID() % UE_ARRAY_COUNT(Palette));
		BodyColor = Palette[Index];
	}

	ApplyBodyColor();
}

void APKLCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APKLCharacter, BodyColor);
}

void APKLCharacter::OnRep_BodyColor()
{
	ApplyBodyColor();
}

void APKLCharacter::ApplyBodyColor()
{
	if (BodyMID)
	{
		// "Color" works with the engine's basic material; also set common param names as fallback.
		BodyMID->SetVectorParameterValue(TEXT("Color"), BodyColor);
		BodyMID->SetVectorParameterValue(TEXT("BaseColor"), BodyColor);
	}
}

void APKLCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Legacy bindings — mappings live in Config/DefaultInput.ini.
	// Works because EnhancedInputComponent derives from UInputComponent.
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &APKLCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &APKLCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APKLCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APKLCharacter::LookUp);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("ChangeColor"), IE_Pressed, this, &APKLCharacter::OnChangeColorPressed);
}

void APKLCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Value);
	}
}

void APKLCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Value);
	}
}

void APKLCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void APKLCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void APKLCharacter::OnChangeColorPressed()
{
	// Runs on the owning client; forward the request to the server.
	ServerRandomizeColor();
}

void APKLCharacter::ServerRandomizeColor_Implementation()
{
	// Authoritative change; replication + OnRep pushes it to every client.
	BodyColor = FLinearColor::MakeRandomColor();
	ApplyBodyColor(); // update the server's own view immediately
}