#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FCSplineArea.generated.h"

class USplineComponent;

UCLASS(meta = (DisplayName = "Spline Area"))
class FLAGCAPTURE_API AFCSplineArea : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Spline Area", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Category = "Spline Area", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> CenterPoint;

	UPROPERTY(Category = "Spline Area", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> Spline;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> SpriteComponent;
#endif // WITH_EDITORONLY_DATA

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	float DefaultSize;

public:
	AFCSplineArea(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION(BlueprintCallable)
	virtual void CreateArea();

	UFUNCTION(BlueprintCallable)
	virtual void ClearSpline() const;

	UFUNCTION(BlueprintPure)
	TArray<FVector> GetSplinePoints() const;

	UFUNCTION(BlueprintCallable)
	virtual bool IsPontInsideArea(const FVector2D& InPoint) const;

	UFUNCTION(BlueprintCallable)
	virtual bool IsPontInsideArea3d(const FVector& InPoint) const;

protected:
	TArray<FVector2D> SplinePoints2D;

protected:
	UFUNCTION(BlueprintCallable)
	virtual void ShuffleArray(TArray<FTransform>& InArray);

};
