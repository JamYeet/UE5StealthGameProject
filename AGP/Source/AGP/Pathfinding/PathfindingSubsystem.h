// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PathfindingSubsystem.generated.h"

struct FAStarNodeData;
class ANavigationNode;

UCLASS()
class AGP_API UPathfindingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void OnWorldBeginPlay(UWorld& World) override;
	
	TArray<FVector> GetRandomPath(const FVector& StartLocation);
	
	TArray<FVector> GetPath(const FVector& StartLocation, const FVector& TargetLocation);
	
	TArray<FVector> GetPathAway(const FVector& StartLocation, const FVector& TargetLocation);
	
	void PlaceProceduralNodes(const TArray<FVector>& Vertices, const int32 Width, const int32 Height, AActor* ParentActor);
	
	TArray<FVector> GetWaypointPositions();
	
	// Finds the node nearest a location and returns its position and the positions of the nodes it
	// connects to. Returns false if there are no nodes.
	bool GetNearestNodeInfo(const FVector& Location, FVector& OutNodeLocation, TArray<FVector>& OutConnectedLocations);
	
protected:
	UPROPERTY()
	TArray<ANavigationNode*> Nodes;
	
protected:
	void PopulateNodes();
	void RemoveAllNodes();
	
private:
	ANavigationNode* GetRandomNode();
	
	ANavigationNode* FindNearestNode(const FVector& TargetLocation);
	ANavigationNode* FindFurthestNode(const FVector& TargetLocation);
	
	TArray<FVector> GetPath(ANavigationNode* StartNode, ANavigationNode* EndNode);
	
	TArray<FVector> ReconstructPath(const TMap<ANavigationNode*, FAStarNodeData>& NodeMap, ANavigationNode* EndNode);
	
	void DrawDebugPath(TArray<FVector> Path) const;
	
};
