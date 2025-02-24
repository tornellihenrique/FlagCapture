#pragma once

#include "CoreMinimal.h"
#include "World/FCSplineArea.h"
#include "FCTypes.h"
#include "FCSpawnArea.generated.h"

UCLASS(meta = (DisplayName = "Spawn Area"))
class FLAGCAPTURE_API AFCSpawnArea : public AFCSplineArea
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	ETeamSide Team;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	float CapsuleRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	float CapsuleHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	float CapsuleRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	float CapsuleRotationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	float PaddingEdge;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	int32 SpawnRows;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	int32 SpawnColumns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs", meta = (ExposeOnSpawn = true))
	TArray<FTransform> SpawnPointTransforms;

public:
	AFCSpawnArea(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual FVector SnapToGround(const FVector& OriginLocation) const;

public:
	UFUNCTION(BlueprintPure)
	virtual ETeamSide OwnerTeam() const { return Team; }

	UFUNCTION(BlueprintPure)
	virtual bool CanTeamSpawn(ETeamSide InTeam) const { return Team == InTeam; }

	UFUNCTION(BlueprintPure)
	virtual bool IsLocationInsideArea(const FVector& InTargetLocation) const;

	UFUNCTION(BlueprintPure)
	virtual bool IsSpawnPointsAvailable() const;

	UFUNCTION(BlueprintPure)
	virtual bool CanSpawnAtLocation(const FVector& InTargetLocation) const;

	UFUNCTION(BlueprintPure)
	virtual FTransform GetSafeSpawnTransform();

	UFUNCTION(BlueprintPure)
	virtual bool IsPointInSplineArea(const FVector& Point, const TArray<FVector>& ControlPoints) const;

	UFUNCTION(BlueprintPure)
	virtual FVector CalculateSplineAreaCenter(const TArray<FVector>& ControlPoints) const;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Configs")
	virtual void InitializeSpawnPoints();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Configs")
	virtual void ShowDebugVisual();

};
