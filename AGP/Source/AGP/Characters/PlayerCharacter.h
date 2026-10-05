// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "PlayerCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class AGP_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly)
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly)
	UInputAction* JumpAction;
	
	UPROPERTY(EditDefaultsOnly)
	UInputAction* FireAction;
	
	UPROPERTY(EditDefaultsOnly)
	UInputAction* ReloadAction;
	
	UPROPERTY(EditDefaultsOnly)
	UInputAction* MeleeAction;

	UPROPERTY(EditDefaultsOnly)
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, meta = (UIMin = "0.0", UIMax = "1.0"))
	float LookSensitivity;
	
	// Melee reach in cm, measured from the camera.
	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	float MeleeRange = 60.0f;
	
	// Damage dealt by one melee hit.
	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	float MeleeDamage = 100.0f;
	
	// Melee swinging cooldown.
	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	float MeleeCooldown = 0.7f;
	
	// World time of the last swing.
	float LastMeleeTime = -1000.0f;

	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void FireWeapon(const FInputActionValue& Value);
	void ReloadWeapon(const FInputActionValue& Value);
	void Melee(const FInputActionValue& Value);


};
