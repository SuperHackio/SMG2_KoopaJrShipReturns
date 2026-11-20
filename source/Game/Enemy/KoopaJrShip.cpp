#include "Game/Enemy/KoopaJrShip.h"
#include "Game/Enemy/HomingKiller.h"
#include "Game/Enemy/Kameck.h"
#include "Game/Enemy/KoopaJrShipCannonMainShell.h"
#include "Game/Enemy/KoopaJrShipCannonShell.h"
#include "Game/LiveActor/HitSensor.h"
#include "Game/LiveActor/ModelObj.h"
#include "Game/Animation/XAnimeCore.h"
#include "Game/NPC/KoopaJr.h"
#include "Game/Util.h"
#include "Game/Demo/DemoFunction.h"
#include "Game/Enemy/KameckBeamHolder.h"

namespace {
    static const char* cJointNamePropellerBack0 = "Screw00";
    static const char* cJointNamePropellerBack1 = "Screw01";
    static const char* cJointNamePropellerTop = "Propeller00";
    static const char* cJointNameCannonMain = "FirePoint3";
    static const char* cJointNameKoopaJrPos = "KoopaJrShip";
    static const char* cJointNamePodPos = "KoopaJrShipPod";

    static const Vec sKillerLauncherAngle[6] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
                                                {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

    static const Vec sKoopaJrPos = {135.0f, 188.0f, 0.0f};
    static const Vec sKoopaJrPosFront = {0.0f, 188.0f, 135.0f};

    static const char* const cJointNameCannon[6] = {"FirePoint0", "FirePoint1", "FirePoint2", "FirePoint4", "FirePoint5", "FirePoint6"};
    static const char* const cEffectNameShoot[6] = {"ShootJ0", "ShootJ1", "ShootJ2", "ShootJ4", "ShootJ5", "ShootJ6"};
};  // namespace

namespace NrvKoopaJrShip {
    FULL_NERVE(HostTypeAppear, KoopaJrShip, Appear);
    FULL_NERVE(HostTypeMove, KoopaJrShip, Move);
    FULL_NERVE(HostTypeMoveFrontAttack, KoopaJrShip, MoveFrontAttack);
    FULL_NERVE(HostTypeShoot1, KoopaJrShip, Shoot);
    FULL_NERVE(HostTypeShoot2, KoopaJrShip, Shoot);
    FULL_NERVE(HostTypeShootMain, KoopaJrShip, ShootMain);
    FULL_NERVE(HostTypeStopAtEnd, KoopaJrShip, StopAtEnd);
    FULL_NERVE(HostTypePowerUp, KoopaJrShip, PowerUp);
    FULL_NERVE(HostTypeDamage, KoopaJrShip, Damage);
    FULL_NERVE(HostTypeBreakStart, KoopaJrShip, BreakStart);
    FULL_NERVE(HostTypeBreak, KoopaJrShip, Break);
    FULL_NERVE(HostTypeBreakEnd, KoopaJrShip, BreakEnd);
    FULL_NERVE(HostTypeTurnFront, KoopaJrShip, TurnFront);
};  // namespace NrvKoopaJrShip

namespace {
    const f32 PI = 3.1415927f;

    inline f32 toRadian(f32 angle) {
        f32 x = (180.0f / PI);
        return angle * x;
    }

    void calcRailRateToNextPoint(f32* pRate, const LiveActor* pActor) {
        RailRider* railRider = pActor->mRailRider;
        f32 currPointCoord = railRider->getCurrentPointCoord();
        f32 nextPointCoord = railRider->getNextPointCoord();
        f32 coord = railRider->mCoord;

        if (nextPointCoord == currPointCoord) {
            *pRate = 0.0f;
        }
        else {
            *pRate = (coord - currPointCoord) / (nextPointCoord - currPointCoord);
        }
    }

    void setJointTransformLocalMtx(const LiveActor* pActor, const char* pName, MtxPtr pMtx) {
        XjointTransform* pTransform = MR::getJointTransform(pActor, pName);
        pTransform->_68 = (u32)pMtx;
    }
}

extern "C" {
    bool __kAutoMap_8013B110(LiveActor*, const char*, const char*, const Nerve*, const Nerve*, s32, DemoStartInfo::DemoType, DemoStartInfo::CinemaFrameType, DemoStartInfo::StarPointerType, DemoStartInfo::DeleteEffectType); // requestStartTimeKeepDemo
}

KoopaJrShip::KoopaJrShip(const char* pName)
    : LiveActor(pName), mShellHolder(nullptr), mMainShellHolder(nullptr), mJr(nullptr), mShipBreakModel(nullptr), mPodModel(nullptr), mHealth(5), mHealthSecond(2), mHealthFirst(3), mStarId(-1),
      _D4(0.0f, 0.0f, 0.0f), _E0(0.0f, 0.0f, 1.0f), _EC(0), mPropRotateSpeed(20.0f), _184(0.0f), _188(0), _1EC(::sKoopaJrPos), _1F8(0.0f, 0.0f, 0.0f) {
    mScrew00Mtx.identity();
    mScrew01Mtx.identity();
    mPropellerMtx.identity();
    mShipMtx.identity();
    mPodMtx.identity();
}

bool KoopaJrShip::isStateBreak(void) const {
    return isNerve(&NrvKoopaJrShip::HostTypeBreakStart::sInstance) || isNerve(&NrvKoopaJrShip::HostTypeBreak::sInstance) ||
           isNerve(&NrvKoopaJrShip::HostTypeBreakEnd::sInstance);
}

void KoopaJrShip::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    initModelManagerWithAnm("KoopaJrShip", nullptr, nullptr, false);
    MR::startBck(this, "KoopaJrShip", nullptr);
    MR::setBckRate(this, 0.0f);
    MR::initJointTransform(this);
    ::setJointTransformLocalMtx(this, ::cJointNamePropellerTop, mPropellerMtx);
    ::setJointTransformLocalMtx(this, ::cJointNamePropellerBack0, mScrew00Mtx);
    ::setJointTransformLocalMtx(this, ::cJointNamePropellerBack1, mScrew01Mtx);
    MR::connectToSceneCollisionEnemyStrongLight(this);
    initEffectKeeper(1, nullptr, false);
    initSound(8, false, nullptr, TVec3f(0.0f, 0.0f, 0.0f));
    initHitSensor(1);
    MR::initCollisionParts(this, "KoopaJrShip", MR::addBodyMessageSensorEnemy(this), nullptr);
    MR::invalidateClipping(this);
    initRailRider(rIter);

    MR::getJMapInfoArg0NoInit(rIter, &mStarId);
    if (mStarId > 0)
        MR::declarePowerStar(this, mStarId);
    MR::addToAttributeGroupSearchTurtle(this);
    initShells();
    initKoopaJr(rIter);
    initKameck(rIter);
    initKiller();
    mShipBreakModel = MR::createModelObjMapObjStrongLight("クッパJrシップ壊れモデル", "KoopaJrShipBreak", mShipMtx);
    mShipBreakModel->makeActorDead();
    mPodModel = MR::createModelObjMapObjStrongLight("クッパJrシップポッドモデル", "KoopaJrShipPod", mPodMtx);
    mPodModel->makeActorDead();

    MR::getJMapInfoArg1NoInit(rIter, &mHealthFirst);
    MR::getJMapInfoArg2NoInit(rIter, &mHealthSecond);

    mHealth = mHealthFirst + mHealthSecond;

    if (MR::tryRegisterDemoCast(this, rIter)) {
        MR::registerDemoActionNerve(this, &NrvKoopaJrShip::HostTypeAppear::sInstance, "出現");
        MR::registerDemoActionFunctor(this, MR::Functor(this, &KoopaJrShip::setStateTurnFront), "旋廻");
        MR::tryRegisterDemoCast(mShipBreakModel, rIter);
        MR::tryRegisterDemoCast(mPodModel, rIter);
        initNerve(&NrvKoopaJrShip::HostTypeAppear::sInstance, 0);
        makeActorDead();
    } else {
        initNerve(&NrvKoopaJrShip::HostTypeMove::sInstance, 0);
        makeActorAppeared();
    }
}

void KoopaJrShip::initAfterPlacement() {
    MR::moveCoordAndTransToNearestRailPos(this);
    _1F8.set(mTranslation);
}

void KoopaJrShip::kill() {
    if (mStarId > 0)
        MR::requestAppearPowerStar(this, mStarId, mTranslation);
    LiveActor::kill();
}

void KoopaJrShip::control() {
    _EC = MR::repeat(_EC + mPropRotateSpeed, 0.0f, 360.0f);

    f32 angle = ::toRadian(_EC);
    f32 s = sin(angle);
    f32 c = cos(angle);

    mPropellerMtx[0][0] = c;
    mPropellerMtx[1][1] = 1.0f;
    mPropellerMtx[0][2] = s;
    mPropellerMtx[2][0] = -s;
    mPropellerMtx[2][2] = c;
    mPropellerMtx[2][1] = 0.0f;
    mPropellerMtx[1][2] = 0.0f;
    mPropellerMtx[1][0] = 0.0f;
    mPropellerMtx[0][1] = 0.0f;

    mScrew00Mtx.setEulerZ(::toRadian(_EC));
    mScrew01Mtx.setEulerZ(::toRadian(_EC));
    MR::setRailCoordSpeed(this, _184);

    if (getHealth() > 0) {
        if (getHealth() <= mHealthSecond+1) {
            MR::startLevelSound(this, "SE_BM_LV_KOOPAJR_SHIP_BURN1", -1, -1, -1);
        }
        if (getHealth() <= 1) {
            MR::startLevelSound(this, "SE_BM_LV_KOOPAJR_SHIP_BURN2", -1, -1, -1);
        }
        MR::startLevelSound(this, "SE_BM_LV_KOOPAJR_SHIP_MOVE", -1, -1, -1);
    }

    updateKoopaJrPos();
    updateKillerPos();
}

bool KoopaJrShip::receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isMsgJetTurtleAttack(msg)) {
        if (receiveMsgJetTurtleAttack(pSender, pReceiver)) {
            return true;
        }
    }

    return false;
}

void KoopaJrShip::initShells() {
    mShellHolder = new CannonShellHolder(0x12);
    //CannonShellUtil::registerKoopaJrShipCannonShell(mShellHolder, 0x12);
    for (s32 i = 0; i < 0x12; i++) {
        KoopaJrShipCannonShell* pShell = new KoopaJrShipCannonShell("砲弾");
        pShell->initWithoutIter();
        pShell->makeActorDead();
        mShellHolder->registerCannonShell(pShell);
    }

    mMainShellHolder = new CannonShellHolder(9);
    //CannonShellUtil::registerKoopaJrShipCannonMainShell(mMainShellHolder, 9);
    for (s32 i = 0; i < 9; i++) {
        KoopaJrShipCannonMainShell* pShell = new KoopaJrShipCannonMainShell("メイン砲弾");
        pShell->initWithoutIter();
        pShell->makeActorDead();
        mMainShellHolder->registerCannonShell(pShell);
    }
}

void KoopaJrShip::initKoopaJr(const JMapInfoIter& rIter) {
    mJr = new KoopaJr("クッパJr");
    TPos3f mtx;
    mtx.set(getBaseMtx());
    mtx.setTrans(mTranslation);
    mtx.mult(_1EC, mJr->mTranslation);
    mJr->init(rIter);
    mJr->kill();
}

void KoopaJrShip::initKameck(const JMapInfoIter& rIter) {
    s32 childNum = MR::getChildObjNum(rIter);
    for (s32 i = 0; i < childNum; i++) {
        const char* name;
        MR::getChildObjName(&name, rIter, i);
        if (!MR::isEqualSubString(name, "Kameck"))
            continue;

        if (mKamecks.size() >= 2)
            continue;

        Kameck* kameck = new Kameck("カメック[クッパＪｒシップ]");
        MR::initChildObj(kameck, rIter, i);
        kameck->makeActorDead();
        mKamecks.push_back(kameck);
    }
}

void KoopaJrShip::initKiller() {
    for (HomingKiller** pActor = mKillers.begin(); pActor != mKillers.end(); pActor++) {
        *pActor = new HomingKiller("ホーミングキラー");
        (*pActor)->initWithoutIter();
        //(*pActor)->setChaseStartEndDistance(5500.0f, 6500.0f);
        (*pActor)->_98 = 5500.0f;
        (*pActor)->_9C = 6500.0f;
    }
}

bool KoopaJrShip::receiveMsgJetTurtleAttack(HitSensor* pSender, HitSensor* pReceiver) {
    if (!MR::isSensorEnemy(pReceiver)) {
        return false;
    }

    if (isNerve(&NrvKoopaJrShip::HostTypeDamage::sInstance)) {
        return false;
    }

    if (isNerve(&NrvKoopaJrShip::HostTypePowerUp::sInstance)) {
        return false;
    }

    if (isStateBreak()) {
        return false;
    }

    _D4.set(pSender->mPosition);
    _E0.set(pSender->mActor->mVelocity);
    MR::normalizeOrZero(&_E0);

    if (isNextStateDamage(mHealth)) {
        MR::startSound(this, "SE_BM_KOOPAJR_SHIP_DAMAGE_L");
        MR::startSound(mJr, "SE_BV_KOOPAJR_DAMAGE_L");
        MR::startSystemSE("SE_SY_VS_BOSS_LAST_HIT", -1, -1);
        setNerve(&NrvKoopaJrShip::HostTypeBreakStart::sInstance);
    } else {
        MR::startSound(this, "SE_BM_KOOPAJR_SHIP_DAMAGE");
        MR::startSound(mJr, "SE_BV_KOOPAJR_DAMAGE_S");
        MR::startSystemSE("SE_SY_VS_BOSS_DAMAGE_1", -1, -1);
        setNerve(&NrvKoopaJrShip::HostTypeDamage::sInstance);
    }

    return true;
}

s32 KoopaJrShip::getNumShootShells() const {
    if (isNerve(&NrvKoopaJrShip::HostTypeShoot1::sInstance)) {
        return 1;
    }

    return isNerve(&NrvKoopaJrShip::HostTypeShoot2::sInstance) != 0 ? 5 : 0;
}

f32 KoopaJrShip::getPropellerRotSpeed() const {
    if (isNerve(&NrvKoopaJrShip::HostTypeBreakStart::sInstance)) {
        return (40.0f * (1.0f - MR::calcNerveRate(this, 60)));
    }

    if (isNerve(&NrvKoopaJrShip::HostTypeBreak::sInstance)) {
        return 0.0f;
    }

    if (isNerve(&NrvKoopaJrShip::HostTypePowerUp::sInstance) || getHealth() <= mHealthSecond && !isNerve(&NrvKoopaJrShip::HostTypeDamage::sInstance)) {
        return 40.0f;
    }

    return 20.0f;
}

void KoopaJrShip::updateCoordSpeed() {
    f32 rate = 0.0f;
    ::calcRailRateToNextPoint(&rate, this);

    if (0.95f < rate) {
        f32 v3;
        if (getHealth() <= mHealthSecond) {
            v3 = 2.0f;
        } else {
            v3 = 4.0f;
        }

        _184 = MR::getEaseInValue(rate, v3, 1.0f, 1.0f);
    } else {
        if (_188 >= 0) {
            f32 v5;
            if (getHealth() <= mHealthSecond) {
                v5 = 2.0f;
            } else {
                v5 = 4.0f;
            }

            f32 ease = MR::getEaseInValue(_188, v5, 0.0f, 60.0f);
            _184 = ease;
            _188 = _188 - 1;
            MR::startLevelSound(this, "SE_BM_LV_KOOPAJR_SHIP_ACCEL");
        }

        mPropRotateSpeed = getPropellerRotSpeed();
    }
}

void KoopaJrShip::calcLauncherInfo(TVec3f* a1, TVec3f* a2, s32 idx) const {
    TPos3f mtx;
    mtx.set(MR::getJointMtx(this, ::cJointNameCannon[idx]));
    mtx.getTrans(*a1);
    mtx.getXDir(*a2);
    MR::normalize(a2);
}

void KoopaJrShip::calcLauncherInfoKiller(TVec3f* a1, TVec3f* a2, s32 idx) const {
    TPos3f mtx;
    TVec3f v16;
    TVec3f v15;

    mtx.set(MR::getJointMtx(this, ::cJointNameCannon[idx]));
    mtx.getTrans(*a1);
    mtx.getXDir(*a2);
    mtx.getYDir(v16);
    MR::normalize(&v16);

    mtx.getZDir(v15);
    MR::normalize(&v15);
    MR::rotateVecDegree(a2, v16, ::sKillerLauncherAngle[idx].y);
    MR::rotateVecDegree(a2, v15, ::sKillerLauncherAngle[idx].z);
    MR::normalize(a2);
    a1->add(*a2 * -100.0f);
}

void KoopaJrShip::shootShell(s32 idx) {
    CannonShellBase* shell = mShellHolder->getValidShell();

    if (shell != nullptr) {
        TVec3f v8, v7;
        calcLauncherInfo(&v8, &v7, idx);
        shell->launch(v8, v7 * 15.0f);
    }

    MR::tryEmitEffect(this, ::cEffectNameShoot[idx]);
}

bool KoopaJrShip::tryShootAllKillers() {
    if (isExistActiveKiller()) {
        return false;
    }

    if (isExistActiveKameck()) {
        return false;
    }

    for (HomingKiller** pActor = mKillers.begin(); pActor != mKillers.end(); pActor++) {
        TVec3f v6, v5;
        s32 idx = mKillers.indexOf(pActor);
        calcLauncherInfoKiller(&v6, &v5, idx);
        (*pActor)->appear(v6, v5);
        MR::tryEmitEffect(this, ::cEffectNameShoot[idx]);
    }

    return true;
}

void KoopaJrShip::shootKiller(s32 idx) {
    HomingKiller* killer = mKillers[idx];
    if (MR::isDead(killer)) {
        TVec3f v6, v5;
        calcLauncherInfoKiller(&v6, &v5, idx);
        killer->appear(v6, v5);
        MR::tryEmitEffect(this, ::cEffectNameShoot[idx]);
    }
}

void KoopaJrShip::shootKillersAfterDamage() {
    if (getHealth() <= mHealthSecond) {
        tryShootAllKillers();
    } else {
        shootKiller(0);
        shootKiller(1);
        shootKiller(2);
    }
}

bool KoopaJrShip::isExistActiveKiller() const {

    for (HomingKiller* const* pActor = mKillers.begin(); pActor != mKillers.end(); pActor++) {
        if (!MR::isDead(*pActor))
            return true;
    }
    return false;

    //return std::find_if(mKillers.begin(), mKillers.end(), std::not1(std::ptr_fun(&MR::isDead))) != mKillers.end();
}

bool KoopaJrShip::isExistActiveKameck() const {
    for (Kameck* const* pActor = mKamecks.begin(); pActor != mKamecks.end(); pActor++) {
        if (!MR::isDead(*pActor))
            return true;
    }
    return false;
    //return std::find_if(mKamecks.begin(), mKamecks.end(), std::not1(std::ptr_fun(&MR::isDead))) != mKamecks.end();
}

void KoopaJrShip::shootMainShells() {
    TPos3f m;
    m.set(MR::getJointMtx(this, ::cJointNameCannonMain));

    TVec3f pos;
    m.getTrans(pos);

    // Straight ahead
    {
        TVec3f forward;
        m.getZDir(forward);
        MR::normalize(&forward);
        CannonShellBase* p = mMainShellHolder->getValidShell();
        p->launch(pos, forward * 23.0f);
    }

    MR::rotateMtxLocalYDegree(m, 15.0f);

    // Side
    {
        TVec3f forward;
        m.getZDir(forward);
        MR::normalize(&forward);
        CannonShellBase* p = mMainShellHolder->getValidShell();
        p->launch(pos, forward * 23.0f);
    }

    MR::rotateMtxLocalYDegree(m, -30.0f);

    // Other side
    {
        TVec3f forward;
        m.getZDir(forward);
        MR::normalize(&forward);
        CannonShellBase* p = mMainShellHolder->getValidShell();
        p->launch(pos, forward * 23.0f);
    }
}

void KoopaJrShip::emitDamageSmokeEffect() {
    if (getHealth() == mHealthSecond+1) {
        MR::tryEmitEffect(this, "DamageSmoke1");
    } else if (getHealth() == 1) {
        MR::tryEmitEffect(this, "DamageSmoke2");
    }
}

void KoopaJrShip::emitDamageHitEffect() {
    TPos3f v7;
    v7.identity();
    TVec3f up;
    MR::calcUpVec(&up, this);
    MR::makeMtxSideUpPos(&v7, -_E0, up, _D4);
    MR::emitEffectHit(this, v7, "DamageFire");
    MR::startSound(this, "SE_BM_KOOPAJR_SHIP_IGNIT");
}

void KoopaJrShip::updateKoopaJrPos() {
    if (isNerve(&NrvKoopaJrShip::HostTypeBreak::sInstance)) {
        TPos3f v8;
        v8.set(MR::getJointMtx(mPodModel, ::cJointNamePodPos));
        v8.mult(_1EC, mJr->mTranslation);
    } else {
        TPos3f v7;
        v7.set(MR::getJointMtx(this, ::cJointNameKoopaJrPos));
        MR::faceToPoint(v7, *MR::getPlayerPos(), 5.0f);
        v7.mult(_1EC, mJr->mTranslation);
    }
}

void KoopaJrShip::updateKillerPos() {
    for (HomingKiller** pActor = mKillers.begin(); pActor != mKillers.end(); pActor++) {
        if (MR::isDead(*pActor)) {
            continue;
        }

        TVec3f v7;
        TVec3f v6;
        calcLauncherInfoKiller(&v7, &v6, mKillers.indexOf(pActor));
        //(*pActor)->setBasePosAndFront(v7, v6);
        (*pActor)->mBasePos = v7;
        (*pActor)->mBaseFront = v6;
    }
}

void KoopaJrShip::killAllSubModels() {
    mJr->kill();
    mPodModel->makeActorDead();
    mShipBreakModel->makeActorDead();
}

void KoopaJrShip::setStateTurnFront() {
    mTranslation.set(_1F8);
    MR::moveCoordAndTransToNearestRailPos(this);
    mJr->setStateShipBattlePowerUp();
    _1EC.x = ::sKoopaJrPosFront.x;
    _1EC.y = ::sKoopaJrPosFront.y;
    _1EC.z = ::sKoopaJrPosFront.z;
    setNerve(&NrvKoopaJrShip::HostTypeTurnFront::sInstance);
}

void KoopaJrShip::exeAppear() {
    if (MR::isFirstStep(this)) {
        MR::startSound(this, "SE_BM_KOOPAJR_SHIP_ENTER");
        MR::startBck(this, "Arrival", nullptr);
        MR::hideModel(this);
    }

    if (MR::isStep(this, 1)) {
        MR::showModel(this);
        mJr->appear();
        mJr->setStateShipBattleAppear();
    }

    if (MR::isDemoPartLastStep("クッパJr会話")) {
        mJr->endShipBattleTalk();
        _188 = 60;
        MR::startStageBGM("MBGM_SMG2_BOSS_06A", false);
        setNerve(&NrvKoopaJrShip::HostTypeMove::sInstance);
    }
}

void KoopaJrShip::exeMove() {
    if (MR::isFirstStep(this)) {
        f32 speed = getHealth() <= mHealthSecond ? 2.0f : 4.0f;
        MR::setRailCoordSpeed(this, speed);
    }

    updateCoordSpeed();
    MR::moveCoordAndFollowTrans(this);

    if (MR::isGreaterStep(this, 0)) {
        if (!(getNerveStep() % 120) && !isExistActiveKiller()) {
            if (getHealth() == mHealthFirst + mHealthSecond)
                setNerve(&NrvKoopaJrShip::HostTypeShoot1::sInstance);
            else
                setNerve(&NrvKoopaJrShip::HostTypeShoot2::sInstance);
        }
    }

    if (MR::isRailReachedEdge(this)) {
        setNerve(&NrvKoopaJrShip::HostTypeStopAtEnd::sInstance);
    }
}

void KoopaJrShip::exeMoveFrontAttack() {
    if (MR::isFirstStep(this)) {
        f32 speed = getHealth() <= mHealthSecond ? 2.0f : 4.0f;
        MR::setRailCoordSpeed(this, speed);
        MR::emitEffect(this, "HeadShootSign");
    }

    MR::startLevelSound(this, "SE_BM_LV_KOOPAJR_SHIP_MAIN_PREP");
    updateCoordSpeed();
    MR::moveCoordAndFollowTrans(this);

    if (MR::isRailReachedGoal(this)) {
        if (MR::isGreaterStep(this, 120)) {
            MR::reverseRailDirection(this);
            setNerve(&NrvKoopaJrShip::HostTypeShootMain::sInstance);
        }
    } else if (MR::isStep(this, 120)) {
        setNerve(&NrvKoopaJrShip::HostTypeShootMain::sInstance);
    }
}

void KoopaJrShip::exeShoot() {
    MR::moveCoordAndFollowTrans(this);
    updateCoordSpeed();

    for (s32 i = 0; i < getNumShootShells(); i++) {
        if (MR::isStep(this, 45 * i)) {
            shootShell(1);

            if (i == getNumShootShells() - 1) {
                setNerve(&NrvKoopaJrShip::HostTypeMove::sInstance);
                return;
            }
        }
    }

    if (MR::isRailReachedEdge(this)) {
        setNerve(&NrvKoopaJrShip::HostTypeStopAtEnd::sInstance);
    }
}

void KoopaJrShip::exeShootMain() {
    if (MR::isFirstStep(this)) {
        MR::emitEffect(this, "HeadShoot");
        MR::startSound(this, "SE_BM_KOOPAJR_SHIP_SHOOT_MAIN");
        MR::shakeCameraNormal();

        if (getHealth() <= 1) {
            tryShootAllKillers();
        }
    }

    if (MR::isStep(this, 15)) {
        shootMainShells();
    }

    MR::moveCoordAndFollowTrans(this);
    updateCoordSpeed();

    if (MR::isStep(this, 150)) {
        setNerve(&NrvKoopaJrShip::HostTypeMoveFrontAttack::sInstance);
    }
}

void KoopaJrShip::exeStopAtEnd() {
    s32 step = (getHealth() <= mHealthSecond) ? 90 : 150;

    if (MR::isStep(this, step)) {
        MR::reverseRailDirection(this);
        _188 = 60;

        if (getHealth() <= mHealthSecond) {
            setNerve(&NrvKoopaJrShip::HostTypeMoveFrontAttack::sInstance);
        } else {
            setNerve(&NrvKoopaJrShip::HostTypeMove::sInstance);
        }
    }
}

void KoopaJrShip::exePowerUp() {
    if (MR::isStep(this, 90)) {
        _188 = 60;
        setNerve(&NrvKoopaJrShip::HostTypeMove::sInstance);
    }
}

void KoopaJrShip::exeDamage() {
    if (MR::isFirstStep(this)) {
        _184 = 0.0f;
        MR::startBck(this, "Damage", nullptr);
        mJr->setStateShipBattleShipDamage();
        emitDamageSmokeEffect();
        emitDamageHitEffect();
        MR::shakeCameraNormal();
        MR::tryRumbleDefaultHit(this, 0);
        MR::stopScene(12);
    }

    if (MR::isStep(this, 90)) {
        if (getHealth() == mHealthSecond) {
            if (NameObj* demoexe = (NameObj*)DemoFunction::findDemoExecutor(this))
                __kAutoMap_8013B110(this, demoexe->mName, "旋廻", nullptr, nullptr, 1, DemoStartInfo::DEMOTYPE_1, DemoStartInfo::CINEMAFRAMETYPE_0, DemoStartInfo::STARPOINTERTYPE_0, DemoStartInfo::DELETEEFFECTYPE_0);

            return;
        }

        shootKillersAfterDamage();
        if (getHealth() == mHealthSecond+1) {
            if (mKamecks.size() > 0)
                mKamecks.mArray[0]->appear();
        }

        if (getHealth() == 1) {
            if (mKamecks.size() > 1)
            mKamecks.mArray[1]->appear();
        }

        if (getHealth() == mHealthSecond) {
            setNerve(&NrvKoopaJrShip::HostTypePowerUp::sInstance);
            return;
        }

        _188 = 60;

        // Needed to force another load and comparison
        if (getHealth() <= mHealthSecond) {
            setNerve(&NrvKoopaJrShip::HostTypeMoveFrontAttack::sInstance);
        } else {
            setNerve(&NrvKoopaJrShip::HostTypeMove::sInstance);
        }
    }
}

void KoopaJrShip::exeBreakStart() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Damage", nullptr);
        mJr->setStateShipBattleShipDamage();
        emitDamageHitEffect();
        MR::shakeCameraNormal();
        MR::tryRumbleDefaultHit(this, 0);
        MR::stopScene(12);
    }

    if (MR::isStep(this, 60)) {
        mShellHolder->killActiveShells();
        mMainShellHolder->killActiveShells();

        Kameck* const* pActor;

        for (pActor = mKamecks.begin(); pActor != mKamecks.end(); pActor++) {
            if (!MR::isDead(*pActor)) {
                //(*pActor)->makeActorDeadForce();
                {
                    (*pActor)->resetBeam();
                    (*pActor)->mBeamHolder->killAll();
                    (*pActor)->makeActorDead();
                }
            }
        }

        for (HomingKiller** pActor = mKillers.begin(); pActor != mKillers.end(); pActor++) {
            if (!MR::isDead(*pActor)) {
                (*pActor)->kill();
            }
        }

        //MR::requestStartDemoRegistered(this, nullptr, nullptr, );
        if (NameObj* demoexe = (NameObj*)DemoFunction::findDemoExecutor(this))
            __kAutoMap_8013B110(this, demoexe->mName, "破壊", nullptr, nullptr, 1, DemoStartInfo::DEMOTYPE_1, DemoStartInfo::CINEMAFRAMETYPE_0, DemoStartInfo::STARPOINTERTYPE_0, DemoStartInfo::DELETEEFFECTYPE_0);

        mShipMtx.set(getBaseMtx());
        mShipBreakModel->appear();
        MR::startBck(mShipBreakModel, "Break", nullptr);
        mPodMtx.set(getBaseMtx());
        mPodModel->appear();
        MR::startBck(mPodModel, "Escape", nullptr);
        setNerve(&NrvKoopaJrShip::HostTypeBreak::sInstance);
    }
}

void KoopaJrShip::exeBreak() {
    if (MR::isFirstStep(this)) {
        MR::deleteEffect(this, "EyeLight");
        MR::tryRumblePadStrong(this, 0);
        MR::shakeCameraNormal();
        MR::startSound(this, "SE_BM_KOOPAJR_SHIP_BREAK_S");
        mJr->setStateShipBattleEscape();
        MR::hideModel(this);
        MR::deleteEffect(this, "DamageSmoke1");
        MR::deleteEffect(this, "DamageSmoke2");
    }

    if (!MR::isHiddenModel(mPodModel) && MR::isBckStopped(mPodModel)) {
        MR::hideModel(mPodModel);
        MR::hideModel(mJr);
    }

    if (MR::isStep(this, 287)) {
        MR::startSound(this, "SE_BM_KOOPAJR_SHIP_BREAK_S");
    }

    if (MR::isLessStep(this, 300)) {
        MR::startLevelSound(this, "SE_BM_LV_KOOPAJR_SHIP_BURN1");
        MR::startLevelSound(this, "SE_BM_LV_KOOPAJR_SHIP_BURN2");
    }

    if (MR::isStep(this, 300)) {
        MR::shakeCameraNormal();
        MR::tryRumblePadStrong(this, 0);
        MR::startSound(this, "SE_BM_KOOPAJR_SHIP_BREAK_L");
    }

    if (!MR::isHiddenModel(mShipBreakModel) && (MR::isBckStopped(mShipBreakModel) || (MR::isStep(this, 320)) != 0)) {
        mShipBreakModel->makeActorDead();
    }

    if (MR::isDemoLastStep()) {
        killAllSubModels();
        setNerve(&NrvKoopaJrShip::HostTypeBreakEnd::sInstance);
    }
}

void KoopaJrShip::exeTurnFront() {
    if (MR::isFirstStep(this)) {
        MR::stopStageBGM(30);
        mShellHolder->killActiveShells();
        mMainShellHolder->killActiveShells();

        if (mKamecks.size() > 0 && !MR::isDead(mKamecks.mArray[0])) {
            _AC.push_back(mKamecks[0]);
        }

        //mKamecks[0]->makeActorDeadForce();
        if (mKamecks.size() > 0)
        {
            mKamecks[0]->resetBeam();
            mKamecks[0]->mBeamHolder->killAll();
            mKamecks[0]->makeActorDead();
        }
        //std::for_each(mKillers.begin(), mKillers.end(), std::mem_func(&HomingKiller::makeActorDead));
        for (HomingKiller** pActor = mKillers.begin(); pActor != mKillers.end(); pActor++) {
            (*pActor)->makeActorDead();
        }
    }

    MR::startLevelSound(this, "SE_BM_LV_KOOPAJR_SHIP_3RD_DEMO");

    if (MR::isLessEqualStep(this, 150)) {
        mRotation.y += 0.60f;
    }

    if (MR::isStep(this, 150)) {
        MR::emitEffect(this, "EyeLight");
        MR::startSound(this, "SE_BM_KOOPAJR_SHIP_EYE_BLINK");
    }

    if (MR::isDemoPartLastStep("旋廻")) {
        _188 = 60;
        MR::startStageBGM("MBGM_SMG2_BOSS_06B", false);

        if (_AC[2] != nullptr) {
            if (mKamecks.size() > 0)
                mKamecks[0]->appear();
        }

        setNerve(&NrvKoopaJrShip::HostTypeMoveFrontAttack::sInstance);
    }
}

void KoopaJrShip::exeBreakEnd() {
    if (MR::isFirstStep(this)) {
        // TODO: BGM?
        //MR::startAfterBossBGM();
        kill();
    }
}
