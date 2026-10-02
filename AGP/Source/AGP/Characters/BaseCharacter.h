// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "AGP/Components/WeaponComponent.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

class UWeaponComponent;
class UHealthComponent;

UCLASS()
class AGP_API ABaseCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// float TimeSinceLastShot;
	// float MinTimeBetweenShots;
	// float WeaponDamage = 10.0f;
	
	UPROPERTY(VisibleAnywhere)
	UWeaponComponent* WeaponComponent;
	
	/** Whether this character is armed. Unarmed characters ignore Fire and Reload. */
	UPROPERTY(EditDefaultsOnly)
	bool bHasWeapon = true;
	
	UPROPERTY(VisibleAnywhere)
	USceneComponent* BulletStartPosition;
	
	UPROPERTY(VisibleAnywhere)
	UHealthComponent* HealthComponent;
	
	UPROPERTY(VisibleAnywhere)
	FGenericTeamId TeamID;
	
	bool Fire(const FVector& FireAtLocation);
	
	void Reload();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	/** Is this character armed? */
	UFUNCTION(BlueprintCallable)
	bool HasWeapon() const;
	
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	
	virtual FGenericTeamId GetGenericTeamId() const override;
	
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	
	
	
};
