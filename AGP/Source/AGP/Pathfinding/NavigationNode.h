// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavigationNode.generated.h"

UCLASS()
class AGP_API ANavigationNode : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANavigationNode();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	TArray<ANavigationNode*> GetConnectedNodes();
	void AddConnectedNode(ANavigationNode* Node);
	
protected:
	UPROPERTY(EditAnywhere)
	TArray<ANavigationNode*> ConnectedNodes;
	
	UPROPERTY(VisibleAnywhere)
	USceneComponent* LocationComponent;

};
