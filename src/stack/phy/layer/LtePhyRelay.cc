//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/phy/layer/LtePhyRelay.h"

Define_Module(LtePhyRelay);

LtePhyRelay::~LtePhyRelay()
{
    cancelAndDelete(bdcStarter_);
}

void LtePhyRelay::initialize(int stage)
{
    if (stage == 0)
    {
        LtePhyBase::initialize(stage);
        nodeType_ = RELAY;
        masterId_ = getAncestorPar("masterId");

        WATCH(nodeType_);
        WATCH(masterId_);
    }
    else if (stage == 1)
    {
        LtePhyBase::initialize(stage);
        // deployer is on DeNB
        txPower_ = relayTxPower_;
        OmnetId masterOmnetId = binder_->getOmnetId(masterId_);
        bdcUpdateInterval_ = getSimulation()->getModule(masterOmnetId)->
        getSubmodule("deployer")->
        par("positionUpdateInterval");
        // TODO: add a parameter not to generate broadcasts (no handovers scenario)
        // e.g.: if (bdcUpdateInterval_ != 0 && !!doHandovers)
        if (bdcUpdateInterval_ != 0)
        {
            // self message provoking the generation of a broadcast message
            bdcStarter_ = new cMessage("bdcStarter");
            scheduleAt(NOW, bdcStarter_);
        }
    }
}

void LtePhyRelay::handleSelfMessage(cMessage *msg)
{
    if (msg->isName("bdcStarter"))
    {
        // send broadcast message
        LteAirFrame *f = createHandoverMessage();
        sendBroadcast(f);
        scheduleAt(NOW + bdcUpdateInterval_, msg);
    }
}

void LtePhyRelay::handleAirFrame(cMessage* msg)
{
    UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->removeControlInfo());
    LteAirFrame* frame = static_cast<LteAirFrame*>(msg);
    EV << "LtePhy: received new LteAirFrame with ID "
       << frame->getId() << " from channel" << endl;

    if (lteInfo->getFrameType() == HANDOVERPKT)
    {
        // handover broadcast frames must not be relayed or processed by eNB
        delete frame;
        return;
    }

    // send H-ARQ feedback up
    if (lteInfo->getFrameType() == HARQPKT)
    {
        handleControlMsg(frame, lteInfo);
        return;
    }

    bool result;
    result = channelModel_->error(frame, lteInfo);
    // update statistics
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << ( result ? "RECEIVED" : "NOT RECEIVED" ) << endl;

    cPacket* pkt = frame->decapsulate();

    // here frame has to be destroyed since it is no more useful
    delete frame;

    // attach the decider result to the packet as control info
    lteInfo->setDeciderResult(result);
    pkt->setControlInfo(lteInfo);

    // send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
    updateDisplayString();
}

