#include "LockOnSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LockOnSettings)

const ULockOnSettings* ULockOnSettings::Get()
{
    return GetDefault<ULockOnSettings>();
}

const FLockOnCameraProfile& ULockOnSettings::GetCameraProfile(bool bUseLargeBossProfile) const
{
    return bUseLargeBossProfile ? LargeBossCameraProfile : DuelCameraProfile;
}
