#include "Game/Enemy/KoopaJrShipCannonMainShell.h"

#include "Game/Util.h"

namespace {
    static const f32 sTmpScale = 1.2f;
    static const f32 sAttackSensorRadius = 75.0f;
    static const f32 sShadowRadius = 60.0f;
    static const f32 sBinderRadius = 75.0f;
    static const s32 sLifeTime = 300;
    static const f32 sCameraShakeDistance = 1000.0f;
    static const s32 sWallHitInvalidTime = 60;


    static const char* sModelName = "KoopaJrShipCannonMainShell";
    static const char* sFlyEffectName = "LocusSmoke";
    static const char* sFlyEffectName2 = "LocusFire";
    static const char* sExplosionEffectName = "Explosion";
    static const char* sMisFireEffectName = "MisFire";
    static const char* sTouchEffectName = "Touch";
};  // namespace



KoopaJrShipCannonMainShell::KoopaJrShipCannonMainShell(const char* pName) : CannonShellBase(pName) {
    mBaseScale = sTmpScale;
    mLifeTime = sLifeTime;
    mHitInvalidTime = sWallHitInvalidTime;
    mCanDrop = false;
    mCanHipDrop = false;
    mCanFreeze = false;
}

void KoopaJrShipCannonMainShell::init(const JMapInfoIter& rIter) {
    mScale.set(mBaseScale);
    initModelManagerWithAnm(sModelName, nullptr, nullptr, false);
    MR::startBtk(this, sModelName);
    initSound(4, false, nullptr, TVec3f());
    initHitSensor(1);
    MR::addHitSensorEnemyAttack(this, "Attack", 8, sAttackSensorRadius * mBaseScale, TVec3f(0.0f, 0.0f, 0.0f));
    initBinder(sBinderRadius * mBaseScale, 0.0f, 0);
    initEffectKeeper(0, nullptr, false);
    MR::connectToSceneEnemy(this);
    MR::invalidateClipping(this);
    MR::initShadowVolumeSphere(this, sShadowRadius * mBaseScale);

    CannonShellBase::init(rIter);
}

void KoopaJrShipCannonMainShell::launch(const TVec3f& rStartPos, const TVec3f& rVelocity) {
    mTranslation.set(rStartPos); // Doing this before LiveActor::appear will hopefully avoid the invisible hitsensor glitch from Galaxy 1
    appear();
    TVec3f vec(rVelocity);
    MR::normalize(&vec);
    TPos3f mtx;
    MR::makeMtxFrontUp(&mtx, vec, -mGravity); // Different from the CannonShellBase
    mtx.getQuat(_8C);
    mVelocity.set(rVelocity);
    setNerve(&NrvCannonShellBase::HostTypeFly::sInstance);


    MR::startSound(this, "SE_BM_KOOPAJR_SHIP_SHOOT_NORMAL", -1, -1);
}

void KoopaJrShipCannonMainShell::kill() {
    MR::tryDeleteEffect(this, sFlyEffectName);
    MR::tryDeleteEffect(this, sFlyEffectName2);
    CannonShellBase::kill();
}

void KoopaJrShipCannonMainShell::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (!MR::isSensorEnemyAttack(pSender)) {
        return;
    }

    if (MR::isSensorPlayer(pReceiver) && MR::sendMsgEnemyAttackFireStrong(pReceiver, pSender)) {
        explosion();
        return;
    }

    if (!MR::isSensorEnemy(pReceiver)) {
        return;
    }

    if (!isNerve(&NrvCannonShellBase::HostTypeFly::sInstance)) {
        return;
    }

    if (MR::sendMsgEnemyAttackExplosion(pReceiver, pSender)) {
        return;
    }
}

bool KoopaJrShipCannonMainShell::receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isMsgJetTurtleAttack(msg)) {
        MR::sendMsgEnemyAttack(pReceiver, pSender);
        return true;
    }

    return MR::isMsgStarPieceReflect(msg);
}

bool KoopaJrShipCannonMainShell::receiveMsgEnemyAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    return false;
}

bool KoopaJrShipCannonMainShell::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    return false;
}

void KoopaJrShipCannonMainShell::explosion() {
    MR::tryEmitEffect(this, sExplosionEffectName);
    MR::startSound(this, "SE_EM_KILLER_EXPLOSION", -1, -1);
    CannonShellBase::explosion();
}

void KoopaJrShipCannonMainShell::exeFly() {
    if (MR::isFirstStep(this)) {
        MR::tryEmitEffect(this, sFlyEffectName);
        MR::tryEmitEffect(this, sFlyEffectName2);
    }
    MR::startLevelSound(this, "SE_EM_LV_KILLER_FLY", -1, -1, -1);
    CannonShellBase::exeFly();
}