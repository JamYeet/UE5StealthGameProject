// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponComponent.h"

#include "AGP/Characters/BaseCharacter.h"
#include "Engine/DamageEvents.h"

// Sets default values for this component's properties
UWeaponComponent::UWeaponComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	TimeSinceLastShot = 0.0f;
}

// Called when the game starts
void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// Start with a full magazine.
	RoundsRemainingInMagazine = WeaponStats.MagazineSize;
}


// Called every frame
void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastShot += DeltaTime;
	
	if (IsReloading())
	{
		ReloadTimer -= DeltaTime;
		if (ReloadTimer <= 0.0f)
		{
			EndReload();
		}
	}
}

FVector UWeaponComponent::ApplyAccuracyToShot(const FVector& BulletStart, const FVector& FireAtLocation) const
{
	FVector Direction;
	float Distance;
	(FireAtLocation - BulletStart).ToDirectionAndLength(Direction, Distance);
	FVector RandomDirection = FMath::VRand();
	FVector AccuracyAffectedDirection = FMath::Lerp(RandomDirection, Direction, WeaponStats.Accuracy);
	FVector AccuracyAffectedFireAtLocation = BulletStart + Distance*AccuracyAffectedDirection;
	return AccuracyAffectedFireAtLocation;
}

bool UWeaponComponent::Fire(const FVector& BulletStart, const FVector& FireAtLocation)
{
	if (TimeSinceLastShot < WeaponStats.FireRate) return false;
	if (!HasAmmoInMagazine()) return false;
	
	TimeSinceLastShot = 0.0f;
	RoundsRemainingInMagazine--;
	
	FHitResult HitResult;
	FCollisionQueryParams CollisionParameters;
	CollisionParameters.AddIgnoredActor(GetOwner());
	
	FVector AccuracyAffectedFireAtLocation = ApplyAccuracyToShot(BulletStart, FireAtLocation);
	
	bool bIsHit = false;
	if (GetWorld()->LineTraceSingleByChannel(
		HitResult, 
		BulletStart, 
		AccuracyAffectedFireAtLocation, 
		ECC_Pawn, 
		CollisionParameters))
	{
		bIsHit = true;
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(HitResult.GetActor()))
		{
			Character->TakeDamage(WeaponStats.BaseDamage,FDamageEvent(), Cast<ABaseCharacter>(GetOwner())->GetController(), GetOwner());
			DrawDebugLine(
				GetWorld(),
				BulletStart, HitResult.ImpactPoint,
				FColor::Green,false,
				1.0f,0,3.0f
				);
		}
		else
		{
			DrawDebugLine(
				GetWorld(),
				BulletStart, HitResult.ImpactPoint,
				FColor::Orange,false,
				1.0f,0,3.0f
				);
		}
	}
	else
	{
		DrawDebugLine(
				GetWorld(),
				BulletStart, AccuracyAffectedFireAtLocation,
				FColor::Red,false,
				1.0f,0,3.0f
				);
	}
	
	return bIsHit;
	
}

void UWeaponComponent::StartReload()
{
	if (IsReloading()) return;
	ReloadTimer = WeaponStats.ReloadTime;
}

bool UWeaponComponent::HasAmmoInMagazine() const
{
	return RoundsRemainingInMagazine > 0;
}

void UWeaponComponent::EndReload()
{
	RoundsRemainingInMagazine = WeaponStats.MagazineSize;
	ReloadTimer = 0.0f;
}

bool UWeaponComponent::IsReloading() const
{
	return ReloadTimer > 0.0f;
}

