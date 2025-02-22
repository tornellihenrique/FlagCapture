#include "Ability/AbilityTask/FCAT_WaitDelayOneFrame.h"

void UFCAT_WaitDelayOneFrame::Activate()
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UFCAT_WaitDelayOneFrame::OnDelayFinish);
}

UFCAT_WaitDelayOneFrame* UFCAT_WaitDelayOneFrame::WaitDelayOneFrame(UGameplayAbility* OwningAbility)
{
	UFCAT_WaitDelayOneFrame* MyObj = NewAbilityTask<UFCAT_WaitDelayOneFrame>(OwningAbility);
	return MyObj;
}

void UFCAT_WaitDelayOneFrame::OnDelayFinish()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFinish.Broadcast();
	}

	EndTask();
}
