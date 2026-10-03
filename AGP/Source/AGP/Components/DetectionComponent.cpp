// Fill out your copyright notice in the Description page of Project Settings.


#include "DetectionComponent.h"

UDetectionComponent::UDetectionComponent()
{
	// The owning guard drives this component, so it does not need its own tick.
	PrimaryComponentTick.bCanEverTick = false;
}

void UDetectionComponent::UpdateDetection(float DeltaTime, bool bCanSeeTarget, const FVector& TargetLocation, float DistanceToTarget)
{
	if (bCanSeeTarget)
	{
		TimeSinceSeen = 0.0f;
		LastKnownLocation = TargetLocation;
		bHasLastKnownLocation = true;

		// Blend between the close and far fill rates by distance (clamped outside the range).
		const float FillRate = FMath::GetMappedRangeValueClamped(
			FVector2D(CloseRange, FarRange), FVector2D(CloseFillRate, FarFillRate), DistanceToTarget);
		Meter += FillRate * DeltaTime;
	}
	else
	{
		TimeSinceSeen += DeltaTime;

		// The delay stops a brief break in sight from draining the meter straight away.
		if (TimeSinceSeen >= DecayDelay)
		{
			Meter -= DecayRate * DeltaTime;
		}
	}

	Meter = FMath::Clamp(Meter, MeterFloor, 1.0f);
}

void UDetectionComponent::ResetMeter()
{
	Meter = 0.0f;
	TimeSinceSeen = 0.0f;
}

void UDetectionComponent::SetMeterFloor(float NewFloor)
{
	MeterFloor = FMath::Clamp(NewFloor, 0.0f, 1.0f);
	Meter = FMath::Max(Meter, MeterFloor);
}

float UDetectionComponent::GetMeter() const
{
	return Meter;
}

float UDetectionComponent::GetSuspiciousThreshold() const
{
	return SuspiciousThreshold;
}

bool UDetectionComponent::IsMeterFull() const
{
	return Meter >= 1.0f;
}

bool UDetectionComponent::IsAboveSuspiciousThreshold() const
{
	return Meter > SuspiciousThreshold;
}

float UDetectionComponent::GetTimeSinceSeen() const
{
	return TimeSinceSeen;
}

bool UDetectionComponent::HasLastKnownLocation() const
{
	return bHasLastKnownLocation;
}

FVector UDetectionComponent::GetLastKnownLocation() const
{
	return LastKnownLocation;
}