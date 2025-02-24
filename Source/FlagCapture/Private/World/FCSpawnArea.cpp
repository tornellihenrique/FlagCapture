#include "World/FCSpawnArea.h"

#include "Components/SplineComponent.h"

AFCSpawnArea::AFCSpawnArea(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Team(ETeamSide::None)
	, CapsuleRadius(32.0f)
	, CapsuleHeight(90.0f)
	, CapsuleRotation(0.0f)
	, CapsuleRotationOffset(0.0f)
	, PaddingEdge(2.0f)
	, SpawnRows(4)
	, SpawnColumns(4)
{
	PrimaryActorTick.bCanEverTick = false;
}

#if WITH_EDITOR
void AFCSpawnArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AFCSpawnArea::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AFCSpawnArea::BeginPlay()
{
	Super::BeginPlay();
}

void AFCSpawnArea::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AFCSpawnArea::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FVector AFCSpawnArea::SnapToGround(const FVector& OriginLocation) const
{
	const float TraceZOffsets = 50000.0f;
	const float ZOffsets = 10.0f;
	FVector UpVector = FRotationMatrix(GetActorRotation()).GetScaledAxis(EAxis::Z) * TraceZOffsets;

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	CollisionParams.bReturnPhysicalMaterial = false;

	bool HitTheGround = false;
	if (GetWorld()->LineTraceSingleByChannel(HitResult, OriginLocation + UpVector, OriginLocation - UpVector, ECC_WorldStatic, CollisionParams))
	{
		HitTheGround = HitResult.bBlockingHit;
	}

	if (HitTheGround)
	{
		FVector TagetLocation = OriginLocation;
		TagetLocation.Z = HitResult.Location.Z + ZOffsets + CapsuleHeight;

		return TagetLocation;
	}

	return FVector(OriginLocation.X, OriginLocation.Y, OriginLocation.Z + ZOffsets);
}

bool AFCSpawnArea::IsLocationInsideArea(const FVector& InTargetLocation) const
{
	return IsPontInsideArea(FVector2D(InTargetLocation.X, InTargetLocation.Y));
}

bool AFCSpawnArea::IsSpawnPointsAvailable() const
{
	if (SpawnPointTransforms.Num() == 0) return false;

	TArray<FTransform> FilteredSpawnPointTransforms = SpawnPointTransforms.FilterByPredicate([this](const FTransform& SpawnPoint)
	{
		const FVector& TaregtLocation = GetRootComponent()->GetComponentLocation() + SpawnPoint.GetLocation();
		return IsLocationInsideArea(TaregtLocation) && CanSpawnAtLocation(TaregtLocation);
	});

	return FilteredSpawnPointTransforms.Num() != 0;
}

bool AFCSpawnArea::CanSpawnAtLocation(const FVector& InTargetLocation) const
{
	FCollisionQueryParams Params;
	Params.bTraceComplex = true;

	Params.AddIgnoredActor(this);

	FHitResult OutHit;
	if (GetWorld()->SweepSingleByChannel(OutHit, InTargetLocation, InTargetLocation + 0.0001f, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHeight), Params))
	{
		return false;
	}

	return true;
}

FTransform AFCSpawnArea::GetSafeSpawnTransform()
{
	TArray<FTransform> FilteredSpawnPointTransforms = SpawnPointTransforms.FilterByPredicate([this](const FTransform& SpawnPoint)
	{
		const FVector& TaregtLocation = GetRootComponent()->GetComponentLocation() + SpawnPoint.GetLocation();
		return IsLocationInsideArea(TaregtLocation);
	});

	FTransform SelectedSpawnPoint = CenterPoint ? CenterPoint->GetComponentTransform() : GetActorTransform();

	if (FilteredSpawnPointTransforms.Num() != 0)
	{
		ShuffleArray(FilteredSpawnPointTransforms);

		for (int32 i = 0; i < FilteredSpawnPointTransforms.Num(); i++)
		{
			FTransform TargetSelectedSpawnPoint = FilteredSpawnPointTransforms[i];
			TargetSelectedSpawnPoint.SetLocation(GetRootComponent()->GetComponentLocation() + TargetSelectedSpawnPoint.GetLocation());

			if (CanSpawnAtLocation(TargetSelectedSpawnPoint.GetLocation()))
			{
				SelectedSpawnPoint = TargetSelectedSpawnPoint;
				break;
			}
		}
	}

	return SelectedSpawnPoint;
}

bool AFCSpawnArea::IsPointInSplineArea(const FVector& Point, const TArray<FVector>& ControlPoints) const
{
	bool bInside = false;
	int32 NumPoints = ControlPoints.Num();

	for (int32 i = 0, j = NumPoints - 1; i < NumPoints; j = i++)
	{
		FVector Vertex1 = ControlPoints[i];
		FVector Vertex2 = ControlPoints[j];

		if (((Vertex1.Y > Point.Y) != (Vertex2.Y > Point.Y)) && (Point.X < (Vertex2.X - Vertex1.X) * (Point.Y - Vertex1.Y) / (Vertex2.Y - Vertex1.Y) + Vertex1.X))
		{
			bInside = !bInside;
		}
	}

	FBox BoundingBox = FBox(ControlPoints);
	bool bInsideBoundingBox = BoundingBox.IsInsideXY(Point);

	return bInside && bInsideBoundingBox;
}

FVector AFCSpawnArea::CalculateSplineAreaCenter(const TArray<FVector>& ControlPoints) const
{
	FTransform LocalToWorld = GetRootComponent()->GetComponentTransform();
	FVector Center = FVector::ZeroVector;
	int32 NumPoints = ControlPoints.Num();

	for (const FVector& Point : ControlPoints)
	{
		Center += LocalToWorld.TransformPosition(Point);
	}

	if (NumPoints > 0)
	{
		Center /= NumPoints;
		Center = LocalToWorld.InverseTransformPosition(Center);
	}

	return Center;
}

void AFCSpawnArea::InitializeSpawnPoints()
{
	const FVector& RootLocation = GetRootComponent()->GetComponentLocation();

	const FBox SplineBounds = Spline->Bounds.GetBox();

	const FVector BoxExtent = SplineBounds.GetExtent();
	const float BoxWidth = BoxExtent.X * 2.0f;
	const float BoxDepth = BoxExtent.Y * 2.0f;

	const float AvailableWidth = BoxWidth - 2.0f * PaddingEdge;
	const float AvailableDepth = BoxDepth - 2.0f * PaddingEdge;

	const float RowStep = AvailableDepth / SpawnRows;
	const float ColumnStep = AvailableWidth / SpawnColumns;

	const int32 InstanceCount = Spline->GetNumberOfSplinePoints();
	TArray<FVector> ControlPoints;
	for (int32 i = 0; i < InstanceCount; i++)
	{
		FVector ControlPoint = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		ControlPoint.Z = 0.0f;
		ControlPoints.Add(ControlPoint);
	}

	SpawnPointTransforms.Empty();

	for (int32 RowIndex = 0; RowIndex < SpawnRows; RowIndex++)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < SpawnColumns; ColumnIndex++)
		{
			int32 Index = ColumnIndex + (RowIndex * SpawnColumns) + (SpawnColumns * SpawnRows);
			FQuat AQuat = FQuat(FRotator(0.0f, CapsuleRotationOffset, 0.0f) * Index);
			FQuat BQuat = FQuat(FRotator(0.0f, CapsuleRotation, 0.0f));
			FRotator GridRotaion(BQuat * AQuat);

			const float X = SplineBounds.Min.X + PaddingEdge + ColumnIndex * ColumnStep + ColumnStep * 0.5f;
			const float Y = SplineBounds.Min.Y + PaddingEdge + RowIndex * RowStep + RowStep * 0.5f;
			const FVector GridPoint = SnapToGround(FVector(X, Y, SplineBounds.Min.Z + CapsuleHeight));

			if (IsPointInSplineArea(FVector(X, Y, 0.0f), ControlPoints))
			{
				SpawnPointTransforms.Add(FTransform(GridRotaion, GridPoint - RootLocation, FVector::OneVector));
			}
		}
	}

	FVector CenterPointLocation = CalculateSplineAreaCenter(ControlPoints);
	CenterPointLocation.Z = RootLocation.Z;

	ShowDebugVisual();

	const FVector& TargetCenter = CenterPointLocation - RootLocation;
	CenterPoint->SetRelativeLocation(TargetCenter);

	Modify(true);
}

void AFCSpawnArea::ShowDebugVisual()
{
	const FVector& RootLocation = GetRootComponent()->GetComponentLocation();

	FlushPersistentDebugLines(GetWorld());

	for (const FTransform& Point : SpawnPointTransforms)
	{
		const FVector& PointLocation = RootLocation + Point.GetLocation();
		const FRotator& PointRotation = Point.GetRotation().Rotator();
		const FColor CapsuleColor = CanSpawnAtLocation(PointLocation) ? FColor::Green : FColor::Red;

		DrawDebugCapsule(GetWorld(), PointLocation, CapsuleHeight, CapsuleRadius, FQuat::Identity, CapsuleColor, false, 10.0f, 0, 1.0f);
		DrawDebugDirectionalArrow(GetWorld(), PointLocation, PointLocation + (PointRotation.Vector() * 40.0f), 10.0f, FColor::Red, false, 10.0f, 0, 1.0f);
	}
}
