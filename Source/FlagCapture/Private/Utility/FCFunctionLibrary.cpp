#include "Utility/FCFunctionLibrary.h"

#include "Ability/FCGameplayAbility.h"
#include "Ability/FCAbilitySystemComponent.h"

#include "Game/FCGameState.h"
#include "Game/FCGameMode.h"
#include "Player/FCPlayerController.h"
#include "Player/FCPlayerState.h"
#include "Character/FCCharacter.h"
#include "Player/FCSpectatorPawn.h"
#include "DamageType/FCDamageType.h"
#include "FCSettings.h"

void UFCFunctionLibrary::K2_GetGameMode(const UObject* WorldContextObject, AFCGameMode*& OutValue, EOutResultPins& OutResult)
{
	if (UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		if (AFCGameState* const GameState = World->GetGameState<AFCGameState>())
		{
			if (const AFCGameMode* GameMode = GameState->GetDefaultGameMode<AFCGameMode>())
			{
				OutValue = (AFCGameMode*)GameMode;
				if (OutValue && ::IsValid(OutValue))
				{
					OutResult = EOutResultPins::IsValid;
					return;
				}
			}
		}
	}

	OutResult = EOutResultPins::IsNotValid;
}

void UFCFunctionLibrary::K2_GetGameState(const UObject* WorldContextObject, AFCGameState*& OutValue, EOutResultPins& OutResult)
{
	if (UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		OutValue = World->GetGameState<AFCGameState>();
		if (OutValue && ::IsValid(OutValue))
		{
			OutResult = EOutResultPins::IsValid;
			return;
		}
	}

	OutResult = EOutResultPins::IsNotValid;
}

void UFCFunctionLibrary::K2_GetPlayerController(const UObject* WorldContextObject, AFCPlayerController*& OutValue, EOutResultPins& OutResult)
{
	if (UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		OutValue = World->GetFirstPlayerController<AFCPlayerController>();
		if (OutValue && ::IsValid(OutValue))
		{
			OutResult = EOutResultPins::IsValid;
			return;
		}
	}

	OutResult = EOutResultPins::IsNotValid;
}

void UFCFunctionLibrary::K2_GetPlayerState(const UObject* WorldContextObject, AFCPlayerState*& OutValue, EOutResultPins& OutResult)
{
	if (UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		if (const AFCPlayerController* PlayerController = World->GetFirstPlayerController<AFCPlayerController>())
		{
			OutValue = PlayerController->GetPlayerState<AFCPlayerState>();
			if (OutValue && ::IsValid(OutValue))
			{
				OutResult = EOutResultPins::IsValid;
				return;
			}
		}
	}

	OutResult = EOutResultPins::IsNotValid;
}

void UFCFunctionLibrary::K2_GetPlayerCharacter(const UObject* WorldContextObject, AFCCharacter*& OutValue, EOutResultPins& OutResult)
{
	if (UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		if (const AFCPlayerController* PlayerController = World->GetFirstPlayerController<AFCPlayerController>())
		{
			OutValue = PlayerController->GetPawn<AFCCharacter>();
			if (OutValue && ::IsValid(OutValue))
			{
				OutResult = EOutResultPins::IsValid;
				return;
			}
		}
	}

	OutResult = EOutResultPins::IsNotValid;
}

void UFCFunctionLibrary::K2_GetSpectatorPawn(const UObject* WorldContextObject, AFCSpectatorPawn*& OutValue, EOutResultPins& OutResult)
{
	if (UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		if (const AFCPlayerController* PlayerController = World->GetFirstPlayerController<AFCPlayerController>())
		{
			OutValue = Cast<AFCSpectatorPawn>(PlayerController->GetSpectatorPawn());
			if (OutValue && ::IsValid(OutValue))
			{
				OutResult = EOutResultPins::IsValid;
				return;
			}
		}
	}

	OutResult = EOutResultPins::IsNotValid;
}

void UFCFunctionLibrary::GetSpawnPointsAtLocation(const UObject* WorldContextObject, FVector const& Location, float Radius, int32 NumPoints, float CapsuleHeight, TArray<FTransform>& Out)
{
	if (UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		const float AngleIncrement = 2 * PI / NumPoints;

		for (int32 i = 0; i < NumPoints; i++)
		{
			const float Angle = i * AngleIncrement;
			const float X = Location.X + Radius * FMath::Cos(Angle);
			const float Y = Location.Y + Radius * FMath::Sin(Angle);

			FVector Point(FVector(X, Y, Location.Z));
			const FRotator LookAt = FRotationMatrix::MakeFromX(Location - Point).Rotator();

			FCollisionQueryParams CollisionParams;
			FCollisionQueryParams Params;
			Params.bTraceComplex = true;

			FHitResult HitResult;

			FVector EndLocation = Location;
			EndLocation.Z -= CapsuleHeight * 4.0f;

			if (World->LineTraceSingleByChannel(HitResult, Point, EndLocation, COLLISION_PROJECTILE, CollisionParams))
			{
				if (HitResult.bBlockingHit)
				{
					Point.Z = HitResult.Location.Z + CapsuleHeight;
				}
			}

			Out.Add(FTransform(LookAt, Point, FVector::OneVector));
		}
	}
}

bool UFCFunctionLibrary::CanSpawnAtLocation(const UObject* WorldContextObject, FVector const& Location, float CapsuleRadius, float CapsuleHeight, TArray<AActor*> const& Ignores)
{
	if (UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		FCollisionQueryParams Params;
		Params.bTraceComplex = true;
		Params.AddIgnoredActors(Ignores);

		FHitResult OutHit;

		FVector EndLocation = Location;
		EndLocation.Z -= CapsuleHeight * 2.0f;

		if (World->LineTraceSingleByChannel(OutHit, Location, EndLocation, ECC_Visibility, Params))
		{
			if (World->SweepSingleByChannel(OutHit, Location, Location + 0.0001f, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHeight), Params))
			{
				return !OutHit.bBlockingHit;
			}

			return true;
		}
	}

	return false;
}

void UFCFunctionLibrary::ShuffleArrayTransform(TArray<FTransform>& InArray)
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

FDamageData UFCFunctionLibrary::GetDamageData(const FKillEventData& KillEventData)
{
	if (KillEventData.IsValid())
	{
		if (UFCDamageType* const DamageType = Cast<UFCDamageType>(KillEventData.GetDamageType()))
		{
			FText DamageDisplayName = DamageType->DamageTitle;
			UTexture2D* DamageDisplayImage = DamageType->DamageIcon;
			UTexture2D* DamageDisplayIcon = DamageType->KillFeedIcon;

			return FDamageData(DamageDisplayName, DamageDisplayIcon, DamageDisplayImage, KillEventData.bIsHeadshot, DamageType->KillReward, DamageType->HeadshotReward);
		}
	}

	return FDamageData();
}

FLinearColor UFCFunctionLibrary::GetTeamColor(ETeamSide Team)
{
	return UFCSettings::Get()->GetColorForTeam(Team);
}

FLinearColor UFCFunctionLibrary::GetOppositeTeamColor(ETeamSide Team)
{
	return UFCSettings::Get()->GetOppositeColorForTeam(Team);
}

UFCGameplayAbility* UFCFunctionLibrary::GetPrimaryAbilityInstanceFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
{
	if (AbilitySystemComponent)
	{
		FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (AbilitySpec)
		{
			return Cast<UFCGameplayAbility>(AbilitySpec->GetPrimaryInstance());
		}
	}

	return nullptr;
}

UFCGameplayAbility* UFCFunctionLibrary::GetPrimaryAbilityInstanceFromClass(UAbilitySystemComponent* AbilitySystemComponent, TSubclassOf<UGameplayAbility> InAbilityClass)
{
	if (AbilitySystemComponent)
	{
		FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(InAbilityClass);
		if (AbilitySpec)
		{
			return Cast<UFCGameplayAbility>(AbilitySpec->GetPrimaryInstance());
		}
	}

	return nullptr;
}

bool UFCFunctionLibrary::IsPrimaryAbilityInstanceActive(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
{
	if (AbilitySystemComponent)
	{
		FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (AbilitySpec)
		{
			return Cast<UFCGameplayAbility>(AbilitySpec->GetPrimaryInstance())->IsActive();
		}
	}

	return false;
}

bool UFCFunctionLibrary::IsAbilitySpecHandleValid(FGameplayAbilitySpecHandle Handle)
{
	return Handle.IsValid();
}
