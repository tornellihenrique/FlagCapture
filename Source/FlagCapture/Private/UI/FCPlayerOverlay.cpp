#include "UI/FCPlayerOverlay.h"

#include "Components/CanvasPanel.h"

#include "Player/FCPlayerController.h"
#include "Player/FCPlayerState.h"
#include "Character/FCCharacter.h"
#include "Weapon/FCWeapon.h"

#include "Ability/AttributeSet/FCAmmoAttributeSet.h"

UFCPlayerOverlay::UFCPlayerOverlay(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFCPlayerOverlay::NativeConstruct()
{
	Super::NativeConstruct();

	bIsDead = true;
	MainPanel->SetRenderOpacity(0.0f);

	AFCPlayerController* const OwningPC = GetOwningPlayer<AFCPlayerController>();
	if (!OwningPC) return;

	AFCPlayerState* const OwningPS = OwningPC->GetPlayerState<AFCPlayerState>();
	if (!OwningPS) return;

	SetCurrentHealth(OwningPS->GetHealth());
	SetMaxHealth(OwningPS->GetMaxHealth());
	SetHealthPercentage(OwningPS->GetHealth() / OwningPS->GetMaxHealth());
	SetHealthRegenRate(OwningPS->GetHealthRegenRate());
	SetCurrentStamina(OwningPS->GetStamina());
	SetMaxStamina(OwningPS->GetMaxStamina());
	SetStaminaPercentage(OwningPS->GetStamina() / OwningPS->GetMaxStamina());
	SetStaminaRegenRate(OwningPS->GetStaminaRegenRate());

	AFCCharacter* Character = GetOwningPlayerPawn<AFCCharacter>();
	if (Character)
	{
		AFCWeapon* CurrentWeapon = Character->GetWeapon();
		if (CurrentWeapon)
		{
			SetAmmoAmount(CurrentWeapon->GetAmmoAmount());

			if (OwningPS->GetAmmoAttributeSet())
			{
				FGameplayAttribute Attribute = OwningPS->GetAmmoAttributeSet()->GetAmmoAttributeFromTag(CurrentWeapon->AmmoType);
				if (Attribute.IsValid())
				{
					SetReserveAmmoAmount(OwningPS->GetAbilitySystemComponent()->GetNumericAttribute(Attribute));
				}
			}
		}
	}
}

void UFCPlayerOverlay::NativeDestruct()
{
	Super::NativeDestruct();
}

void UFCPlayerOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (MainPanel->GetRenderOpacity() <= 1.0f && !bIsDead)
	{
		const float TargetOpacity = FMath::FInterpTo(MainPanel->GetRenderOpacity(), 1.0f, InDeltaTime, 4.0f);
		MainPanel->SetRenderOpacity(TargetOpacity);
	}
}

void UFCPlayerOverlay::OnPlaying()
{
	bIsDead = false;
	OnPlayingEvent();
}

void UFCPlayerOverlay::OnDeath()
{
	bIsDead = true;
	MainPanel->SetRenderOpacity(0.0f);

	OnDeathEvent();
}
