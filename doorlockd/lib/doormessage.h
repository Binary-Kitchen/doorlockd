#ifndef DOORMESSAGE_H
#define DOORMESSAGE_H

struct Doormessage
{
    Doormessage(bool isUnlockButton,
                bool isLockButton,
                bool isEmergencyUnlock);

    Doormessage();

    bool isUnlockButton = { false };
    bool isLockButton = { false };
    bool isEmergencyUnlock = { false };
};

#endif
