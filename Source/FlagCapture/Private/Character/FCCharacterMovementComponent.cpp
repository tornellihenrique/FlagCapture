#include "Character/FCCharacterMovementComponent.h"

#include "Character/FCCharacterBase.h"

void UFCCharacterMovementComponent::FFCSavedMove::Clear()
{
	Super::Clear();

	SavedRequestToStartSprinting = false;
	SavedRequestToStartAiming = false;
}

uint8 UFCCharacterMovementComponent::FFCSavedMove::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (SavedRequestToStartSprinting)
	{
		Result |= FLAG_Custom_0;
	}

	if (SavedRequestToStartAiming)
	{
		Result |= FLAG_Custom_1;
	}

	return Result;
}

bool UFCCharacterMovementComponent::FFCSavedMove::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	if (SavedRequestToStartSprinting != ((FFCSavedMove*)NewMove.Get())->SavedRequestToStartSprinting)
	{
		return false;
	}

	if (SavedRequestToStartAiming != ((FFCSavedMove*)NewMove.Get())->SavedRequestToStartAiming)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void UFCCharacterMovementComponent::FFCSavedMove::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UFCCharacterMovementComponent* CharacterMovement = Cast<UFCCharacterMovementComponent>(Character->GetCharacterMovement());
	if (CharacterMovement)
	{
		SavedRequestToStartSprinting = CharacterMovement->RequestToStartSprinting;
		SavedRequestToStartAiming = CharacterMovement->RequestToStartAiming;
	}
}

void UFCCharacterMovementComponent::FFCSavedMove::PrepMoveFor(class ACharacter* Character)
{
	Super::PrepMoveFor(Character);
}

UFCCharacterMovementComponent::FGSNetworkPredictionData_Client::FGSNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UFCCharacterMovementComponent::FGSNetworkPredictionData_Client::AllocateNewMove()
{
	return FSavedMovePtr(new FFCSavedMove());
}

UFCCharacterMovementComponent::UFCCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SprintSpeedMultiplier(1.5f)
	, AimingSpeedMultiplier(0.6f)
{
}

float UFCCharacterMovementComponent::GetMaxSpeed() const
{
	AFCCharacterBase* Owner = Cast<AFCCharacterBase>(GetOwner());
	
	if (!Owner) return Super::GetMaxSpeed();

	if (!Owner->IsAlive()) return 0.0f;

	if (RequestToStartSprinting)
	{
		return Owner->GetMoveSpeed() * SprintSpeedMultiplier;
	}

	if (RequestToStartAiming)
	{
		return Owner->GetMoveSpeed() * AimingSpeedMultiplier;
	}

	return Owner->GetMoveSpeed();
}

void UFCCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	RequestToStartSprinting = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;

	RequestToStartAiming = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
}

class FNetworkPredictionData_Client* UFCCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);

	if (!ClientPredictionData)
	{
		UFCCharacterMovementComponent* MutableThis = const_cast<UFCCharacterMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FGSNetworkPredictionData_Client(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}

void UFCCharacterMovementComponent::StartSprinting()
{
	RequestToStartSprinting = true;
}

void UFCCharacterMovementComponent::StopSprinting()
{
	RequestToStartSprinting = false;
}

void UFCCharacterMovementComponent::StartAiming()
{
	RequestToStartAiming = true;
}

void UFCCharacterMovementComponent::StopAiming()
{
	RequestToStartAiming = false;
}
