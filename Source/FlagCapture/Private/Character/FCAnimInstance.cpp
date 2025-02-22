#include "Character/FCAnimInstance.h"

#include "Character/FCCharacter.h"

UFCAnimInstance::UFCAnimInstance()
{
}

void UFCAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AFCCharacter>(TryGetPawnOwner());
}
