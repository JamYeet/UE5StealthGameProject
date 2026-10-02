// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

/**
 * Fixed stats shared by every armed character. Tune these in the editor rather than in code.
 */
USTRUCT(BlueprintType)
struct FWeaponStats
{
	GENERATED_BODY()
	
public:
	/** 0 = fully random direction, 1 = perfectly accurate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Accuracy = 0.95f;
	
	/** Minimum seconds between shots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FireRate = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BaseDamage = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MagazineSize = 10;
	
	/** Seconds taken to reload. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ReloadTime = 2.0f;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AGP_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponComponent();
	
	// Called when the game starts
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere)
	FWeaponStats WeaponStats;
	int32 RoundsRemainingInMagazine = 0;
	float TimeSinceLastShot = 0.0f;
	float ReloadTimer = 0.0f;
	
	void EndReload();
	bool IsReloading() const;
	

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FVector ApplyAccuracyToShot(const FVector& BulletStart, const FVector& FireAtLocation) const;

	bool Fire(const FVector& BulletStart, const FVector& FireAtLocation);

	void StartReload();
	
	bool HasAmmoInMagazine() const;
	
};
