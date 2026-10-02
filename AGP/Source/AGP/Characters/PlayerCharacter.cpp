// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include <InputAction.h>
#include <EnhancedInputSubsystems.h>
#include <EnhancedInputComponent.h>

#include "Kismet/KismetMathLibrary.h"


// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	LookSensitivity = 0.3f;
	TeamID = FGenericTeamId(1);
	
	// Players are unarmed (a melee weapon is planned for later).
	bHasWeapon = false;

}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// This gets the controller  that controls this character (if possessed by the player).
	// Then we get the LocalPlayerSubsystem connected to the controller (local means on this client, which matters more once we get to online multiplayer).
	// This subsystem is cast to the UEnhancedInputLocalPlayerSubsystem and add the mapping context to that subsystem. 
	// This connects the IMC to this player controller and allows movement inputs to be read.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) 
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) 
		{
			Subsystem->AddMappingContext(InputMappingContext,0);
		}
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind your movement functions to the InputActions belonging to the PlayerInputComponent. 
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		// Check if input actions are not null to avoid potential reference errors.
		if (MoveAction) {
			Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		}
		if (LookAction) {
			Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		}
		if (JumpAction) {
			// Note that I am binding the Jump action of the ACharacter SuperClass that APlayerCharacter is inheriting from.
			Input->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		}
		if (FireAction)
		{
			Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &APlayerCharacter::FireWeapon);
		}
		if (ReloadAction)
		{
			Input->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &APlayerCharacter::ReloadWeapon);
		}
	}
}


/// <summary>
/// Apply movement inputs to the player character.
/// </summary>
void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// Get the 2D Movement Input value
	const FVector2D MovementVector = Value.Get<FVector2D>();

	// Apply the forward movement to the character.
	const FVector ForwardVector = GetActorForwardVector();
	AddMovementInput(ForwardVector, MovementVector.X);

	// Apply the later movement to the character.
	const FVector RightVector = GetActorRightVector();
	AddMovementInput(RightVector, MovementVector.Y);
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookVector.X * LookSensitivity);
	// Negative 1.0f factor to invert pitch direction as using Mouse XY 2D-Axis, you cannot apply the Negate Modifier on only one axis.
	AddControllerPitchInput(-1.0f* LookVector.Y * LookSensitivity);
}

void APlayerCharacter::FireWeapon(const FInputActionValue& Value)
{
	// Cache Camera Position and Rotation of Player's camera.
	FVector CameraPosition;
	FRotator CameraRotation;
	GetWorld()->GetFirstPlayerController()->GetPlayerViewPoint(CameraPosition, CameraRotation);
	
	// Get the forward vector of the camera rotation in world-space.
	const FVector CameraForward = UKismetMathLibrary::GetForwardVector(CameraRotation);
	
	// Fire the weapon in the direction the player camera is facing.
	if (BulletStartPosition)
	{
		Fire(BulletStartPosition->GetComponentLocation() + 10000.0f*CameraForward);
	}
}

void APlayerCharacter::ReloadWeapon(const FInputActionValue& Value)
{
	Reload();
}


