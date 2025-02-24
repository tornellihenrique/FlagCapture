#include "Player/FCSpectatorPawn.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"

#include "Player/FCPlayerController.h"
#include "Character/FCCharacter.h"
#include "Player/FCPlayerState.h"

AFCSpectatorPawn::AFCSpectatorPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;
	Camera->bUsePawnControlRotation = false;

	PrimaryActorTick.TickGroup = ETickingGroup::TG_PostPhysics;
}

void AFCSpectatorPawn::BeginPlay()
{
	Super::BeginPlay();

	LastKnowingBounds = FBoxSphereBounds(FVector::OneVector, FVector::OneVector, 45.0f);
	LastKnowingLocation = GetActorLocation() + (Camera->GetForwardVector() * 500.0f);
}

void AFCSpectatorPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AFCPlayerController* const PC = GetController<AFCPlayerController>())
	{
		if (APlayerCameraManager* PlayerCameraManager = PC->PlayerCameraManager)
		{
			PlayerCameraManager->OnBlendComplete().RemoveAll(this);
		}

		PC->OnTargetViewPlayerChanged.RemoveDynamic(this, &AFCSpectatorPawn::OnTargetViewPlayerChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void AFCSpectatorPawn::PossessedBy(class AController* NewController)
{
	Super::PossessedBy(NewController);

	ClientPossessedBy(NewController);
}

void AFCSpectatorPawn::UnPossessed()
{
	ClientUnPossessed();

	Super::UnPossessed();
}

void AFCSpectatorPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Camera->GetCameraView(DeltaTime, OutResult);
}

void AFCSpectatorPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocallyControlled()) return;

	float FinalSize = 0.0f;
	const bool bIsValidTarget = FocusedPawn && ::IsValid(FocusedPawn) && FocusedPawn->GetPlayerState() && ::IsValid(FocusedPawn->GetPlayerState());
	const FVector OriginLocation = Camera->GetComponentLocation();
	float InterpolationSpeed = CameraRotationInterpSpeed;

	if (bIsValidTarget)
	{
		LastKnowingLocation = FocusedPawn->GetActorLocation();

		if (AFCCharacter* const FocusedCharacter = Cast<AFCCharacter>(FocusedPawn))
		{
			if (USkeletalMeshComponent* SkeletalMeshComponent = FocusedCharacter->GetMesh())
			{
				if (SkeletalMeshComponent->GetSkinnedAsset())
				{
					LastKnowingBounds = SkeletalMeshComponent->GetSkinnedAsset()->GetBounds();
				}
			}
		}
	}
	else
	{
		RespawnCheck();
	}

	const FRotator ViewRotation = FRotationMatrix::MakeFromX(LastKnowingLocation - OriginLocation).Rotator();
	const FRotator TargetRotation = FMath::RInterpTo(Camera->GetComponentRotation(), ViewRotation, DeltaSeconds, InterpolationSpeed);

	Camera->SetWorldRotation(TargetRotation);

	const float X = OriginLocation.X - LastKnowingLocation.X;
	const float Y = OriginLocation.Y - LastKnowingLocation.Y;
	const float Z = OriginLocation.Z - LastKnowingLocation.Z;
	const float XX = X * X;
	const float YY = Y * Y;
	const float ZZ = Z * Z;

	const float Radius = LastKnowingBounds.SphereRadius * 0.75f;
	const float Sqrt = FMath::Sqrt(XX + YY + ZZ);
	const float TargetSize = Radius / (Sqrt * FMath::Sin(PI / (180.f) * 90.0f));
	FinalSize = FMath::Clamp(TargetSize, 0.0f, 1.0f);

	const float Division = FMath::GetMappedRangeValueClamped(FVector2D(0.1f, 0.0f), FVector2D(1.0f, 8.0f), FinalSize);
	const float DefaultFOV = GetDefault<AFCSpectatorPawn>()->FOV;
	const float DesiredFOV = !bIsValidTarget ? DefaultFOV : DefaultFOV / Division;

	const float TargetFOV = FMath::FInterpTo(Camera->FieldOfView, DesiredFOV, DeltaSeconds, CameraInterpSpeed);
	Camera->SetFieldOfView(TargetFOV);
}

void AFCSpectatorPawn::InitializeCamera()
{
	FVector TargetLocation = GetGroundLocationWithOffsets(184.f);
	SetActorLocation(TargetLocation);

	LastKnowingBounds = FBoxSphereBounds(FVector::OneVector, FVector::OneVector, 45.0f);
	LastKnowingLocation = GetActorLocation() + (Camera->GetForwardVector() * 500.0f);
}

void AFCSpectatorPawn::RespawnCheck()
{
	if (AFCPlayerController* const PC = GetController<AFCPlayerController>())
	{
		if (CachedTargetPlayer)
		{
			APawn* const TargetPawn = CachedTargetPlayer->GetPawn<APawn>();
			if (TargetPawn && ::IsValid(TargetPawn))
			{
				bool bNeedRefresh = FocusedPawn != TargetPawn;

				if (bNeedRefresh)
				{
					if (PC->GetViewTarget() != this)
					{
						PC->ClientSetViewTarget(this);
					}

					const float Duratio = PC->FadeToPlayingDuration;
					PC->FadeOut(Duratio);

					InitializeCamera();

					FocusedPawn = TargetPawn;
					bBlendFinish = bTargetPawnDead = false;
				}
			}
		}
	}
}

void AFCSpectatorPawn::ClientPossessedBy_Implementation(AController* NewController)
{
	if (AFCPlayerController* const PC = Cast<AFCPlayerController>(NewController))
	{
		if (APlayerCameraManager* PlayerCameraManager = PC->PlayerCameraManager)
		{
			PlayerCameraManager->OnBlendComplete().AddUObject(this, &AFCSpectatorPawn::OnBlendComplete);
		}

		if (AFCPlayerState* const TargetPlayer = Cast<AFCPlayerState>(PC->GetLastKillerPlayer()))
		{
			CachedTargetPlayer = TargetPlayer;

			RespawnCheck();
		}

		PC->OnTargetViewPlayerChanged.AddDynamic(this, &AFCSpectatorPawn::OnTargetViewPlayerChanged);
	}
}

void AFCSpectatorPawn::ClientUnPossessed_Implementation()
{
	FocusedPawn = nullptr;
}

void AFCSpectatorPawn::OnBlendComplete()
{
	bBlendFinish = true;
}

void AFCSpectatorPawn::OnTargetViewPlayerChanged(APlayerState* InPlayerState)
{
	if (InPlayerState)
	{
		CachedTargetPlayer = Cast<AFCPlayerState>(InPlayerState);
		if (CachedTargetPlayer)
		{
			FocusedPawn = nullptr;
		}
	}
	else
	{
		if (CachedTargetPlayer)
		{
			CachedTargetPlayer = nullptr;
			FocusedPawn = nullptr;
		}
	}
}

FVector AFCSpectatorPawn::GetGroundLocationWithOffsets(float Offsets)
{
	const FVector OriginLocation = GetActorLocation();
	const FVector UpVector = FRotationMatrix(GetActorRotation()).GetScaledAxis(EAxis::Z) * Offsets;

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);
	CollisionParams.bReturnPhysicalMaterial = false;

	bool HitTheGround = false;
	if (GetWorld()->LineTraceSingleByChannel(HitResult, OriginLocation + UpVector, OriginLocation - UpVector, ECC_Visibility, CollisionParams))
	{
		HitTheGround = HitResult.bBlockingHit;
	}

	if (HitTheGround)
	{
		FVector TagetLocation = OriginLocation;
		TagetLocation.Z = HitResult.Location.Z + Offsets;

		return TagetLocation;
	}

	return OriginLocation;
}

void AFCSpectatorPawn::OnStateReset()
{
	if (AFCPlayerController* const PC = GetController<AFCPlayerController>())
	{
		CachedTargetPlayer = nullptr;
		FocusedPawn = nullptr;

		if (PC->GetViewTarget() != this)
		{
			PC->ClientSetViewTarget(this);
		}
	}
}

void AFCSpectatorPawn::OnMatchEnded()
{
	if (AFCPlayerController* const PC = GetController<AFCPlayerController>())
	{
		CachedTargetPlayer = nullptr;
		FocusedPawn = nullptr;

		if (PC->GetViewTarget() != this)
		{
			FViewTargetTransitionParams Trans;
			Trans.BlendTime = 2.0f;
			Trans.bLockOutgoing = true;
			Trans.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;

			PC->ClientSetViewTarget(this, Trans);
		}

		const float Duratio = PC->FadeToPlayingDuration;
		PC->FadeOut(Duratio);
	}
}

AFCCharacter* AFCSpectatorPawn::GetFocusedCharacter() const
{
	return Cast<AFCCharacter>(FocusedPawn);
}
