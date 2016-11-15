//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <assert.h>
#include "LtePhyUe.h"
#include "LteFeedbackPkt.h"
#include "IP2lte.h"
#include "LteDlFeedbackGenerator.h"

Define_Module(LtePhyUe);

LtePhyUe::LtePhyUe()
{
    handoverStarter_ = NULL;
    handoverTrigger_ = NULL;
}

LtePhyUe::~LtePhyUe()
{
    cancelAndDelete(handoverStarter_);
    delete das_;
}

void LtePhyUe::initialize(int stage)
{
    LtePhyBase::initialize(stage);

    if (stage == 0)
    {
        nodeType_ = UE;
        masterId_ = getAncestorPar("masterId");
        candidateMasterId_ = masterId_;

        dasRssiThreshold_ = 1.0e-5;
        das_ = new DasFilter(this, binder_, NULL, dasRssiThreshold_);
        das_->setMasterRuSet(masterId_);

        currentMasterRssi_ = 0;
        candidateMasterRssi_ = 0;
        hysteresisTh_ = 0;
        hysteresisFactor_ = 10;
        handoverDelta_ = 0.00001;
        handoverLatency_ = par("handoverLatency").doubleValue();

        // disabled
        useBattery_ = false;

        enableHandover_ = par("enableHandover");
        enableHandover_ = par("enableHandover"); // TODO : USE IT
        int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
        //int index2=intuniform(0,binder_->phyPisaData.maxChannel2()-1);
        deployer_->lambdaInit(nodeId_, index);
        //deployer_->channelUpdate(nodeId_,index2);
//        servingCell_ = registerSignal("servingCell");
//        emit(servingCell_, (long)masterId_);
        WATCH(nodeType_);
        WATCH(masterId_);
        WATCH(candidateMasterId_);
        WATCH(dasRssiThreshold_);
        WATCH(currentMasterRssi_);
        WATCH(candidateMasterRssi_);
        WATCH(hysteresisTh_);
        WATCH(hysteresisFactor_);
        WATCH(handoverDelta_);
        WATCH(das_);
    }
    else if (stage == 1)
    {
        if (useBattery_)
        {
            // TODO register the device to the battery with two accounts, e.g. 0=tx and 1=rx
            // it only affects statistics
            //registerWithBattery("LtePhy", 2);
//            txAmount_ = par("batteryTxCurrentAmount");
//            rxAmount_ = par("batteryRxCurrentAmount");
//
//            WATCH(txAmount_);
//            WATCH(rxAmount_);
        }

        txPower_ = ueTxPower_;

        lastFeedback_ = 0;

        handoverStarter_ = new cMessage("handoverStarter");
        mac_ = check_and_cast<LteMacUe *>(
            getParentModule()-> // nic
            getSubmodule("mac"));
        rlcUm_ = check_and_cast<LteRlcUm *>(
            getParentModule()-> // nic
            getSubmodule("rlc")->
                getSubmodule("um"));
    }
}

void LtePhyUe::handleSelfMessage(cMessage *msg)
{
    if (msg->isName("handoverStarter"))
        triggerHandover();
    else if (msg->isName("handoverTrigger"))
    {
        doHandover();
        delete msg;
        handoverTrigger_ = NULL;
    }
}

void LtePhyUe::handoverHandler(LteAirFrame* frame, UserControlInfo* lteInfo)
{
    lteInfo->setDestId(nodeId_);
    if (!enableHandover_)
    {
        // Even if handover is not enabled, this call is necessary
        // to allow Reporting Set computation.
        if (getNodeTypeById(lteInfo->getSourceId()) == ENODEB && lteInfo->getSourceId() == masterId_)
        {
            // Broadcast message from my master enb
            das_->receiveBroadcast(frame, lteInfo);
        }

        delete frame;
        delete lteInfo;
        return;
    }

    frame->setControlInfo(lteInfo);
    double rssi;

    if (getNodeTypeById(lteInfo->getSourceId()) == ENODEB && lteInfo->getSourceId() == masterId_)
    {
        // Broadcast message from my master enb
        rssi = das_->receiveBroadcast(frame, lteInfo);
    }
    else
    {
        // Broadcast message from relay or not-master enb
        std::vector<double>::iterator it;
        rssi = 0;
        std::vector<double> rssiV = channelModel_->getSINR(frame, lteInfo);
        for (it = rssiV.begin(); it != rssiV.end(); ++it)
            rssi += *it;
        rssi /= rssiV.size();
    }

    EV << "UE " << nodeId_ << " broadcast frame from " << lteInfo->getSourceId() << " with RSSI: " << rssi << " at " << simTime() << endl;

    if (rssi > candidateMasterRssi_ + hysteresisTh_)
    {
        if (lteInfo->getSourceId() == masterId_)
        {
            // receiving even stronger broadcast from current master
            currentMasterRssi_ = rssi;
            candidateMasterId_ = masterId_;
            candidateMasterRssi_ = rssi;
            hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);
            cancelEvent(handoverStarter_);
        }
        else
        {
            // broadcast from another master with higher rssi
            candidateMasterId_ = lteInfo->getSourceId();
            candidateMasterRssi_ = rssi;
            hysteresisTh_ = updateHysteresisTh(rssi);
            // schedule self message to evaluate handover parameters after
            // all broadcast messages are arrived
            if (!handoverStarter_->isScheduled())
            {
                // all broadcast messages are scheduled at the very same time, a small delta
                // guarantees the ones belonging to the same turn have been received
                scheduleAt(simTime() + handoverDelta_, handoverStarter_);
            }
        }
    }
    else
    {
        if (lteInfo->getSourceId() == masterId_)
        {
            currentMasterRssi_ = rssi;
            candidateMasterRssi_ = rssi;
            hysteresisTh_ = updateHysteresisTh(rssi);
        }
    }

    delete frame;
}

void LtePhyUe::triggerHandover()
{
    // TODO: remove asserts after testing
    assert(masterId_ != candidateMasterId_);

    EV << "####Handover starting:####" << endl;
    EV << "current master: " << masterId_ << endl;
    EV << "current rssi: " << currentMasterRssi_ << endl;
    EV << "candidate master: " << candidateMasterId_ << endl;
    EV << "candidate rssi: " << candidateMasterRssi_ << endl;
    EV << "############" << endl;

    EV << NOW << " LtePhyUe::triggerHandover - UE " << nodeId_ << " is starting handover to eNB " << candidateMasterId_ << "... " << endl;

    binder_->addUeHandoverTriggered(nodeId_);

    // inform the eNB's IP2lte module to forward data to the target eNB
    IP2lte* enbIp2lte =  check_and_cast<IP2lte*>(getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("nic")->getSubmodule("ip2lte"));
    enbIp2lte->triggerHandoverSource(nodeId_,candidateMasterId_);

    handoverTrigger_ = new cMessage("handoverTrigger");
    scheduleAt(simTime() + handoverLatency_, handoverTrigger_);

    if (getEnvir()->isGUI())
        getParentModule()->getParentModule()->bubble("Starting handover");
}

void LtePhyUe::doHandover()
{
    // Delete Old Buffers
    deleteOldBuffers(masterId_);

    // amc calls
    LteAmc *oldAmc = getAmcModule(masterId_);
    LteAmc *newAmc = getAmcModule(candidateMasterId_);

    // TODO verify the amc is the relay one and remove after tests
    assert(newAmc != NULL);

    oldAmc->detachUser(nodeId_, UL);
    oldAmc->detachUser(nodeId_, DL);
    newAmc->attachUser(nodeId_, UL);
    newAmc->attachUser(nodeId_, DL);

    // binder calls
    binder_->unregisterNextHop(masterId_, nodeId_);
    binder_->registerNextHop(candidateMasterId_, nodeId_);
    binder_->updateUeInfoCellId(nodeId_,candidateMasterId_);
    das_->setMasterRuSet(candidateMasterId_);

    // change masterId and notify handover to the MAC layer
    MacNodeId oldMaster = masterId_;
    masterId_ = candidateMasterId_;
    mac_->doHandover(candidateMasterId_);  // do MAC operations for handover
    currentMasterRssi_ = candidateMasterRssi_;
    hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);

    // update deployer
    LteMacEnb* newMacEnb =  check_and_cast<LteMacEnb*>(getSimulation()->getModule(binder_->getOmnetId(candidateMasterId_))->getSubmodule("nic")->getSubmodule("mac"));
    LteDeployer* newDeployer = newMacEnb->getDeployer();
    deployer_->detachUser(nodeId_);
    newDeployer->attachUser(nodeId_);
    deployer_ = newDeployer;

    // update DL feedback generator
    LteDlFeedbackGenerator* fbGen = check_and_cast<LteDlFeedbackGenerator*>(getParentModule()->getSubmodule("dlFbGen"));
    fbGen->handleHandover(masterId_);

    // collect stat
    emit(servingCell_, (long)masterId_);

    if (getEnvir()->isGUI())
        getParentModule()->getParentModule()->bubble("Handover complete!");

    EV << NOW << " LtePhyUe::doHandover - UE " << nodeId_ << " has completed handover to eNB " << masterId_ << "... " << endl;
    binder_->removeUeHandoverTriggered(nodeId_);

    // inform the eNB's IP2lte module to forward data to the target eNB
    IP2lte* enbIp2lte =  check_and_cast<IP2lte*>(getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("nic")->getSubmodule("ip2lte"));
    enbIp2lte->signalHandoverCompleteTarget(nodeId_,oldMaster);

    // TODO: transfer buffers
}


// TODO: ***reorganize*** method
void LtePhyUe::handleAirFrame(cMessage* msg)
{
    UserControlInfo* lteInfo = dynamic_cast<UserControlInfo*>(msg->removeControlInfo());

    if (useBattery_)
    {
        //TODO BatteryAccess::drawCurrent(rxAmount_, 0);
    }
    connectedNodeId_ = masterId_;
    LteAirFrame* frame = check_and_cast<LteAirFrame*>(msg);
    EV << "LtePhy: received new LteAirFrame with ID "
       << frame->getId() << " from channel" << endl;
    //Update coordinates of this user
    if (lteInfo->getFrameType() == HANDOVERPKT)
    {
        // check if handover is already in process
        if (handoverTrigger_ != NULL && handoverTrigger_->isScheduled())
        {
            delete lteInfo;
            delete frame;
            return;
        }

        handoverHandler(frame, lteInfo);
        return;
    }

    // Check if the frame is for us ( MacNodeId matches )
    if (lteInfo->getDestId() != nodeId_)
    {
        EV << "ERROR: Frame is not for us. Delete it." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        endSimulation();
    }

        /*
         * This could happen if the ue associates with a new master while the old one
         * already scheduled a packet for him: the packet is in the air while the ue changes master.
         * Event timing:      TTI x: packet scheduled and sent for UE (tx time = 1ms)
         *                     TTI x+0.1: ue changes master
         *                     TTI x+1: packet from old master arrives at ue
         */
    if (lteInfo->getSourceId() != masterId_)
    {
        EV << "WARNING: frame from an old master during handover: deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "Master MacNodeId: " << masterId_ << endl;
        delete frame;
        return;
    }

        // send H-ARQ feedback up
    if (lteInfo->getFrameType() == HARQPKT || lteInfo->getFrameType() == GRANTPKT || lteInfo->getFrameType() == RACPKT)
    {
        handleControlMsg(frame, lteInfo);
        return;
    }
    if ((lteInfo->getUserTxParams()) != NULL)
    {
        int cw = lteInfo->getCw();
        if (lteInfo->getUserTxParams()->readCqiVector().size() == 1)
            cw = 0;
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];
        tSample_->sample_ = cqi;
        tSample_->id_ = nodeId_;
        tSample_->module_ = getMacByMacNodeId(nodeId_);
        emit(averageCqiDl_, tSample_);
        emit(averageCqiDlvect_,cqi);
    }
    // apply decider to received packet
    bool result = true;
    RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
    if (r.size() > 1)
    {
        // DAS
        for (RemoteSet::iterator it = r.begin(); it != r.end(); it++)
        {
            EV << "LtePhy: Receiving Packet from antenna " << (*it) << "\n";

            /*
             * On UE set the sender position
             * and tx power to the sender das antenna
             */

//            cc->updateHostPosition(myHostRef,das_->getAntennaCoord(*it));
            // Set position of sender
//            Move m;
//            m.setStart(das_->getAntennaCoord(*it));
            RemoteUnitPhyData data;
            data.txPower=lteInfo->getTxPower();
            data.m=getRadioPosition();
            frame->addRemoteUnitPhyDataVector(data);
        }
        // apply analog models For DAS
        result=channelModel_->errorDas(frame,lteInfo);
    }
    else
    {
        //RELAY and NORMAL
        result = channelModel_->error(frame,lteInfo);
    }

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

void LtePhyUe::handleUpperMessage(cMessage* msg)
{
//    if (useBattery_) {
//    TODO     BatteryAccess::drawCurrent(txAmount_, 1);
//    }

    UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->getControlInfo());
    MacNodeId dest = lteInfo->getDestId();
    if (dest != masterId_)
    {
        // UE is not sending to its master!!
        EV << "ERROR: Ue preparing to send message to " << dest << "instead "
        "of its master (" << masterId_ << ")" << endl;
        endSimulation();
    }

    // Store the RBs used for transmission. For interference computation
    RbMap rbMap = lteInfo->getGrantedBlocks();
    UsedRBs info;
    info.time_ = NOW;
    info.rbMap_ = rbMap;

    usedRbs_.push_back(info);

    std::vector<UsedRBs>::iterator it = usedRbs_.begin();
    while (it != usedRbs_.end())  // purge old allocations
    {
        if (it->time_ < NOW - 0.002)
            usedRbs_.erase(it++);
        else
            ++it;
    }

    lastActive_ = NOW;

    LtePhyBase::handleUpperMessage(msg);
}

//void LtePhyUe::handleHostState(const HostState& state)  {
//    /*
//     * If a module is not using the battery, but it has a battery module,
//     * battery capacity never decreases, neither at timeouts,
//     * because a draw is never called and a draw amount for the device
//     * is never set (devices[i].currentActivity stuck at -1). See simpleBattery.cc @ line 244.
//     */
//    if (state.get() == HostState::ACTIVE)
//        return;
//
//    if (!useBattery_ && state.get() == HostState::FAILED) {
//        EV << "Warning: host state failed at node " << getName() << " while not using a battery!";
//        return;
//    }
//
//    if (state.get() == HostState::FAILED) {
//        //depleted battery
//        EV << "Battery depleted at node" << getName() << " with id " << getId();
//        //TODO: stop sending and receiving messages or just collect statistics?
//    }
//}

double LtePhyUe::updateHysteresisTh(double v)
{
    if (hysteresisFactor_ == 0)
        return 0;
    else
        return v / hysteresisFactor_;
}

void LtePhyUe::deleteOldBuffers(MacNodeId masterId)
{
    OmnetId masterOmnetId = binder_->getOmnetId(masterId);

    /* Delete Mac Buffers */

    // delete macBuffer[nodeId_] at old master
    LteMacEnb *masterMac = check_and_cast<LteMacEnb *>(getSimulation()->getModule(masterOmnetId)->
    getSubmodule("nic")->getSubmodule("mac"));
    masterMac->deleteQueues(nodeId_);

    // delete queues for master at this ue
    mac_->deleteQueues(masterId_);

    /* Delete Rlc UM Buffers */

    // delete UmTxQueue[nodeId_] at old master
    LteRlcUm *masterRlcUm = check_and_cast<LteRlcUm *>(getSimulation()->getModule(masterOmnetId)->
    getSubmodule("nic")->getSubmodule("rlc")->getSubmodule("um"));
    masterRlcUm->deleteQueues(nodeId_);

    // delete queues for master at this ue
    rlcUm_->deleteQueues(nodeId_);
}

DasFilter* LtePhyUe::getDasFilter()
{
    return das_;
}

void LtePhyUe::sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req)
{
    Enter_Method("SendFeedback");
    EV << "LtePhyUe: feedback from Feedback Generator" << endl;

    //Create a feedback packet
    LteFeedbackPkt* fbPkt = new LteFeedbackPkt();
    //Set the feedback
    fbPkt->setLteFeedbackDoubleVectorDl(fbDl);
    fbPkt->setLteFeedbackDoubleVectorDl(fbUl);
    fbPkt->setSourceNodeId(nodeId_);
    UserControlInfo* uinfo = new UserControlInfo();
    uinfo->setSourceId(nodeId_);
    uinfo->setDestId(masterId_);
    uinfo->setFrameType(FEEDBACKPKT);
    uinfo->setIsCorruptible(false);
    // create LteAirFrame and encapsulate a feedback packet
    LteAirFrame* frame = new LteAirFrame("feedback_pkt");
    frame->encapsulate(check_and_cast<cPacket*>(fbPkt));
    uinfo->feedbackReq = req;
    uinfo->setDirection(UL);
    simtime_t signalLength = TTI;
    uinfo->setTxPower(txPower_);
    // initialize frame fields

    frame->setSchedulingPriority(airFramePriority_);
    frame->setDuration(signalLength);

    uinfo->setCoord(getRadioPosition());

    frame->setControlInfo(uinfo);
    //TODO access speed data Update channel index
//    if (coherenceTime(move.getSpeed())<(NOW-lastFeedback_)){
//        deployer_->channelIncrease(nodeId_);
//        deployer_->lambdaIncrease(nodeId_,1);
//    }
    lastFeedback_ = NOW;
    EV << "LtePhy: " << nodeTypeToA(nodeType_) << " with id "
       << nodeId_ << " sending feedback to the air channel" << endl;
    sendUnicast(frame);
}
