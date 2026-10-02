// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"

#include "AGP/Components/HealthComponent.h"
#include "AGP/Components/WeaponComponent.h"
#include "Engine/DamageEvents.h"

// Sets default values
ABaseCharacter::ABaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("Weapon Component"));
	
	BulletStartPosition = CreateDefaultSubobject<USceneComponent>(TEXT("Bullet Start Position"));
	if (BulletStartPosition)
	{
		BulletStartPosition->SetupAttachment(GetRootComponent());
	}
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health Component"));
}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (HealthComponent)
	{
		OnTakeAnyDamage.AddDynamic(HealthComponent, &UHealthComponent::HandleApplyDamage);
	}
	
}

bool ABaseCharacter::Fire(const FVector& FireAtLocation)
{
	if (!HasWeapon()) return false;
	
	return WeaponComponent->Fire(BulletStartPosition->GetComponentLocation(), FireAtLocation);
	
}

void ABaseCharacter::Reload()
{
	if (!HasWeapon()) return;
	
	WeaponComponent->StartReload();
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

bool ABaseCharacter::HasWeapon() const
{
	return bHasWeapon && WeaponComponent != nullptr;
}

void ABaseCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	this->TeamID=NewTeamID;
}

FGenericTeamId ABaseCharacter::GetGenericTeamId() const
{
	return TeamID;
}

ETeamAttitude::Type ABaseCharacter::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other))
	{
		if (GetGenericTeamId() == OtherTeamAgent->GetGenericTeamId())
		{
			return ETeamAttitude::Friendly;
		}
		return ETeamAttitude::Hostile;
	}
	return ETeamAttitude::Neutral;
}

