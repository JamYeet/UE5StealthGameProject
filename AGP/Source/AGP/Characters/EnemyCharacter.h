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
class ANavigationNode;


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
	
	
	UPROPERTY(VisibleAnywhere, Category = "AI | Perception")
	UAISenseConfig_Sight* SightConfig;
	
	UPROPERTY(VisibleAnywhere, Category = "AI | Detection")
	UDetectionComponent* DetectionComponent;
	
	// True while the perception system reports the player as currently in sight
	bool bPlayerVisible = false;
	
	UPROPERTY(EditAnywhere, Category = "AI | Debug")
	bool bDrawDebug = true;
	
	void DrawSightCone() const;
	void DrawDebugInfo() const;
	
	// Waypoints for this guard's route, picked in order in the editor for now. The guard walks them back and forth, it falls back to random patrol.
	UPROPERTY(EditInstanceOnly, Category = "AI | Patrol")
	TArray<ANavigationNode*> PatrolNodes;
	
	// Seconds a guard stands still at either end of its route before turning around.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Patrol")
	float PatrolWaitDuration = 5.0f;
	
	// The route as positions, built from PatrolNodes at BeginPlay or set directly with SetPatrolRoute.
	TArray<FVector> PatrolRoute;
	
	// Index of the waypoint the guard is heading to, and which way along the route it is going (+1 or -1).
	int32 PatrolTargetIndex = 0;
	int32 PatrolDirection = 1;
	
	// True once a path to the current waypoint has been requested.
	bool bPatrolPathRequested = false;
	
	bool bWaitingAtEnd = false;
	float PatrolWaitTimer = 0.0f;
	
	// Set after a wait: the guard turns on the spot to face the way back before it starts walking.
	bool bTurnBeforeWalking = false;
	
	void TickPatrolRoute(float DeltaTime);
	void OnReachedWaypoint();
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Search")
	float SearchDuration = 12.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Suspicious")
	float SuspiciousDuration = 3.0f;
	
	// Seconds the player has been out of sight while Suspicious.
	float SuspiciousTimer = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Alerted")
	float LoseTargetDuration = 5.0f;
	
	// Seconds the player has been out of sight while Alerted. 
	float LostSightTimer = 0.0f;
	
	// Seconds left in the current search. 
	float SearchTimer = 0.0f;
	
	// Walk speeds per state in cm/s. Suspicious and Death stand still.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float PatrolSpeed = 150.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float SuspiciousSpeed = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float SearchSpeed = 275.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float AlertedSpeed = 500.0f;
	
	// How fast the guard turns, in degrees per second.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float TurnSpeed = 180.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float TrackTurnSpeed = 720.0f;
	
	
	void SetState(EEnemyState NewState);
	void EnterState(EEnemyState NewState);
	void ExitState(EEnemyState OldState);
	void ApplyStateSpeed(EEnemyState State);
	void FaceLocation(const FVector& Location, float DeltaTime, float Speed = 0.0f);
	void EndSearch();
	
	
	void TickPatrol(float DeltaTime);
	void TickSuspicious(float DeltaTime);
	void TickAlerted(float DeltaTime);
	void TickSearch(float DeltaTime);
	void TickDeath();
	
	void MoveAlongPath(float DeltaTime, bool bFaceMovement = true);
	
	UFUNCTION()
	void OnSensedActor(AActor* Actor, FAIStimulus Stimulus);
	
	UFUNCTION()
	void OnForgetActor(AActor* Actor);
	
	// Feeds the detection meter with what this guard can currently see.
	void UpdateDetection(float DeltaTime);
	
	bool OutOfAmmo();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Replaces this guard's patrol route (e.g. from the level generator). Needs two or more points.
	void SetPatrolRoute(const TArray<FVector>& NewRoute);

};
