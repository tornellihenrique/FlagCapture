#include "FCSettings.h"

UFCSettings::UFCSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TeamAColor = FLinearColor(FColor::FromHex("349CD1FF"));
	TeamBColor = FLinearColor(FColor::FromHex("D03C34FF"));
}

FLinearColor UFCSettings::GetColorForTeam(ETeamSide InTeam)
{
	switch (InTeam)
	{
		case ETeamSide::Team_A:
			return TeamAColor;
		case ETeamSide::Team_B:
			return TeamBColor;
	}

	return FLinearColor(FColor::FromHex("00000000"));
}

FLinearColor UFCSettings::GetOppositeColorForTeam(ETeamSide InTeam)
{
	switch (InTeam)
	{
		case ETeamSide::Team_A:
			return TeamBColor;
		case ETeamSide::Team_B:
			return TeamAColor;
	}

	return FLinearColor(FColor::FromHex("00000000"));
}
