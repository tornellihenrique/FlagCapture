#include "Player/FCPlayerCharacter.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"

#include "Ability/FCAbilitySystemComponent.h"
#include "FCTypes.h"
#include "Player/FCPlayerState.h"


AFCPlayerCharacter::AFCPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Camera = CreateDefaultSubobject<UCameraComponent>(FName{ TEXTVIEW("Camera") });
	Camera->SetupAttachment(GetMesh(), TEXT("CameraSocket"));
	Camera->SetRelativeRotation_Direct(FRotator(0.0f, 90.0f, 0.0f));
	Camera->bEditableWhenInherited = true;
	Camera->bUsePawnControlRotation = true;
}

void AFCPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AFCPlayerState* PS = GetPlayerState<AFCPlayerState>();
	if (PS)
	{
		BindASCInput();
	}
}

void AFCPlayerCharacter::NotifyControllerChanged()
{
	const APlayerController* PreviousPlayer = Cast<APlayerController>(PreviousController);
	if (::IsValid(PreviousPlayer))
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PreviousPlayer->GetLocalPlayer());
		if (IsValid(InputSubsystem))
		{
			InputSubsystem->RemoveMappingContext(InputMappingContext);
		}
	}

	APlayerController* NewPlayer = Cast<APlayerController>(GetController());
	if (::IsValid(NewPlayer))
	{
		NewPlayer->InputYawScale_DEPRECATED = 1.0f;
		NewPlayer->InputPitchScale_DEPRECATED = 1.0f;
		NewPlayer->InputRollScale_DEPRECATED = 1.0f;

		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer());
		if (::IsValid(InputSubsystem))
		{
			InputSubsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	Super::NotifyControllerChanged();
}

void AFCPlayerCharacter::BindASCInput()
{
	if (!bASCInputBound && AbilitySystemComponent.IsValid() && ::IsValid(InputComponent))
	{
		FTopLevelAssetPath AbilityEnumAssetPath = FTopLevelAssetPath(FName("/Script/FlagCapture"), FName("EAbilityInputID"));
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, FGameplayAbilityInputBinds(FString("ConfirmTarget"), FString("CancelTarget"), AbilityEnumAssetPath, static_cast<int32>(EAbilityInputID::Confirm), static_cast<int32>(EAbilityInputID::Cancel)));

		bASCInputBound = true;
	}
}

void AFCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Input);
	if (::IsValid(EnhancedInput))
	{
		EnhancedInput->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &AFCPlayerCharacter::Input_OnLookMouse);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFCPlayerCharacter::Input_OnMove);
	}

	BindASCInput();
}

void AFCPlayerCharacter::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
	const FVector2D Value = ActionValue.Get<FVector2D>();

	AddControllerPitchInput(Value.Y * LookUpMouseSensitivity);
	AddControllerYawInput(Value.X * LookRightMouseSensitivity);
}

void AFCPlayerCharacter::Input_OnMove(const FInputActionValue& ActionValue)
{
	const FVector2D Value = ActionValue.Get<FVector2D>();

	const FRotator YawRotation = FRotator(0.0f, GetControlRotation().Yaw, 0.0f);
	
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection * Value.Y + RightDirection * Value.X);
}
