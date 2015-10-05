#include "doormessage.h"

Doormessage::Doormessage()
{
}

Doormessage::Doormessage(bool isUnlockButton, bool isLockButton, bool isEmergencyUnlock) :
    isUnlockButton(isUnlockButton),
    isLockButton(isLockButton),
    isEmergencyUnlock(isEmergencyUnlock)
{
}
