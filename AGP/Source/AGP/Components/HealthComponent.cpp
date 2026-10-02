// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	MaxHealth = 100.0f;
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	
}


void UHealthComponent::OnDeath()
{
	bIsDead = true;
	UE_LOG(LogTemp, Display,TEXT("The character has died!"));
}

// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UHealthComponent::IsDead() const
{
	return bIsDead;
}

float UHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float UHealthComponent::GetCurrentHealthPercentage() const
{
	return CurrentHealth/MaxHealth;
}

void UHealthComponent::ApplyDamage(const float DamageAmount)
{
	if (bIsDead) return;
	CurrentHealth -= DamageAmount;
	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = 0.0f;
		OnDeath();
	}
}

void UHealthComponent::ApplyHealing(const float HealingAmount)
{
	if (bIsDead) return;
	CurrentHealth += HealingAmount;
	CurrentHealth = FMath::Clamp(CurrentHealth,0.0f,MaxHealth);
}

void UHealthComponent::HandleApplyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage<0)
	{
		ApplyHealing(FMath::Abs(Damage));
	}
	else
	{
		ApplyDamage(Damage);
	}
}

