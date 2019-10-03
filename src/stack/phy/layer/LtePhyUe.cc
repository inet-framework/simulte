//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <assert.h>
#include "stack/phy/layer/LtePhyUe.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "corenetwork/lteip/IP2lte.h"
#include "stack/phy/feedback/LteDlFeedbackGenerator.h"

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

    if (stage == inet::INITSTAGE_LOCAL)
    {
        nodeType_ = UE;
        useBattery_ = false;  // disabled
        enableHandover_ = par("enableHandover");
        handoverLatency_ = par("handoverLatency").doubleValue();
        dynamicCellAssociation_ = par("dynamicCellAssociation");
        currentMasterRssi_ = 0;
        candidateMasterRssi_ = 0;
        hysteresisTh_ = 0;
        hysteresisFactor_ = 10;
        handoverDelta_ = 0.00001;

        dasRssiThreshold_ = 1.0e-5;
        das_ = new DasFilter(this, binder_, NULL, dasRssiThreshold_);

        servingCell_ = registerSignal("servingCell");
        averageCqiDl_ = registerSignal("averageCqiDl");
        averageCqiUl_ = registerSignal("averageCqiUl");

        if (!hasListeners(averageCqiDl_))
            error("no phy listeners");

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
    else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
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
        rlcUm_ = check_and_cast<LteRlcUm*>(
            getParentModule()-> // nic
            getSubmodule("rlc")->
                getSubmodule("um"));
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_LAYER)
    {
        // find the best candidate master cell
        if (dynamicCellAssociation_)
        {
            // this is a fictitious frame that needs to compute the SINR
            LteAirFrame *frame = new LteAirFrame("cellSelectionFrame");
            UserControlInfo *cInfo = new UserControlInfo();

            // get the list of all eNodeBs in the network
            std::vector<EnbInfo*>* enbList = binder_->getEnbList();
            std::vector<EnbInfo*>::iterator it = enbList->begin();
            for (; it != enbList->end(); ++it)
            {
                MacNodeId cellId = (*it)->id;
                LtePhyBase* cellPhy = check_and_cast<LtePhyBase*>((*it)->eNodeB->getSubmodule("lteNic")->getSubmodule("phy"));
                double cellTxPower = cellPhy->getTxPwr();
                Coord cellPos = cellPhy->getCoord();

                // build a control info
                cInfo->setSourceId(cellId);
                cInfo->setTxPower(cellTxPower);
                cInfo->setCoord(cellPos);
                cInfo->setFrameType(FEEDBACKPKT);

                // get RSSI from the eNB
                std::vector<double>::iterator it;
                double rssi = 0;
                std::vector<double> rssiV = channelModel_->getSINR(frame, cInfo);
                for (it = rssiV.begin(); it != rssiV.end(); ++it)
                    rssi += *it;
                rssi /= rssiV.size();   // compute the mean over all RBs

                EV << "LtePhyUe::initialize - RSSI from eNodeB " << cellId << ": " << rssi << " dB (current candidate eNodeB " << candidateMasterId_ << ": " << candidateMasterRssi_ << " dB" << endl;

                if (rssi > candidateMasterRssi_)
                {
                    candidateMasterId_ = cellId;
                    candidateMasterRssi_ = rssi;
                }
            }
            delete cInfo;
            delete frame;

            // set serving cell
            masterId_ = candidateMasterId_;
            getAncestorPar("masterId").setIntValue(masterId_);
            currentMasterRssi_ = candidateMasterRssi_;
            updateHysteresisTh(candidateMasterRssi_);
        }
        else
        {
            // get serving cell from configuration
            masterId_ = getAncestorPar("masterId");
            candidateMasterId_ = masterId_;
        }
        EV << "LtePhyUe::initialize - Attaching to eNodeB " << masterId_ << endl;

        das_->setMasterRuSet(masterId_);
        emit(servingCell_, (long)masterId_);
    }
    else if (stage == inet::INITSTAGE_NETWORK_LAYER_2)
    {
        // get local id
        nodeId_ = getAncestorPar("macNodeId");
        EV << "Local MacNodeId: " << nodeId_ << endl;

        // get cellInfo at this stage because the next hop of the node is registered in the IP2Lte module at the INITSTAGE_NETWORK_LAYER
        cellInfo_ = getCellInfo(nodeId_);
        int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
        cellInfo_->lambdaInit(nodeId_, index);
        cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
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

    // inform the UE's IP2lte module to start holding downstream packets
    IP2lte* ip2lte =  check_and_cast<IP2lte*>(getParentModule()->getSubmodule("ip2lte"));
    ip2lte->triggerHandoverUe();

    // inform the eNB's IP2lte module to forward data to the target eNB
    IP2lte* enbIp2lte =  check_and_cast<IP2lte*>(getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("lteNic")->getSubmodule("ip2lte"));
    enbIp2lte->triggerHandoverSource(nodeId_,candidateMasterId_);

    handoverTrigger_ = new cMessage("handoverTrigger");
    scheduleAt(simTime() + handoverLatency_, handoverTrigger_);
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

    // update cellInfo
    LteMacEnb* newMacEnb =  check_and_cast<LteMacEnb*>(getSimulation()->getModule(binder_->getOmnetId(candidateMasterId_))->getSubmodule("lteNic")->getSubmodule("mac"));
    LteCellInfo* newCellInfo = newMacEnb->getCellInfo();
    cellInfo_->detachUser(nodeId_);
    newCellInfo->attachUser(nodeId_);
    cellInfo_ = newCellInfo;

    // update DL feedback generator
    LteDlFeedbackGenerator* fbGen = check_and_cast<LteDlFeedbackGenerator*>(getParentModule()->getSubmodule("dlFbGen"));
    fbGen->handleHandover(masterId_);

    // collect stat
    emit(servingCell_, (long)masterId_);

    EV << NOW << " LtePhyUe::doHandover - UE " << nodeId_ << " has completed handover to eNB " << masterId_ << "... " << endl;
    binder_->removeUeHandoverTriggered(nodeId_);

    // inform the UE's IP2lte module to forward held packets
    IP2lte* ip2lte =  check_and_cast<IP2lte*>(getParentModule()->getSubmodule("ip2lte"));
    ip2lte->signalHandoverCompleteUe();

    // inform the eNB's IP2lte module to forward data to the target eNB
    IP2lte* enbIp2lte =  check_and_cast<IP2lte*>(getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("lteNic")->getSubmodule("ip2lte"));
    enbIp2lte->signalHandoverCompleteTarget(nodeId_,oldMaster);
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
    EV << "LtePhy: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    int sourceId = binder_->getOmnetId(lteInfo->getSourceId());
    if(sourceId == 0 )
    {
        // source has left the simulation
        delete msg;
        return;
    }

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
        delete lteInfo;
        delete frame;
        return;
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
        emit(averageCqiDl_, cqi);
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
        result=channelModel_->isCorruptedDas(frame,lteInfo);
    }
    else
    {
        //RELAY and NORMAL
        result = channelModel_->isCorrupted(frame,lteInfo);
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

    if (lteInfo->getFrameType() == DATAPKT && (channelModel_->isUplinkInterferenceEnabled() || channelModel_->isD2DInterferenceEnabled()))
    {
        // Store the RBs used for data transmission to the binder (for UL interference computation)
        RbMap rbMap = lteInfo->getGrantedBlocks();
        Remote antenna = MACRO;  // TODO fix for multi-antenna
        binder_->storeUlTransmissionMap(antenna, rbMap, nodeId_, mac_->getMacCellId(), this, UL);
    }

    if (lteInfo->getFrameType() == DATAPKT && lteInfo->getUserTxParams() != NULL)
    {
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[lteInfo->getCw()];
        if (lteInfo->getDirection() == UL)
            emit(averageCqiUl_, cqi);
        else if (lteInfo->getDirection() == D2D)
            emit(averageCqiD2D_, cqi);
    }

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
    getSubmodule("lteNic")->getSubmodule("mac"));
    masterMac->deleteQueues(nodeId_);

    // delete queues for master at this ue
    mac_->deleteQueues(masterId_);

    /* Delete Rlc UM Buffers */

    // delete UmTxQueue[nodeId_] at old master
    LteRlcUm *masterRlcUm = check_and_cast<LteRlcUm*>(getSimulation()->getModule(masterOmnetId)->
    getSubmodule("lteNic")->getSubmodule("rlc")->getSubmodule("um"));
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
//        cellInfo_->channelIncrease(nodeId_);
//        cellInfo_->lambdaIncrease(nodeId_,1);
//    }
    lastFeedback_ = NOW;
    EV << "LtePhy: " << nodeTypeToA(nodeType_) << " with id "
       << nodeId_ << " sending feedback to the air channel" << endl;
    sendUnicast(frame);
}

void LtePhyUe::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH)
    {
        // do this only at deletion of the module during the simulation

        // clear buffers
        deleteOldBuffers(masterId_);

        // amc calls
        LteAmc *amc = getAmcModule(masterId_);
        if (amc != NULL)
        {
            amc->detachUser(nodeId_, UL);
            amc->detachUser(nodeId_, DL);
        }

        // binder call
        binder_->unregisterNextHop(masterId_, nodeId_);

        // cellInfo call
        cellInfo_->detachUser(nodeId_);
    }
}
