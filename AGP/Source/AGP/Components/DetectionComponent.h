// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DetectionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AGP_API UDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDetectionComponent();
	
	// Advances the meter, faster when closer, otherwise decays after a short delay.
	void UpdateDetection(float DeltaTime, bool bCanSeeTarget, const FVector& TargetLocation, float DistanceToTarget);
	
	// Sets meter back to 0.
	void ResetMeter();
	
	// The meter never drops below this.  Alerted sets it to 1.0 (meter held full), Search and Suspicious set it 
	// to the suspicious threshold (the meter decays to it and stops), Patrol uses 0.
	void SetMeterFloor(float NewFloor);
	
	// Getters
	float GetMeter() const;
	float GetSuspiciousThreshold() const;
	bool IsMeterFull() const;
	bool IsAboveSuspiciousThreshold() const;
	bool HasLastKnownLocation() const;
	FVector GetLastKnownLocation() const;

protected:
	// Meter value above this will become suspicious
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float SuspiciousThreshold = 0.50f;
	
	// Closer range
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float CloseRange = 300.0f;
	
	// Normal range
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float FarRange = 1500.0f;
	
	// Meter fills faster when close
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float CloseFillRate = 2.0f;
	
	// Meter fills normal when far
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float FarFillRate = 0.3f;
	
	// Seconds after losing sight before the meter starts to decay
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float DecayDelay = 1.0f;
	
	// Meter decay per second once the delay has passed
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float DecayRate = 0.15f;

private:	
	// Current detection level, 0 to 1.
	UPROPERTY(VisibleInstanceOnly, Category = "Detection")
	float Meter = 0.0f;
	
	// The meter never drops below this.
	UPROPERTY(VisibleInstanceOnly, Category = "Detection")
	float MeterFloor = 0.0f;

	// Seconds since the target was last in sight.
	UPROPERTY(VisibleInstanceOnly, Category = "Detection")
	float TimeSinceSeen = 0.0f;

	// Where the targetr was when it was last in sight. 
	UPROPERTY(VisibleInstanceOnly, Category = "Detection")
	FVector LastKnownLocation = FVector::ZeroVector;

	bool bHasLastKnownLocation = false;

		
};
