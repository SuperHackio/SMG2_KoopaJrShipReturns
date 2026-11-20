#include "Game/NPC/KoopaJr.h"
#include "Game/LiveActor/LodCtrl.h"
#include "Game/LiveActor/ModelObj.h"
#include "Game/Util.h"

namespace NrvKoopaJr {
   FULL_NERVE(HostTypeWait, KoopaJr, Wait);
   FULL_NERVE(HostTypeReaction, KoopaJr, Reaction);
   FULL_NERVE(HostTypeReactionEnd, KoopaJr, ReactionEnd);
   FULL_NERVE(HostTypeShipBattleAppear, KoopaJr, ShipBattleAppear);
   FULL_NERVE(HostTypeShipBattlePowerUp, KoopaJr, ShipBattlePowerUp);
   FULL_NERVE(HostTypeShipBattleEscape, KoopaJr, ShipBattleEscape);
   FULL_NERVE(HostTypeShipBattleShipDamage, KoopaJr, ShipBattleShipDamage);
   FULL_NERVE(HostTypeShipBattleDemoTalkStart, KoopaJr, ShipBattleDemoTalkStart);
   FULL_NERVE(HostTypeShipBattleDemoTalkWait, KoopaJr, ShipBattleDemoTalkWait);
};  // namespace NrvKoopaJr

KoopaJr::KoopaJr(const char* pName) : NPCActor(pName) {
    _15C = 0;
    _15D = 0;
}

void KoopaJr::init(const JMapInfoIter& rIter) {
    setLinkedInfo(rIter);
    NPCActor::init(rIter);
    initModelManagerWithAnm("KoopaJr", nullptr, false, false);
    NPCActorCaps caps("KoopaJr");
    caps._D = 1;
    caps.mInitScene = 1;
    caps.mSceneType = 0;
    caps.mInitDefaultPos = 1;
    caps.mInitLodCtrlNPC = 1;
    caps.mInitLightCtrl = 1;
    caps.mInitSound = 1;
    caps.mSoundObjName = nullptr;
    caps.mSoundObjsCount = 4;
    caps.mInitHitSensor = 1;
    caps.mBodySensorRadius = 175.0f;
    caps.mBodySensorOffset.y = 175.0f;
    caps.mInitSpine = 1;
    caps.mWaitNerve = &NrvKoopaJr::HostTypeWait::sInstance;
    caps.mInitTalkCtrl = 1;
    caps.mInitSphereShadow = 1;
    caps.mSphereShadowRadius = 100.0f;
    caps.mInitEffectKeeper = 1;
    mTalkParam.setSingleAction("Wait");
    mTalkParam.mEnableTurn = 0;
    mTalkParam.mEnableTalkTurn = 0;
    mActionReaction = "Damage";
    initialize(rIter, caps, nullptr, nullptr, false);
    MR::declareStarPiece(this, 20);
    if (MR::tryRegisterDemoCast(this, rIter)) {
        if (MR::tryRegisterDemoActionFunctor(this, MR::Functor(this, &KoopaJr::startShipBattleTalk), "クッパJr会話")) {
#if DEBUG
            OSReport("KoopaJr registered to cutscene\n");
#endif // DEBUG
        }
        if (mLodCtrl != nullptr) {
            if (mLodCtrl->mModelObjMiddle) {
                MR::tryRegisterDemoCast(mLodCtrl->mModelObjMiddle, rIter);
            }

            if (mLodCtrl->mModelObjLow) {
                MR::tryRegisterDemoCast(mLodCtrl->mModelObjLow, rIter);
            }
        }
    }

    if (mLodCtrl != nullptr) {
        mLodCtrl->setDistanceToMiddleAndLow(5000.0f, 10000.0f);
    }

    MR::onCalcShadow(this, nullptr);
    //MR::setShadowDropLength(this, nullptr, 200.0f);
    makeActorAppeared();
    MR::startBva(this, "Apron");
    if (mLodCtrl != nullptr) {
        if (mLodCtrl->mModelObjMiddle != nullptr) {
            MR::startBva(mLodCtrl->mModelObjMiddle, "Apron");
        }

        if (mLodCtrl->mModelObjLow != nullptr) {
            MR::startBva(mLodCtrl->mModelObjLow, "Apron");
        }
    }
}

void KoopaJr::kill() {
    if (mLodCtrl != nullptr) {
        mLodCtrl->invalidate();
    }

    LiveActor::kill();
}

void KoopaJr::setStateShipBattleAppear() {
    setNerve(&NrvKoopaJr::HostTypeShipBattleAppear::sInstance);
}

void KoopaJr::setStateShipBattlePowerUp() {
    setNerve(&NrvKoopaJr::HostTypeShipBattlePowerUp::sInstance);
}

void KoopaJr::setStateShipBattleEscape() {
    setNerve(&NrvKoopaJr::HostTypeShipBattleEscape::sInstance);
}

void KoopaJr::setStateShipBattleShipDamage() {
    setNerve(&NrvKoopaJr::HostTypeShipBattleShipDamage::sInstance);
}

void KoopaJr::endShipBattleTalk() {
    setNerve(mWaitNerve);
}

void KoopaJr::control() {
    NPCActor::control();
    turnToPlayer();
}

void KoopaJr::calcAndSetBaseMtx() {
    MR::setBaseTRMtx(this, mTurnDir);
}

void KoopaJr::startShipBattleTalk() {
    if (!_15D) {
        _15D = 1;
        //MR::tryTalkTimeKeepDemo(mTalkCtrl);
        {
            if (mTalkCtrl->requestTalkForce()) {
                mTalkCtrl->startTalkForce();
            }
        }
        setNerve(&NrvKoopaJr::HostTypeShipBattleDemoTalkStart::sInstance);
    }
}

void KoopaJr::exeWait() {
    if (MR::tryStartReaction(this)) {
        if (!mRequestJetTurtleAttack) {
            MR::startSound(this, "SE_OJ_STAR_PIECE_HIT_ENM_F");
        }

        setNerve(&NrvKoopaJr::HostTypeReaction::sInstance);
    } else {
        MR::tryStartTurnAction(this);
    }
}

void KoopaJr::exeReaction() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Damage", nullptr);
        MR::startSound(this, "SE_BV_KOOPAJR_DAMAGE_S");
        if (!_15C) {
            if (MR::appearStarPiece(this, mTranslation, 20, 10.0f, 40.0f, false)) {
                //MR::startSound(this, "SE_OJ_STAR_PIECE_BURST");
            }
        }
    }

    if (MR::isBckStopped(this)) {
        setNerve(&NrvKoopaJr::HostTypeReactionEnd::sInstance);
    }
}

void KoopaJr::exeReactionEnd() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Recovery", nullptr);
    }

    if (MR::isBckStopped(this)) {
        setNerve(mWaitNerve);
    }
}

void KoopaJr::exeShipBattleAppear() {
    if (MR::isFirstStep(this)) {
        if (MR::isEqualStageName("TriLegLv1Galaxy")) {
            MR::startBck(this, "RoboAppearDemo", nullptr);
        } else {
            MR::startBck(this, "AppearDemo", nullptr);
        }
    }

    if (MR::isBckStopped(this)) {
        setNerve(mWaitNerve);
    }
}

void KoopaJr::exeShipBattlePowerUp() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "AngryDemo", nullptr);
    }

    if (MR::isBckStopped(this)) {
        mTalkParam.setSingleAction("WaitAngry");
        setNerve(mWaitNerve);
    }
}

void KoopaJr::exeShipBattleEscape() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "RunAwayDemo", nullptr);
    }

    if (MR::isBckStopped(this)) {
        setNerve(mWaitNerve);
    }
}

void KoopaJr::exeShipBattleShipDamage() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Stumble", nullptr);
    }

    if (MR::isBckStopped(this)) {
        setNerve(mWaitNerve);
    }
}

void KoopaJr::exeShipBattleDemoTalkStart() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Talk", nullptr);
    }

    if (MR::isBckStopped(this)) {
        setNerve(&NrvKoopaJr::HostTypeShipBattleDemoTalkWait::sInstance);
    }
}

void KoopaJr::exeShipBattleDemoTalkWait() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "TalkWait", nullptr);
    }
}

KoopaJr::~KoopaJr() {
}
