// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"

#include "PlayerCharacter.h"
#include "AGP/Components/HealthComponent.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AISenseConfig_Sight.h"
#include "DrawDebugHelpers.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AI Perception Component"));
	
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
	// TEMPORARY: until the detection meter exists (step 2), seeing the player means Alerted.
	if (SensedPlayer)
	{
		SetState(EEnemyState::Alerted);
		return;
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

void AEnemyCharacter::TickSuspicious()
{
	// Placeholder. Reached from the detection meter in step 2, behaviour in step 4.
}

void AEnemyCharacter::TickAlerted()
{
	// TEMPORARY: losing sight of the player means Search.
	if (!SensedPlayer)
	{
		SetState(EEnemyState::Search);
		return;
	}
	
	if (CurrentPath.IsEmpty())
	{
		if (PathfindingSubsystem)
		{
			CurrentPath = PathfindingSubsystem->GetPath(GetActorLocation(), TargetLocation);
		}
	}
	MoveAlongPath();
	
	if (OutOfAmmo())
	{
		Reload();
	}
	else
	{
		Fire(SensedPlayer->GetActorLocation());
	}
}

void AEnemyCharacter::TickSearch(float DeltaTime)
{
	// Seeing the player again puts the guard straight back to Alerted.
	if (SensedPlayer)
	{
		SetState(EEnemyState::Alerted);
		return;
	}
	
	// Placeholder: the guard stands still until the timer runs out. Real searching is step 4.
	SearchTimer -= DeltaTime;
	if (SearchTimer <= 0.0f)
	{
		SetState(EEnemyState::Patrol);
	}
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
	if (SensedPlayer) return;
	
	if (APlayerCharacter* Player =  Cast<APlayerCharacter>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed())
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

	const FString StateText = UEnum::GetDisplayValueAsText(CurrentState).ToString();

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
		break;
		
	case EEnemyState::Suspicious:
		break;
		
	case EEnemyState::Alerted:
		break;
		
	case EEnemyState::Search:
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

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateTargetLocation();
	DrawSightCone();
	DrawDebugInfo();
		
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
		TickSuspicious();
		break;
		
	case EEnemyState::Alerted:
		TickAlerted();
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

