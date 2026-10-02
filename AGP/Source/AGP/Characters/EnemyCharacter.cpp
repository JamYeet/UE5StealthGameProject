// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"

#include "PlayerCharacter.h"
#include "AGP/Components/HealthComponent.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AI Perception Component"));
	SensedPlayer = nullptr;
	PathingLocationThreshold = 150.0f;
	EvadeHealthPercentageThreshold = 0.4f;
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
	if (CurrentPath.IsEmpty())
	{
		if (PathfindingSubsystem)
		{
			CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
		}
	}
	MoveAlongPath();
}

void AEnemyCharacter::TickEngage()
{
	if (CurrentPath.IsEmpty())
	{
		if (PathfindingSubsystem && SensedPlayer)
		{
			// CurrentPath = PathfindingSubsystem->GetPath(GetActorLocation(),SensedCharacter->GetActorLocation());
			CurrentPath = PathfindingSubsystem->GetPath(GetActorLocation(),TargetLocation);
		}
	}
	MoveAlongPath();
	
	if (SensedPlayer)
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

void AEnemyCharacter::TickEvade()
{
	if (CurrentPath.IsEmpty())
	{
		if (PathfindingSubsystem && SensedPlayer)
		{
			CurrentPath = PathfindingSubsystem->GetPathAway(GetActorLocation(),TargetLocation);
		}
	}
	MoveAlongPath();
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
	const FActorPerceptionInfo* Info = AIPerceptionComponent->GetActorInfo(*SensedPlayer);;
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

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateTargetLocation();

	switch (CurrentState)
	{
	case EEnemyState::Patrol:
		if (SensedPlayer)
		{
			if (HealthComponent->GetCurrentHealthPercentage()>=EvadeHealthPercentageThreshold)
			{
				CurrentPath.Empty();
				CurrentState = EEnemyState::Engage;
				TickEngage();
				break;
			}
			CurrentPath.Empty();
			CurrentState = EEnemyState::Evade;
			TickEvade();
			break;
		}
		TickPatrol();
		break;
		
	case EEnemyState::Engage:
		if (!SensedPlayer)
		{
			CurrentState = EEnemyState::Patrol;
			TickPatrol();
			break;
		}
		if (HealthComponent->GetCurrentHealthPercentage()<EvadeHealthPercentageThreshold)
		{
			CurrentPath.Empty();
			CurrentState = EEnemyState::Evade;
			TickEvade();
			break;
		}
		TickEngage();
		break;
		
	case EEnemyState::Evade:
		if (!SensedPlayer)
		{
			CurrentState = EEnemyState::Patrol;
			TickPatrol();
			break;
		}
		if (HealthComponent->GetCurrentHealthPercentage()>=EvadeHealthPercentageThreshold)
		{
			CurrentPath.Empty();
			CurrentState = EEnemyState::Engage;
			TickEngage();
			break;
		}
		TickEvade();
		break;
	}
}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

