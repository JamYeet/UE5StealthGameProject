// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "EnemyCharacter.generated.h"

struct FAIStimulus;
class UAIPerceptionComponent;
class APlayerCharacter;

UENUM(BlueprintType)
enum class EEnemyState:uint8 
{
	Patrol,
	Engage,
	Evade
};


class UPathfindingSubsystem;

UCLASS()
class AGP_API AEnemyCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY()
	UPathfindingSubsystem* PathfindingSubsystem;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FVector> CurrentPath;
	
	UPROPERTY(EditDefaultsOnly)
	float PathingLocationThreshold;
	
	UPROPERTY(EditAnywhere)
	EEnemyState CurrentState = EEnemyState::Patrol;
	
	UPROPERTY(VisibleAnywhere)
	UAIPerceptionComponent* AIPerceptionComponent;
	
	UPROPERTY(VisibleInstanceOnly)
	APlayerCharacter* SensedPlayer;
	
	UPROPERTY(VisibleInstanceOnly)
	FVector TargetLocation;
	
	UPROPERTY(EditDefaultsOnly)
	float EvadeHealthPercentageThreshold;
	
	void TickPatrol();
	void TickEngage();
	void TickEvade();
	void MoveAlongPath();
	
	UFUNCTION()
	void OnSensedActor(AActor* Actor, FAIStimulus Stimulus);
	
	UFUNCTION()
	void OnForgetActor(AActor* Actor);
	
	void UpdateTargetLocation();
	
	bool OutOfAmmo();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
