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
	
	if (bDebugStandStill)
	{
		GetCharacterMovement()->MaxWalkSpeed = 0.0f;
	}
	
	PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>();
	if (PathfindingSubsystem)
	{
		CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
	}
	
	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyCharacter::OnSensedActor);
		AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this, &AEnemyCharacter::OnForgetActor);
	}
}

void AEnemyCharacter::TickPatrol()
{
	if (DetectionComponent)
	{
		// Full is checked first, so a player at point blank skips Suspicious.
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
	
	if (CurrentPath.IsEmpty())
	{
		if (PathfindingSubsystem)
		{
			CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
		}
	}
	MoveAlongPath();
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
	
	// Placeholder: the guard stands still. Looking toward the sighting is step 4.
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
	
	// Chase the last known position. Repath when the current path runs out.
	if (CurrentPath.IsEmpty() && PathfindingSubsystem && DetectionComponent
		&& DetectionComponent->HasLastKnownLocation())
	{
		CurrentPath = PathfindingSubsystem->GetPath(GetActorLocation(), DetectionComponent->GetLastKnownLocation());
	}
	MoveAlongPath();
	
	// Only shoot while the player is actually in sight.
	if (bPlayerVisible && SensedPlayer)
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
	
	// Placeholder: the guard stands still until the timer runs out. Real searching is step 4.
	SearchTimer -= DeltaTime;
	if (SearchTimer <= 0.0f)
	{
		EndSearch();
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

void AEnemyCharacter::TickDeath()
{
	// Placeholder. Stopping movement and disabling collision is step 4.
}

void AEnemyCharacter::MoveAlongPath()
{
	if (CurrentPath.IsEmpty()) return;
	
	FVector NextLocation = CurrentPath[0];
	FVector Direction = NextLocation - GetActorLocation();
	Direction.Normalize();
	AddMovementInput(Direction,1);
	
	if (FVector::DistSquared(NextLocation,GetActorLocation()) <= PathingLocationThreshold*PathingLocationThreshold)
	{
		CurrentPath.RemoveAt(0);
	}
	
}

void AEnemyCharacter::OnSensedActor(AActor* Actor, FAIStimulus Stimulus)
{
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

void AEnemyCharacter::UpdateTargetLocation()
{
	if (!SensedPlayer) return;
	
	if (!AIPerceptionComponent) return;
	
	// Cache the last known player location!
	const FActorPerceptionInfo* Info = AIPerceptionComponent->GetActorInfo(*SensedPlayer);
	if (!Info) return;
	TargetLocation = Info->GetLastStimulusLocation();
	
	// Visualize the agent's target location!
	DrawDebugSphere(GetWorld(),TargetLocation, 50.0f, 4, FColor::Green, false, -1, 0, 1);
	
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
	if (!bDrawDebug || !SightConfig) return;

	// The eye point and direction are where the perception system looks from
	FVector EyeLocation;
	FRotator EyeRotation;
	GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// DrawDebugCone takes the half-angle in radians. Red when the player is sensed
	const float HalfAngleRadians = FMath::DegreesToRadians(SightConfig->PeripheralVisionAngleDegrees);
	const FColor ConeColor = SensedPlayer ? FColor::Red : FColor::Green;

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
#endif
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
	
	switch (NewState)
	{
	case EEnemyState::Patrol:
		if (DetectionComponent) DetectionComponent->SetMeterFloor(0.0f);
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
		break;
		
	case EEnemyState::Search:
		// The meter decays from full down to the suspicious threshold and holds there.
		if (DetectionComponent) DetectionComponent->SetMeterFloor(DetectionComponent->GetSuspiciousThreshold());
		SearchTimer = SearchDuration;
		break;
		
	case EEnemyState::Death:
		break;
	}
}

void AEnemyCharacter::ExitState(EEnemyState OldState)
{
	// Nothing to clean up yet. Step 3 (speeds) and step 4 (behaviours) will use this.
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

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateTargetLocation();
	DrawSightCone();
	DrawDebugInfo();
	UpdateDetection(DeltaTime);
	
		
	if (HealthComponent && HealthComponent->IsDead())
	{
		SetState(EEnemyState::Death);
	}
	
	switch (CurrentState)
	{
	case EEnemyState::Patrol:
		TickPatrol();
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

