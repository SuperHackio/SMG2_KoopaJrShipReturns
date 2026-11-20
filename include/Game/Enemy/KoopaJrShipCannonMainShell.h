#pragma once

#include "CannonShellBase.h"

class KoopaJrShipCannonMainShell : public CannonShellBase {
public:
    KoopaJrShipCannonMainShell(const char*);

    virtual void init(const JMapInfoIter& rIter);
    virtual void kill();
    virtual void attackSensor(HitSensor* pSender, HitSensor* pReceiver);
    virtual bool receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver);
    virtual bool receiveMsgEnemyAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver);
    virtual bool receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver);

    virtual void launch(const TVec3f& rStartPos, const TVec3f& rVelocity);
    virtual void explosion();
    virtual void exeFly();
};