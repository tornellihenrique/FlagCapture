#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FCTypes.h"
#include "FCSettings.generated.h"

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Flag Capture Settings"))
class FLAGCAPTURE_API UFCSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category = "Configs")
	FLinearColor TeamAColor;

	UPROPERTY(config, EditAnywhere, Category = "Configs")
	FLinearColor TeamBColor;

public:
	UFCSettings(const FObjectInitializer& ObjectInitializer);

	virtual FName GetCategoryName() const override { return FName(TEXT("Flag Capture")); }

public:
	static UFCSettings* Get() { return CastChecked<UFCSettings>(StaticClass()->GetDefaultObject()); }

	FLinearColor GetColorForTeam(ETeamSide InTeam);
	FLinearColor GetOppositeColorForTeam(ETeamSide InTeam);

};
