// Fill out your copyright notice in the Description page of Project Settings.


#include "PathfindingSubsystem.h"
#include "NavigationNode.h"
#include "EngineUtils.h"
#include "Algo/Reverse.h"

// Per-search scratch data for A*. A plain struct (not a USTRUCT) as it never outlives a single search.
struct FAStarNodeData
{
	float GScore = MAX_FLT;
	float HScore = 0;
	float GetFScore() const { return GScore + HScore; }
	
	ANavigationNode* ParentNode = nullptr;
};


void UPathfindingSubsystem::OnWorldBeginPlay(UWorld& World)
{
	Super::OnWorldBeginPlay(World);	
	
	PopulateNodes();
}

TArray<FVector> UPathfindingSubsystem::GetRandomPath(const FVector& StartLocation)
{
	return GetPath(FindNearestNode(StartLocation), GetRandomNode());
}

TArray<FVector> UPathfindingSubsystem::GetPath(const FVector& StartLocation, const FVector& TargetLocation)
{
	return GetPath(FindNearestNode(StartLocation), FindNearestNode(TargetLocation));
}

TArray<FVector> UPathfindingSubsystem::GetPathAway(const FVector& StartLocation, const FVector& TargetLocation)
{
	return GetPath(FindNearestNode(StartLocation), FindFurthestNode(TargetLocation));
}

void UPathfindingSubsystem::PlaceProceduralNodes(const TArray<FVector>& Vertices, const int32 Width, const int32 Height, AActor* ParentActor)
{
	//Clear nodes
	RemoveAllNodes();

	// Spawn Nodes and add to the Nodes array.
	for (int i = 0; i < Width; ++i)
	{
		for (int j = 0; j < Height; ++j)
		{
			FVector Position = Vertices[ i + j * Width];
			if (ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>(Position,FRotator::ZeroRotator))
			{
				Nodes.Add(Node);
				
				// Attach to supplied parent actor to nest nodes to mesh and keep them self-contained.
				if (ParentActor)
				{
					Node->AttachToActor(ParentActor,FAttachmentTransformRules::KeepWorldTransform);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Unable to spawn a node for some reason. This is bad!"))
			}
		}
	}
	
	// Connect Nodes to their neighbours.
	for (int i = 0; i < Width; ++i)
	{
		for (int j = 0; j < Height; ++j)
		{
			ANavigationNode* CurrentNode = Nodes[i + j * Width];
			
			if (!CurrentNode) continue;

			// conditions for valid neighbours.
			const bool bAddLeft = i < Width-1;
			const bool bAddRight = i > 0;
			const bool bAddUp = j < Height -1;
			const bool bAddDown = j > 0;
			
			if (bAddUp)
			{
				CurrentNode->AddConnectedNode(Nodes[i + (j+1) * Width]);
			}
			if (bAddDown)
			{
				CurrentNode->AddConnectedNode(Nodes[i + (j-1) * Width]);
			}
			if (bAddLeft)
			{
				CurrentNode->AddConnectedNode(Nodes[(i+1) + j * Width]);
			}
			if (bAddRight)
			{
				CurrentNode->AddConnectedNode(Nodes[(i-1) + j * Width]);
			}
			if (bAddUp && bAddRight)
			{
				CurrentNode->AddConnectedNode(Nodes[(i-1) + (j+1) * Width]);
			}
			if (bAddUp && bAddLeft)
			{
				CurrentNode->AddConnectedNode(Nodes[(i+1) + (j+1) * Width]);
			}
			if (bAddDown && bAddRight)
			{
				CurrentNode->AddConnectedNode(Nodes[(i-1) + (j-1) * Width]);
			}
			if (bAddDown && bAddLeft)
			{
				CurrentNode->AddConnectedNode(Nodes[(i+1) + (j-1) * Width]);
			}
		}
	}
}

TArray<FVector> UPathfindingSubsystem::GetWaypointPositions()
{
	TArray<FVector> WaypointPositions;
	for (const ANavigationNode* Node : Nodes)
	{
		if (!Node) continue;
		WaypointPositions.Add(Node->GetActorLocation());
	}
	return WaypointPositions;

}

void UPathfindingSubsystem::PopulateNodes()
{
	Nodes.Empty();

	for (TActorIterator<ANavigationNode> It(GetWorld()); It; ++It)
	{
		if (ANavigationNode* Node = *It)
		{
			Nodes.Add(Node);
			UE_LOG(LogTemp, Warning, TEXT("Node at : %s"), *Node->GetActorLocation().ToString());
		}
	}
}

void UPathfindingSubsystem::RemoveAllNodes()
{
	// Destroy all Nodes in the world.
	for (TActorIterator<ANavigationNode> It(GetWorld()); It; ++It)
	{
		if (ANavigationNode* Node = *It)
		{
			Node->Destroy(true);
		}
	}
	// Clear out the list of nodes as pointers are now null.
	Nodes.Empty();
}

ANavigationNode* UPathfindingSubsystem::GetRandomNode()
{
	if (Nodes.IsEmpty())
	{
		return nullptr;
	}
	
	return Nodes[FMath::RandRange(0, Nodes.Num() - 1)];
}

ANavigationNode* UPathfindingSubsystem::FindNearestNode(const FVector& TargetLocation)
{
	if (Nodes.IsEmpty())
	{
		return nullptr;
	}
	
	float ClosestDistance = FLT_MAX;
	ANavigationNode* ClosestNode = nullptr;
	
	for (ANavigationNode* Node : Nodes)
	{
		if (!Node) continue;
		if (const float Distance = FVector::Distance(Node->GetActorLocation(), TargetLocation); Distance<ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestNode = Node;
		}
	}
	return ClosestNode;
	
}

ANavigationNode* UPathfindingSubsystem::FindFurthestNode(const FVector& TargetLocation)
{
	if (Nodes.IsEmpty())
	{
		return nullptr;
	}
	
	float FurthestDistance = -FLT_MAX;
	ANavigationNode* FurthestNode = nullptr;
	
	for (ANavigationNode* Node : Nodes)
	{
		if (!Node) continue;
		if (const float Distance = FVector::Distance(Node->GetActorLocation(), TargetLocation); Distance>FurthestDistance)
		{
			FurthestDistance = Distance;
			FurthestNode = Node;
		}
	}
	return FurthestNode;
}

TArray<FVector> UPathfindingSubsystem::GetPath(ANavigationNode* StartNode, ANavigationNode* EndNode)
{
	if (!StartNode || !EndNode)
	{
		return TArray<FVector>();
	}
	
	TArray<ANavigationNode*> OpenSet;
	TMap<ANavigationNode*, FAStarNodeData> NodeMap;
	
	for (ANavigationNode* Node : Nodes)
	{
		if (!Node) continue;
		NodeMap.Add(Node, FAStarNodeData());
	}
	
	if (!NodeMap.Contains(StartNode) || !NodeMap.Contains(EndNode))
	{
		return TArray<FVector>();
	}
	
	NodeMap[StartNode].GScore = 0;
	NodeMap[StartNode].HScore = FVector::DistSquared(StartNode->GetActorLocation(), EndNode->GetActorLocation());
	
	OpenSet.Add(StartNode);
	
	while (!OpenSet.IsEmpty())
	{
		// Get the lowest-score node (most promising) to search around.
		// This process could be more efficient using a Min-Heap approach to OpenSet, but it is more difficult.
		float LowestFScore = FLT_MAX;
		ANavigationNode* CurrentNode = nullptr;
		
		for (ANavigationNode* Node : OpenSet)
		{
			if (!Node) continue;
			if (NodeMap[Node].GetFScore() < LowestFScore)
			{
				LowestFScore = NodeMap[Node].GetFScore();
				CurrentNode = Node;
			}
		}
		
		if (!CurrentNode)
		{
			UE_LOG(LogTemp, Error, TEXT("No valid current node found in non-empty open set."))
			return TArray<FVector>();
		}
		
		OpenSet.Remove(CurrentNode);
		
		// return the path as we have reached the end-node!
		if (CurrentNode == EndNode)
		{
			return ReconstructPath(NodeMap, EndNode);
		}
		
		// Update values of surrounding nodes, and if improved, add to open set. 
		for (ANavigationNode* ConnectedNode : CurrentNode->GetConnectedNodes())
		{
			if (!ConnectedNode) continue;

			const float TentativeConnectedNodeGScore = NodeMap[CurrentNode].GScore + 
				FVector::DistSquared(CurrentNode->GetActorLocation(), ConnectedNode->GetActorLocation());
			
			if (TentativeConnectedNodeGScore < NodeMap[ConnectedNode].GScore)
			{
				NodeMap[ConnectedNode].ParentNode = CurrentNode;
				NodeMap[ConnectedNode].GScore = TentativeConnectedNodeGScore;
				NodeMap[ConnectedNode].HScore = FVector::DistSquared(ConnectedNode->GetActorLocation(), EndNode->GetActorLocation());
				
				if (!OpenSet.Contains(ConnectedNode))
				{
					OpenSet.Add(ConnectedNode);
				}
				
			}
		}
	}
	
	// If it all fails, return no path.
	UE_LOG(LogTemp, Error, TEXT("No valid path found!"))
	return TArray<FVector>();
}



void UPathfindingSubsystem::DrawDebugPath(TArray<FVector> Path) const
{
	if (!Path.IsEmpty())
	{
		
		DrawDebugBox(GetWorld(), Path[0],FVector(50, 50, 50), FColor::Emerald, false, 5, 0, 5);
		DrawDebugBox(GetWorld(), Path.Top(),FVector(50, 50, 75), FColor::Green, false, 5, 0, 5);
		
		FVector LastPosition = Path[0];
		for (FVector Position:Path)
		{
			if (Position != LastPosition)
			{
				DrawDebugLine(GetWorld(), LastPosition, Position, FColor::Green, false, 5, 0, 5);
				DrawDebugDirectionalArrow(GetWorld(), LastPosition, Position,1000, FColor::Green, false, 5, 0, 5);
			}
			LastPosition = Position;
		}
	}
}

TArray<FVector> UPathfindingSubsystem::ReconstructPath(const TMap<ANavigationNode*, FAStarNodeData>& NodeMap,
	ANavigationNode* EndNode)
{
	TArray<FVector> Path = TArray<FVector>();
	Path.Add(EndNode->GetActorLocation());
	ANavigationNode* NextNode = NodeMap[EndNode].ParentNode;
	while (NextNode)
	{
		Path.Add(NextNode->GetActorLocation());
		NextNode = NodeMap[NextNode].ParentNode;
	}
			
	// Reverse Path as currently end --> start.
	Algo::Reverse(Path);
	
	DrawDebugPath(Path);
			
	return Path;
}
