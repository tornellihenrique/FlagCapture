#include "World/FCSplineArea.h"

#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"

AFCSplineArea::AFCSplineArea(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DefaultSize(500.0f)
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	CenterPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CenterPoint"));
	CenterPoint->SetupAttachment(GetRootComponent());
	CenterPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f));

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetupAttachment(GetRootComponent());

#if WITH_EDITOR
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		if (!IsRunningCommandlet())
		{
			struct FConstructorStatics
			{
				ConstructorHelpers::FObjectFinderOptional<UTexture2D> SplineAreaTexture;
				FName ID_SplineArea;
				FText NAME_SplineArea;
				FConstructorStatics()
					: SplineAreaTexture(TEXT("/Engine/EditorResources/S_Player.S_Player"))
					, ID_SplineArea(TEXT("Spline Area"))
					, NAME_SplineArea(NSLOCTEXT("SpriteCategory", "SplineArea", "Spline Area"))
				{
				}
			};
			static FConstructorStatics ConstructorStatics;

			SpriteComponent->Sprite = ConstructorStatics.SplineAreaTexture.Get();

#if WITH_EDITORONLY_DATA
			SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_SplineArea;
			SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_SplineArea;
#endif
		}

		SpriteComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
		SpriteComponent->SetupAttachment(CenterPoint);
		SpriteComponent->bIsScreenSizeScaled = true;
	}
#endif

	PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
}

#if WITH_EDITOR
void AFCSplineArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SetActorRotation(FRotator::ZeroRotator);

	CreateArea();
}

void AFCSplineArea::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AFCSplineArea, DefaultSize))
	{
		ClearSpline();

		CreateArea();
	}
}
#endif

void AFCSplineArea::BeginPlay()
{
	for (int32 i = 0; i < Spline->GetNumberOfSplinePoints(); i++)
	{
		const FVector& SplinePointLocation = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		SplinePoints2D.Add(FVector2D(SplinePointLocation.X, SplinePointLocation.Y));
	}

	Super::BeginPlay();
}

void AFCSplineArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AFCSplineArea::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AFCSplineArea::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFCSplineArea::CreateArea()
{
	for (int32 i = 0; i < Spline->GetNumberOfSplinePoints(); i++)
	{
		Spline->SetSplinePointType(i, ESplinePointType::Linear, false);

		const FVector& TargetCorection = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		Spline->SetLocationAtSplinePoint(i, TargetCorection, ESplineCoordinateSpace::World);
	}
}

void AFCSplineArea::ClearSpline() const
{
	Spline->ClearSplinePoints(true);

	TArray<FSplinePoint> SplinePoints;
	SplinePoints.Add(FSplinePoint(0, FVector(-DefaultSize, -DefaultSize, 0.f)));
	SplinePoints.Add(FSplinePoint(1, FVector(DefaultSize, -DefaultSize, 0.f)));
	SplinePoints.Add(FSplinePoint(2, FVector(DefaultSize, DefaultSize, 0.f)));
	SplinePoints.Add(FSplinePoint(3, FVector(-DefaultSize, DefaultSize, 0.f)));

	Spline->AddPoints(SplinePoints, false);
	for (int32 i = 0; i < Spline->GetNumberOfSplinePoints(); i++)
	{
		Spline->SetSplinePointType(i, ESplinePointType::Linear, false);
	}

	Spline->SetClosedLoop(true);
}

TArray<FVector> AFCSplineArea::GetSplinePoints() const
{
	TArray<FVector> SplinePoints;
	for (int32 i = 0; i < Spline->GetNumberOfSplinePoints(); i++)
	{
		FVector SplinePointLocation = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
		SplinePoints.Add(SplinePointLocation);
	}

	return SplinePoints;
}

bool AFCSplineArea::IsPontInsideArea(const FVector2D& InPoint) const
{
	if (SplinePoints2D.Num() == 0) return true;

	const FVector2D& Point = InPoint;
	FVector2D Lsp = SplinePoints2D[SplinePoints2D.Num() - 1];

	bool bIsInside = false;
	for (const FVector2D& Csp : SplinePoints2D)
	{
		const bool bCheckIsInside = ((((Csp.Y < Point.Y) && (Lsp.Y >= Point.Y)) || ((Lsp.Y < Point.Y) && (Csp.Y >= Point.Y))) && ((Csp.X + (((Point.Y - Csp.Y) / (Lsp.Y - Csp.Y)) * (Lsp.X - Csp.X))) < Point.X));
		if (bCheckIsInside)
		{
			bIsInside = !bIsInside;
		}

		Lsp = Csp;
	}

	return bIsInside;
}

bool AFCSplineArea::IsPontInsideArea3d(const FVector& InPoint) const
{
	return IsPontInsideArea(FVector2D(InPoint.X, InPoint.Y));
}

void AFCSplineArea::ShuffleArray(TArray<FTransform>& InArray)
{
	if (InArray.Num() > 0)
	{
		int32 LastIndex = InArray.Num() - 1;
		for (int32 i = 0; i <= LastIndex; ++i)
		{
			int32 Index = FMath::RandRange(i, LastIndex);
			if (i != Index)
			{
				InArray.Swap(i, Index);
			}
		}
	}
}
