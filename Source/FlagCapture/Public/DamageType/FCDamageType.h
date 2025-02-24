#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "FCDamageType.generated.h"

UCLASS()
class FLAGCAPTURE_API UFCDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageType, meta = (ExposeOnSpawn = "true"))
	FText DamageTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageType, meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UTexture2D> DamageIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageType, meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UTexture2D> KillFeedIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageType, meta = (ExposeOnSpawn = "true"))
	int32 KillReward = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageType, meta = (ExposeOnSpawn = "true"))
	int32 HeadshotReward = 50;

	UFCDamageType() {}

};
