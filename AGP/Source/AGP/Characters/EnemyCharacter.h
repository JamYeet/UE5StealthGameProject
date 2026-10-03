// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "EnemyCharacter.generated.h"

struct FAIStimulus;
class UAIPerceptionComponent;
class APlayerCharacter;
class UAISenseConfig_Sight;
class UDetectionComponent;

UENUM(BlueprintType)
enum class EEnemyState:uint8 
{
	Patrol,
	Suspicious,
	Alerted,
	Search,
	Death
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
	
	UPROPERTY(VisibleInstanceOnly)
	EEnemyState CurrentState = EEnemyState::Patrol;
	
	UPROPERTY(VisibleAnywhere)
	UAIPerceptionComponent* AIPerceptionComponent;
	
	UPROPERTY(VisibleInstanceOnly)
	APlayerCharacter* SensedPlayer;
	
	UPROPERTY(VisibleInstanceOnly)
	FVector TargetLocation;
	
	UPROPERTY(VisibleAnywhere, Category = "AI | Perception")
	UAISenseConfig_Sight* SightConfig;
	
	UPROPERTY(VisibleAnywhere, Category = "AI | Detection")
	UDetectionComponent* DetectionComponent;
	
	// True while the perception system reports the player as currently in sight
	bool bPlayerVisible = false;
	
	UPROPERTY(EditAnywhere, Category = "AI | Debug")
	bool bDrawDebug = true;
	
	UPROPERTY(EditAnywhere, Category = "AI | Debug")
	bool bDebugStandStill = true;
	

	void DrawSightCone() const;
	void DrawDebugInfo() const;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Search")
	float SearchDuration = 12.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Suspicious")
	float SuspiciousDuration = 3.0f;
	
	//
	float SuspiciousTimer = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Alerted")
	float LoseTargetDuration = 5.0f;
	
	// Seconds the player has been out of sight while Alerted. 
	float LostSightTimer = 0.0f;
	
	// Seconds left in the current search. 
	float SearchTimer = 0.0f;
	
	
	
	void SetState(EEnemyState NewState);
	void EnterState(EEnemyState NewState);
	void ExitState(EEnemyState OldState);
	void EndSearch();
	
	
	void TickPatrol();
	void TickSuspicious(float DeltaTime);
	void TickAlerted(float DeltaTime);
	void TickSearch(float DeltaTime);
	void TickDeath();
	
	void MoveAlongPath();
	
	UFUNCTION()
	void OnSensedActor(AActor* Actor, FAIStimulus Stimulus);
	
	UFUNCTION()
	void OnForgetActor(AActor* Actor);
	
	void UpdateTargetLocation();
	
	// Feeds the detection meter with what this guard can currently see.
	void UpdateDetection(float DeltaTime);
	
	bool OutOfAmmo();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
