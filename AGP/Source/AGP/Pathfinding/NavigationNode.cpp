// Fill out your copyright notice in the Description page of Project Settings.


#include "NavigationNode.h"

// Sets default values
ANavigationNode::ANavigationNode()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	LocationComponent = CreateDefaultSubobject<USceneComponent>("Location Component");
	if (LocationComponent)
	{
		SetRootComponent(LocationComponent);
	}

}

// Called when the game starts or when spawned
void ANavigationNode::BeginPlay()
{
	Super::BeginPlay();
	
	DrawDebugSphere(GetWorld(),GetActorLocation(),50.0f, 16, FColor::Blue, true);
	for (auto Node : ConnectedNodes)
	{
		if (!Node) continue;
		FColor LineColor = FColor::Red;
		float LineThickness = 2.0f;
		for (auto NeighbourNode : Node->ConnectedNodes)
		{
			if (!NeighbourNode) continue;
			if (NeighbourNode == this)
			{
				LineColor = FColor::Blue;
				LineThickness = 1.0f;
				break;
			}
		}
		DrawDebugLine(GetWorld(),GetActorLocation(),Node->GetActorLocation(),LineColor,true, -1,0,LineThickness);
	}
	
}

// Called every frame
void ANavigationNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

TArray<ANavigationNode*> ANavigationNode::GetConnectedNodes()
{
	return ConnectedNodes;
}

void ANavigationNode::AddConnectedNode(ANavigationNode* Node)
{
	if (ConnectedNodes.Contains(Node))
	{
		return;
	}
	ConnectedNodes.Add(Node);
}

