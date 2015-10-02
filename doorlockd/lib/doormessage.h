#ifndef DOORMESSAGE_H
#define DOORMESSAGE_H

struct Doormessage
{
    bool isOpen = { false };
    bool isUnlockButton = { false };
    bool isLockButton = { false };
    bool isEmergencyUnlock = { false };
};

#endif
