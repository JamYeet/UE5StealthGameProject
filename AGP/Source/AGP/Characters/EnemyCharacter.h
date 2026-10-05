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
class UPathfindingSubsystem;


// The behaviour a guard is currently in. See the state machine diagram in the video.
UENUM(BlueprintType)
enum class EEnemyState:uint8 
{
	Patrol,
	Suspicious,
	Alerted,
	Search,
	Death
};

// Search mode phases. Walking to the node, looking around it, then a final wait before giving up.
enum class ESearchPhase : uint8
{
	MovingToNode,
	LookingAround,
	FinalWait
};



//  A guard driven by a hand-written finite state machine. Sight feeds a detection meter (UDetectionComponent),
//  and the meter and a few timers decide which state the guard is in. Movement uses the pathfinding subsystem.
UCLASS()
class AGP_API AEnemyCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();
	
	virtual void Tick(float DeltaTime) override;

	// Applies the state-based damage rule: an Alerted guard takes a fraction of the damage, any other state takes it all.
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// Replaces this guard's patrol route (e.g. from the level generator). Needs two or more points.
	void SetPatrolRoute(const TArray<FVector>& NewRoute);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// ----------------------------------------------------------------------------------------------------
	// Components
	// ----------------------------------------------------------------------------------------------------
	
	UPROPERTY(VisibleAnywhere, Category = "AI | Perception")
	UAIPerceptionComponent* AIPerceptionComponent;

	/** Sight sense used by the perception component. Configured in the constructor. */
	UPROPERTY(VisibleAnywhere, Category = "AI | Perception")
	UAISenseConfig_Sight* SightConfig;

	/** Holds the detection meter and the last known player position. */
	UPROPERTY(VisibleAnywhere, Category = "AI | Detection")
	UDetectionComponent* DetectionComponent;
	
	// ----------------------------------------------------------------------------------------------------
	// Movement settings
	// ----------------------------------------------------------------------------------------------------
	
	// How close in cm the guard must get to a path point before it counts as reached.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float PathingLocationThreshold = 150.0f;

	// Walk speeds per state in cm/s. Suspicious and Death stand still.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float PatrolSpeed = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float SuspiciousSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float SearchSpeed = 275.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float AlertedSpeed = 500.0f;

	// How fast the guard turns when walking or looking around, in degrees per second.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float TurnSpeed = 180.0f;

	// Turn speed while tracking the player, high so the player cannot orbit a guard and slip out of its sight cone.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Movement")
	float TrackTurnSpeed = 720.0f;
	
	// ----------------------------------------------------------------------------------------------------
	// Patrol settings
	// ----------------------------------------------------------------------------------------------------
	
	// Waypoints for this guard's route, picked in order in the editor. The guard walks them back and forth.
	UPROPERTY(EditInstanceOnly, Category = "AI | Patrol")
	TArray<ANavigationNode*> PatrolNodes;

	//Seconds a guard stands still at either end of its route before turning around.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Patrol")
	float PatrolWaitDuration = 5.0f;

	// ----------------------------------------------------------------------------------------------------
	// Suspicious settings
	// ----------------------------------------------------------------------------------------------------
	
	// Seconds a guard keeps looking when the player is out of view before it goes into Search mode.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Suspicious")
	float SuspiciousDuration = 3.0f;
	
	// ----------------------------------------------------------------------------------------------------
	// Alerted settings
	// ----------------------------------------------------------------------------------------------------
	
	// Seconds without sight of the player before an Alerted guard gives up the chase and starts searching.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Alerted")
	float LoseTargetDuration = 5.0f;

	// A visible player inside this distance makes the guard stop and shoot instead of chasing.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Alerted")
	float EngageRange = 600.0f;

	// The guard only starts chasing again once the player is beyond this. Larger than EngageRange so a player at
	// the edge of the range does not make the guard stutter between stopping and running.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Alerted")
	float ResumeChaseRange = 800.0f;

	// Seconds between path rebuilds while chasing.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Alerted")
	float RepathInterval = 0.5f;

	// Fraction of incoming damage an Alerted guard takes. 0.25 means a 100 damage hit does 25.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Alerted", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AlertedDamageMultiplier = 0.25f;
	
	// ----------------------------------------------------------------------------------------------------
	// Search settings
	// ----------------------------------------------------------------------------------------------------
	
	// Failsafe: a search always ends after this many seconds, however far it has got.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Search")
	float SearchDuration = 30.0f;

	// Seconds the guard pauses on each direction while looking around at the search node.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Search")
	float SearchLookPauseDuration = 1.0f;

	// Seconds the guard stands still at the search node after the last look before giving up.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Search")
	float SearchEndWaitDuration = 5.0f;

	// ----------------------------------------------------------------------------------------------------
	// Death settings
	// ----------------------------------------------------------------------------------------------------
	
	// Seconds a dead guard's body stays in the world before it is destroyed.
	UPROPERTY(EditDefaultsOnly, Category = "AI | Death")
	float DeathLifeSpan = 10.0f;

	// ----------------------------------------------------------------------------------------------------
	// Debug settings
	// ----------------------------------------------------------------------------------------------------
	
	// Sight cone for enemy, state text above each guard
	UPROPERTY(EditAnywhere, Category = "AI | Debug")
	bool bDrawDebug = true;
	
	// ----------------------------------------------------------------------------------------------------
	// Runtime state
	// ----------------------------------------------------------------------------------------------------
	
	UPROPERTY(VisibleInstanceOnly, Category = "AI | State")
	EEnemyState CurrentState = EEnemyState::Patrol;

	UPROPERTY()
	UPathfindingSubsystem* PathfindingSubsystem = nullptr;

	/** The points the guard is walking toward. The first is the next one. */
	UPROPERTY(VisibleAnywhere, Category = "AI | State")
	TArray<FVector> CurrentPath;

	/** The player once they have been sensed. Cleared when the perception system forgets them. */
	UPROPERTY(VisibleInstanceOnly, Category = "AI | State")
	APlayerCharacter* SensedPlayer = nullptr;

	/** True while the perception system reports the player as currently in sight. */
	bool bPlayerVisible = false;

	// Patrol route state.
	TArray<FVector> PatrolRoute;								// The route as positions, built from PatrolNodes or set with SetPatrolRoute.
	int32 PatrolTargetIndex = 0;								// Index of the waypoint the guard is heading to.
	int32 PatrolDirection = 1;									// Which way along the route it is going (+1 or -1).
	bool bPatrolPathRequested = false;							// True once a path to the current waypoint has been requested.
	bool bWaitingAtEnd = false;									// True while standing still at an end of the route.
	float PatrolWaitTimer = 0.0f;								// Seconds left in that wait.
	bool bTurnBeforeWalking = false;							// Set after a wait: turn on the spot to face the way back before walking.

	// Suspicious and Alerted state.
	float SuspiciousTimer = 0.0f;								// Seconds the player has been out of sight while Suspicious.
	float LostSightTimer = 0.0f;								// Seconds the player has been out of sight while Alerted.
	bool bHoldingPosition = false;								// True while standing still to shoot.
	float RepathTimer = 0.0f;									// Seconds until the next path rebuild while chasing.

	// Search state.
	float SearchTimer = 0.0f;                                  // Seconds left before the failsafe ends the search.
	ESearchPhase SearchPhase = ESearchPhase::MovingToNode;
	bool bSearchHasNode = false;                               // False if no search node was found, so the search ends at once.
	FVector SearchNodeLocation = FVector::ZeroVector;          // The node nearest the last known position.
	TArray<FVector> SearchLookTargets;                         // Locations to look toward from that node, sorted clockwise.
	int32 SearchLookStart = 0;                                 // Which look target the sweep starts from.
	int32 SearchLooksDone = 0;                                 // How many directions have been looked at so far.
	float SearchPauseTimer = 0.0f;                             // Pause timer for the current look, then the final wait.
	
	
	// ----------------------------------------------------------------------------------------------------
	// State functions
	// ----------------------------------------------------------------------------------------------------
	
	// The only place the state changes. Logs the change and runs EnterState.
	void SetState(EEnemyState NewState);

	// One-off setup when a state begins
	void EnterState(EEnemyState NewState);
	void TickPatrol(float DeltaTime);
	void TickSuspicious(float DeltaTime);
	void TickAlerted(float DeltaTime);
	void TickSearch(float DeltaTime);
	
	// Ends the search: resets the meter and returns to Patrol.
	void EndSearch();

	// Stops the guard moving, turns it into a ragdoll and schedules it for destroy().
	void Die();

	// Sets the character's max walk speed for the given state.
	void ApplyStateSpeed(EEnemyState State);
	
	// ----------------------------------------------------------------------------------------------------
	// Movement functions
	// ----------------------------------------------------------------------------------------------------
	
	// Moves toward the next path point. Turns to face it unless bFaceMovement is false.
	void MoveAlongPath(float DeltaTime, bool bFaceMovement = true);

	// Builds an A* path to a destination, dropping leading points the guard has already passed.
	TArray<FVector> BuildPath(const FVector& Destination);

	// Turns the guard on the spot toward a location (yaw only). A Speed of 0 means use TurnSpeed.
	void FaceLocation(const FVector& Location, float DeltaTime, float Speed = 0.0f);

	// Faces the player while they are in sight, otherwise the spot they were last seen.
	void TrackPlayer(float DeltaTime);

	// ----------------------------------------------------------------------------------------------------
	// Patrol route and search functions
	// ----------------------------------------------------------------------------------------------------
	
	// Follows the patrol route back and forth, waiting at each end.
	void TickPatrolRoute(float DeltaTime);

	// Picks the next waypoint, reversing at either end, and starts the wait at an end.
	void OnReachedWaypoint();
	
	// Sorts the directions the search node connects to clockwise
	void BuildSearchLookTargets(const FVector& NodeLocation, const TArray<FVector>& ConnectedLocations);
	
	// ----------------------------------------------------------------------------------------------------
	// Perception and detection functions
	// ----------------------------------------------------------------------------------------------------
	
	UFUNCTION()
	void OnSensedActor(AActor* Actor, FAIStimulus Stimulus);
	
	UFUNCTION()
	void OnForgetActor(AActor* Actor);
	
	// Feeds the detection meter with what this guard can currently see.
	void UpdateDetection(float DeltaTime);
	
	// True while the player is in sight and the reference to them is valid.
	bool CanSeePlayer() const;

	bool OutOfAmmo();
	
	// ----------------------------------------------------------------------------------------------------
	// Debug drawing functions
	// ----------------------------------------------------------------------------------------------------
	void DrawSightCone() const;
	void DrawDebugInfo() const;
	
	

};
