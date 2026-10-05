// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"

#include "PlayerCharacter.h"
#include "AGP/Components/HealthComponent.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AISenseConfig_Sight.h"
#include "DrawDebugHelpers.h"
#include "AGP/Components/DetectionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AGP/Pathfinding/NavigationNode.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"


// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AI Perception Component"));
	DetectionComponent = CreateDefaultSubobject<UDetectionComponent>(TEXT("Detection Component"));
	
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));
	if (SightConfig)
	{
		SightConfig->SightRadius = 1500.0f;
		SightConfig->LoseSightRadius = 1500.0f;
		SightConfig->PeripheralVisionAngleDegrees = 45.0f; // Half-angle, so a 90 degree cone.
		SightConfig->SetMaxAge(1.0f);                      // Seconds before a lost stimulus is forgotten.

		// The player is team 1 and guards are team 2, so the player counts as an enemy.
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

		AIPerceptionComponent->ConfigureSense(*SightConfig);
		AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	}
	
	SensedPlayer = nullptr;
	PathingLocationThreshold = 150.0f;
	TeamID = FGenericTeamId(2);
	
	
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = false;
	}
	
	ApplyStateSpeed(CurrentState);
	
	PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>();
	if (PathfindingSubsystem)
	{
		CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
	}
	
	for (const ANavigationNode* Node : PatrolNodes)
	{
		if (Node)
		{
			PatrolRoute.Add(Node->GetActorLocation());
		}
	}
	
	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::OnSensedActor);
		AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this, &AEnemyCharacter::OnForgetActor);
	}
}

void AEnemyCharacter::TickPatrol(float DeltaTime)
{
	if (DetectionComponent)
	{
		// Full is checked first, so a player at point-blank skips Suspicious.
		if (DetectionComponent->IsMeterFull())
		{
			SetState(EEnemyState::Alerted);
			return;
		}
		if (DetectionComponent->IsAboveSuspiciousThreshold())
		{
			SetState(EEnemyState::Suspicious);
			return;
		}
	}
	
	if (PatrolRoute.Num() >= 2 && PathfindingSubsystem)
	{
		TickPatrolRoute(DeltaTime);
		return;
	}
	
	if (CurrentPath.IsEmpty())
	{
		if (PathfindingSubsystem)
		{
			CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
		}
	}
	MoveAlongPath(DeltaTime);
}

void AEnemyCharacter::TickPatrolRoute(float DeltaTime)
{
	// Waiting at an end point: stand still. This is the window for the player to get close.
	if (bWaitingAtEnd)
	{
		PatrolWaitTimer -= DeltaTime;
		if (PatrolWaitTimer <= 0.0f)
		{
			bWaitingAtEnd = false;
			bTurnBeforeWalking = true;
		}
		return;
	}
	
	// Ask A* for a path to the current waypoint.
	if (!bPatrolPathRequested)
	{
		CurrentPath = BuildPath(PatrolRoute[PatrolTargetIndex]);
		bPatrolPathRequested = true;
		
		// The path starts at the node the guard is standing on. Drop points it is already at, so the
		// turn below faces the real next point.
		while (!CurrentPath.IsEmpty()
			&& FVector::DistSquared2D(CurrentPath[0], GetActorLocation()) <= FMath::Square(PathingLocationThreshold))
		{
			CurrentPath.RemoveAt(0);
		}
	}
	
	// After a wait, turn around on the spot first, so the guard does not back away while still turning.
	if (bTurnBeforeWalking && !CurrentPath.IsEmpty())
	{
		FaceLocation(CurrentPath[0], DeltaTime);
		
		const FVector ToNext = CurrentPath[0] - GetActorLocation();
		if (ToNext.SizeSquared2D() > FMath::Square(PathingLocationThreshold)
			&& FVector::DotProduct(GetActorForwardVector(), ToNext.GetSafeNormal2D()) < 0.95f)
		{
			return;
		}
	}
	bTurnBeforeWalking = false;
	
	MoveAlongPath(DeltaTime);
	
	// MoveAlongPath removes points as it reaches them, so an empty path means we have arrived.
	if (CurrentPath.IsEmpty())
	{
		bPatrolPathRequested = false;
		OnReachedWaypoint();
	}
}

void AEnemyCharacter::OnReachedWaypoint()
{
	const int32 LastIndex = PatrolRoute.Num() - 1;
	const bool bAtEnd = PatrolTargetIndex == 0 || PatrolTargetIndex == LastIndex;
	
	// Back and forth: reverse direction at either end of the route.
	if (bAtEnd)
	{
		PatrolDirection = (PatrolTargetIndex == 0) ? 1 : -1;
	}
	PatrolTargetIndex = FMath::Clamp(PatrolTargetIndex + PatrolDirection, 0, LastIndex);
	
	// Only the end points get a wait. Middle waypoints are walked straight through.
	if (bAtEnd)
	{
		bWaitingAtEnd = true;
		PatrolWaitTimer = PatrolWaitDuration;
	}
}

void AEnemyCharacter::SetPatrolRoute(const TArray<FVector>& NewRoute)
{
	PatrolRoute = NewRoute;
	PatrolTargetIndex = 0;
	PatrolDirection = 1;
	bPatrolPathRequested = false;
	bWaitingAtEnd = false;
	bTurnBeforeWalking = false;
	CurrentPath.Empty();
}

void AEnemyCharacter::TickSuspicious(float DeltaTime)
{
	if (!DetectionComponent) return;
	
	if (DetectionComponent->IsMeterFull())
	{
		SetState(EEnemyState::Alerted);
		return;
	}
	
	// Still looking at the player resets the timer. Only time spent without sight counts.
	if (bPlayerVisible)
	{
		SuspiciousTimer = 0.0f;
	}
	else
	{
		SuspiciousTimer += DeltaTime;
	}
	
	// "Did I see that?" - no sight for long enough, so go and check the last known position.
	if (SuspiciousTimer >= SuspiciousDuration)
	{
		SetState(EEnemyState::Search);
		return;
	}
	
	if (bPlayerVisible && SensedPlayer)
	{
		FaceLocation(SensedPlayer->GetActorLocation(), DeltaTime, TrackTurnSpeed);
	}
	else if (DetectionComponent->HasLastKnownLocation())
	{
		FaceLocation(DetectionComponent->GetLastKnownLocation(), DeltaTime, TrackTurnSpeed);
	}
}

void AEnemyCharacter::TickAlerted(float DeltaTime)
{
	// Count how long the player has been out of sight. Seeing them again resets it.
	if (bPlayerVisible)
	{
		LostSightTimer = 0.0f;
	}
	else
	{
		LostSightTimer += DeltaTime;
	}
	
	if (LostSightTimer >= LoseTargetDuration)
	{
		SetState(EEnemyState::Search);
		return;
	}
	
	const bool bCanSeePlayer = bPlayerVisible && SensedPlayer;
	
	// Stand and shoot once the player is inside EngageRange. Chasing only resumes beyond the larger
	// ResumeChaseRange, so the guard does not stutter at the edge. Out of sight, always chase.
	if (bCanSeePlayer)
	{
		const float DistanceToPlayer = FVector::Dist(GetActorLocation(), SensedPlayer->GetActorLocation());
		if (bHoldingPosition)
		{
			if (DistanceToPlayer >= ResumeChaseRange)
			{
				bHoldingPosition = false;
			}
		}
		else if (DistanceToPlayer <= EngageRange)
		{
			bHoldingPosition = true;
			CurrentPath.Empty();
		}
	}
	else
	{
		bHoldingPosition = false;
	}
	
	if (!bHoldingPosition)
	{
		RepathTimer -= DeltaTime;
		
		// While the player is in sight, rebuild the path on an interval so the guard follows them. Out of
		// sight, build it once to the last known position, and stop rebuilding once the guard is there.
		if (DetectionComponent && DetectionComponent->HasLastKnownLocation() && RepathTimer <= 0.0f)
		{
			const FVector Destination = DetectionComponent->GetLastKnownLocation();
			const bool bFarFromDestination = FVector::DistSquared2D(GetActorLocation(), Destination)
				> FMath::Square(PathingLocationThreshold);
			
			if (bCanSeePlayer || (CurrentPath.IsEmpty() && bFarFromDestination))
			{
				CurrentPath = BuildPath(Destination);
				CurrentPath.Add(Destination); // End at the exact spot, not just the nearest node.
				RepathTimer = RepathInterval;
			}
		}
		MoveAlongPath(DeltaTime, false);
	}
	
	// The guard never turns to face its path while chasing. It looks at the player while they are in
	// sight, and at the spot where they were last seen once they are not.
	if (bCanSeePlayer)
	{
		FaceLocation(SensedPlayer->GetActorLocation(), DeltaTime, TrackTurnSpeed);
	}
	else if (DetectionComponent && DetectionComponent->HasLastKnownLocation())
	{
		FaceLocation(DetectionComponent->GetLastKnownLocation(), DeltaTime, TrackTurnSpeed);
	}
	
	// Shoot whenever the player is in sight, whether running or standing.
	if (bCanSeePlayer)
	{
		if (OutOfAmmo())
		{
			Reload();
		}
		else
		{
			Fire(SensedPlayer->GetActorLocation());
		}
	}
}

void AEnemyCharacter::TickSearch(float DeltaTime)
{
	if (DetectionComponent && DetectionComponent->IsMeterFull())
	{
		SetState(EEnemyState::Alerted);
		return;
	}
	
	// Seeing the player again: stop and look (Suspicious), then re-search if they are lost.
	if (bPlayerVisible)
	{
		SetState(EEnemyState::Suspicious);
		return;
	}
	
	// Failsafe: a search always ends after SearchDuration seconds, however far it has got.
	SearchTimer -= DeltaTime;
	if (SearchTimer <= 0.0f || !bSearchHasNode)
	{
		EndSearch();
		return;
	}
	
	switch (SearchPhase)
	{
	case ESearchPhase::MovingToNode:
		MoveAlongPath(DeltaTime);
		
		// MoveAlongPath removes points as it reaches them, so an empty path means the guard has arrived.
		if (CurrentPath.IsEmpty())
		{
			// Start the sweep from the direction nearest the way the guard is already facing.
			const float CurrentYaw = GetActorRotation().Yaw;
			float SmallestDifference = FLT_MAX;
			SearchLookStart = 0;
			
			for (int32 i = 0; i < SearchLookTargets.Num(); ++i)
			{
				const float TargetYaw = (SearchLookTargets[i] - GetActorLocation()).Rotation().Yaw;
				const float Difference = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw));
				if (Difference < SmallestDifference)
				{
					SmallestDifference = Difference;
					SearchLookStart = i;
				}
			}
			
			SearchLooksDone = 0;
			SearchPauseTimer = 0.0f;
			SearchPhase = ESearchPhase::LookingAround;
		}
		break;
		
	case ESearchPhase::LookingAround:
		{
			// A node with no connections has nothing to look at.
			if (SearchLookTargets.IsEmpty())
			{
				SearchPhase = ESearchPhase::FinalWait;
				SearchPauseTimer = 0.0f;
				return;
			}
			
			// Turn to the current direction. Only once the guard is actually facing it does the pause start.
			const FVector LookTarget = SearchLookTargets[(SearchLookStart + SearchLooksDone) % SearchLookTargets.Num()];
			FaceLocation(LookTarget, DeltaTime);
			
			const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(
				GetActorRotation().Yaw, (LookTarget - GetActorLocation()).Rotation().Yaw));
			
			if (YawError < 2.0f)
			{
				SearchPauseTimer += DeltaTime;
				if (SearchPauseTimer >= SearchLookPauseDuration)
				{
					SearchPauseTimer = 0.0f;
					++SearchLooksDone;
					
					// Every direction has been looked at: nothing found, so the search is over.
					if (SearchLooksDone >= SearchLookTargets.Num())
					{
						SearchPhase = ESearchPhase::FinalWait;
						SearchPauseTimer = 0.0f;
					}
				}
			}
		}
		break;
	
	case ESearchPhase::FinalWait:
		// Stand still facing the last direction for a moment, a last chance to spot the player.
		SearchPauseTimer += DeltaTime;
		if (SearchPauseTimer >= SearchEndWaitDuration)
		{
			EndSearch();
			return;
		}
		break;
	}
	
}

void AEnemyCharacter::EndSearch()
{
	// Lower the floor first, otherwise it would pull the reset meter straight back up.
	if (DetectionComponent)
	{
		DetectionComponent->SetMeterFloor(0.0f);
		DetectionComponent->ResetMeter();
	}
	SetState(EEnemyState::Patrol);
}

void AEnemyCharacter::BuildSearchLookTargets(const FVector& NodeLocation, const TArray<FVector>& ConnectedLocations)
{
	SearchLookTargets = ConnectedLocations;
	
	// Sort clockwise by the yaw of the direction from the node, so the guard sweeps around smoothly
	// instead of zig-zagging between directions.
	SearchLookTargets.Sort([&NodeLocation](const FVector& A, const FVector& B)
	{
		return FRotator::ClampAxis((A - NodeLocation).Rotation().Yaw)
			< FRotator::ClampAxis((B - NodeLocation).Rotation().Yaw);
	});
}

void AEnemyCharacter::Die()
{
	// Stop all movement, and stop the capsule blocking the player and other guards.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Ragdoll: the skeletal mesh simulates physics and collides using the Ragdoll profile.
	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		SkeletalMesh->SetCollisionProfileName(TEXT("Ragdoll"));
		SkeletalMesh->SetSimulatePhysics(true);
	}
	
	// Remove the body after a delay.
	SetLifeSpan(DeathLifeSpan);
}

void AEnemyCharacter::TickDeath()
{
	// Nothing happens on death per tick yet.
}

void AEnemyCharacter::MoveAlongPath(float DeltaTime, bool bFaceMovement)
{
	if (CurrentPath.IsEmpty()) return;
	
	FVector NextLocation = CurrentPath[0];
	FVector Direction = NextLocation - GetActorLocation();
	Direction.Normalize();
	AddMovementInput(Direction,1);
	
	if (bFaceMovement)
	{
		FaceLocation(NextLocation, DeltaTime);
	}
	
	if (FVector::DistSquared(NextLocation,GetActorLocation()) <= PathingLocationThreshold*PathingLocationThreshold)
	{
		CurrentPath.RemoveAt(0);
	}
}

TArray<FVector> AEnemyCharacter::BuildPath(const FVector& Destination)
{
	if (!PathfindingSubsystem) return TArray<FVector>();
	
	TArray<FVector> Path = PathfindingSubsystem->GetPath(GetActorLocation(), Destination);
	
	// A* starts at the node nearest the guard, which can be behind it. If the guard is already closer to
	// the second point than the first point is, the first point is behind it, so drop it.
	while (Path.Num() >= 2
		&& FVector::DistSquared(GetActorLocation(), Path[1]) < FVector::DistSquared(Path[0], Path[1]))
	{
		Path.RemoveAt(0);
	}
	
	return Path;
}

void AEnemyCharacter::OnSensedActor(AActor* Actor, FAIStimulus Stimulus)
{
	if (CurrentState == EEnemyState::Death) return;
	
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(Actor))
	{
		// This event fires when sight is gained and when it is lost.
		bPlayerVisible = Stimulus.WasSuccessfullySensed();
		
		if (bPlayerVisible)
		{
			SensedPlayer = Player;
			UE_LOG(LogTemp, Display, TEXT("Sensed Player"))
		}
	}
}

void AEnemyCharacter::OnForgetActor(AActor* Actor)
{
	if (!SensedPlayer) return;
	
	if (APlayerCharacter* Player =  Cast<APlayerCharacter>(Actor))
	{
		if (Player==SensedPlayer)
		{
			UE_LOG(LogTemp, Display, TEXT("Lost Player"))
			SensedPlayer = nullptr;
			bPlayerVisible = false;
		}
	}
}

bool AEnemyCharacter::OutOfAmmo()
{
	if (!HasWeapon()) return false;
	if (WeaponComponent->HasAmmoInMagazine()) return false;
	return true;
}

void AEnemyCharacter::DrawSightCone() const
{
#if ENABLE_DRAW_DEBUG
	if (!bDrawDebug || !SightConfig || CurrentState == EEnemyState::Death) return;

	// The eye point and direction are where the perception system looks from
	FVector EyeLocation;
	FRotator EyeRotation;
	GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// DrawDebugCone takes the half-angle in radians. The colour follows the detection meter.
	const float HalfAngleRadians = FMath::DegreesToRadians(SightConfig->PeripheralVisionAngleDegrees);
	const float Meter = DetectionComponent ? DetectionComponent->GetMeter() : 0.0f;
	const float Threshold = DetectionComponent ? DetectionComponent->GetSuspiciousThreshold() : 0.25f;
	
	FLinearColor ConeLinear = FMath::Lerp(FLinearColor::Green, FLinearColor::Yellow,
		FMath::Clamp(Meter / Threshold, 0.0f, 1.0f));
	ConeLinear = FMath::Lerp(ConeLinear, FLinearColor::Red,
		FMath::GetMappedRangeValueClamped(FVector2D(Threshold, 1.0f), FVector2D(0.0f, 1.0f), Meter));
	const FColor ConeColor = ConeLinear.ToFColor(true);

	DrawDebugCone(GetWorld(), EyeLocation, EyeRotation.Vector(), SightConfig->SightRadius, HalfAngleRadians, HalfAngleRadians, 12, ConeColor, false, -1.0f, 0, 2.0f);
#endif
}

void AEnemyCharacter::DrawDebugInfo() const
{
#if ENABLE_DRAW_DEBUG
	if (!bDrawDebug) return;

	const float Meter = DetectionComponent ? DetectionComponent->GetMeter() : 0.0f;
	const FString StateText = FString::Printf(TEXT("%s  %.2f"), *UEnum::GetDisplayValueAsText(CurrentState).ToString(), Meter);

	// Passing 'this' makes the offset relative to the guard, so the text follows it.
	// Duration 0 draws for one frame, since this is redrawn every tick.
	DrawDebugString(GetWorld(), FVector(0.0f, 0.0f, 120.0f), StateText, const_cast<AEnemyCharacter*>(this), FColor::White, 0.0f, true);

	if (DetectionComponent && DetectionComponent->HasLastKnownLocation() && CurrentState != EEnemyState::Death)
	{
		DrawDebugSphere(GetWorld(), DetectionComponent->GetLastKnownLocation(), 50.0f, 8, FColor::Cyan, false, -1.0f, 0, 1.0f);
	}
#endif
}

void AEnemyCharacter::ApplyStateSpeed(EEnemyState State)
{
	float Speed = 0.0f;
	
	switch (State)
	{
	case EEnemyState::Patrol:
		Speed = PatrolSpeed;
		break;
		
	case EEnemyState::Suspicious:
		Speed = SuspiciousSpeed;
		break;
		
	case EEnemyState::Alerted:
		Speed = AlertedSpeed;
		break;
		
	case EEnemyState::Search:
		Speed = SearchSpeed;
		break;
		
	case EEnemyState::Death:
		break;
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = Speed;
	}
}

void AEnemyCharacter::FaceLocation(const FVector& Location, float DeltaTime, float Speed)
{
	FVector ToTarget = Location - GetActorLocation();
	ToTarget.Z = 0.0f; 
	
	if (ToTarget.IsNearlyZero()) return;
	
	const float TurnRate = Speed > 0.0f ? Speed : TurnSpeed;
	
	// FixedTurn moves the yaw toward the target by at most this frame's turn, handling the 360 wrap.
	const float NewYaw = FMath::FixedTurn(GetActorRotation().Yaw, ToTarget.Rotation().Yaw, TurnRate * DeltaTime);
	SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
}

void AEnemyCharacter::SetState(EEnemyState NewState)
{
	if (NewState == CurrentState) return;
	
	ExitState(CurrentState);
	
	UE_LOG(LogTemp, Display, TEXT("%s: %s -> %s"), *GetName(),
		*UEnum::GetValueAsString(CurrentState), *UEnum::GetValueAsString(NewState));
	
	CurrentState = NewState;
	EnterState(NewState);
}

void AEnemyCharacter::EnterState(EEnemyState NewState)
{
	// A path made for the previous state is never valid for the new one.
	CurrentPath.Empty();
	ApplyStateSpeed(NewState);
	
	switch (NewState)
	{
	case EEnemyState::Patrol:
		if (DetectionComponent) DetectionComponent->SetMeterFloor(0.0f);
		// Resume the route toward the same waypoint, not partway through a wait.
		bPatrolPathRequested = false;
		bWaitingAtEnd = false;
		bTurnBeforeWalking = false;
		break;
		
	case EEnemyState::Suspicious:
		// The meter holds at the threshold while the guard works out what it saw.
		if (DetectionComponent) DetectionComponent->SetMeterFloor(DetectionComponent->GetSuspiciousThreshold());
		SuspiciousTimer = 0.0f;
		break;
		
	case EEnemyState::Alerted:
		// A floor of 1.0 holds the meter full for as long as the guard is Alerted.
		if (DetectionComponent) DetectionComponent->SetMeterFloor(1.0f);
		LostSightTimer = 0.0f;
		bHoldingPosition = false;
		RepathTimer = 0.0f; // Build a path on the first tick.
		break;
		
	case EEnemyState::Search:
		// The meter decays from full down to the suspicious threshold and holds there.
		if (DetectionComponent) DetectionComponent->SetMeterFloor(DetectionComponent->GetSuspiciousThreshold());
		SearchTimer = SearchDuration;
		
		// Plan the search: walk to the node nearest the last known position, then look down every
		// direction that node connects to.
		SearchPhase = ESearchPhase::MovingToNode;
		SearchLookTargets.Reset();
		SearchLookStart = 0;
		SearchLooksDone = 0;
		SearchPauseTimer = 0.0f;
		bSearchHasNode = false;
		
		if (PathfindingSubsystem && DetectionComponent && DetectionComponent->HasLastKnownLocation())
		{
			TArray<FVector> ConnectedLocations;
			bSearchHasNode = PathfindingSubsystem->GetNearestNodeInfo(
				DetectionComponent->GetLastKnownLocation(), SearchNodeLocation, ConnectedLocations);
			
			if (bSearchHasNode)
			{
				BuildSearchLookTargets(SearchNodeLocation, ConnectedLocations);
				CurrentPath = BuildPath(SearchNodeLocation);
				UE_LOG(LogTemp, Display, TEXT("%s search plan: %d directions to look at"), *GetName(), SearchLookTargets.Num());
			}
		}
		break;
		
	case EEnemyState::Death:
		Die();
		break;
	}
}

void AEnemyCharacter::ExitState(EEnemyState OldState)
{
	// Nothing to clean up yet
	switch (OldState)
	{
	case EEnemyState::Patrol:
	case EEnemyState::Suspicious:
	case EEnemyState::Alerted:
	case EEnemyState::Search:
	case EEnemyState::Death:
		break;
	}
}

void AEnemyCharacter::UpdateDetection(float DeltaTime)
{
	if (!DetectionComponent) return;
	
	const bool bCanSee = bPlayerVisible && SensedPlayer;
	const FVector PlayerLocation = bCanSee ? SensedPlayer->GetActorLocation() : FVector::ZeroVector;
	const float Distance = bCanSee ? FVector::Dist(GetActorLocation(), PlayerLocation) : 0.0f;
	
	DetectionComponent->UpdateDetection(DeltaTime, bCanSee, PlayerLocation, Distance);
}

float AEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Only an Alerted guard is protected. Patrol, Suspicious and Search guards take full damage.
	if (CurrentState == EEnemyState::Alerted)
	{
		DamageAmount *= AlertedDamageMultiplier;
	}
	
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	DrawSightCone();
	DrawDebugInfo();
	
	
	// Death overrides every other state, so it is checked first.
	if (HealthComponent && HealthComponent->IsDead())
	{
		SetState(EEnemyState::Death);
	}
	
	// A dead guard no longer senses or tracks the player.
	if (CurrentState != EEnemyState::Death)
	{
		UpdateDetection(DeltaTime);
	}
	
	switch (CurrentState)
	{
	case EEnemyState::Patrol:
		TickPatrol(DeltaTime);
		break;
		
	case EEnemyState::Suspicious:
		TickSuspicious(DeltaTime);
		break;
		
	case EEnemyState::Alerted:
		TickAlerted(DeltaTime);
		break;
		
	case EEnemyState::Search:
		TickSearch(DeltaTime);
		break;
		
	case EEnemyState::Death:
		TickDeath();
		break;
	}
}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

