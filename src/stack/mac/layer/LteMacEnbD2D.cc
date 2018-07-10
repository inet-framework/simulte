//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/layer/LteMacEnbD2D.h"
#include "stack/mac/layer/LteMacUeD2D.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/amc/AmcPilotD2D.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/conflict_graph/DistanceBasedConflictGraph.h"

Define_Module(LteMacEnbD2D);

LteMacEnbD2D::LteMacEnbD2D() :
    LteMacEnb()
{
}

LteMacEnbD2D::~LteMacEnbD2D()
{
}

void LteMacEnbD2D::initialize(int stage)
{
    LteMacEnb::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        cModule* rlc = getParentModule()->getSubmodule("rlc");
        std::string rlcUmType = rlc->par("LteRlcUmType").stdstringValue();
        bool rlcD2dCapable = rlc->par("d2dCapable").boolValue();
        if (rlcUmType.compare("LteRlcUm") != 0 || !rlcD2dCapable)
            throw cRuntimeError("LteMacEnbD2D::initialize - %s module found, must be LteRlcUmD2D. Aborting", rlcUmType.c_str());
    }
    else if (stage == INITSTAGE_LOCAL + 1)
    {
        usePreconfiguredTxParams_ = par("usePreconfiguredTxParams");
        Cqi d2dCqi = par("d2dCqi");
        if (usePreconfiguredTxParams_)
            check_and_cast<AmcPilotD2D*>(amc_->getPilot())->setPreconfiguredTxParams(d2dCqi);

        msHarqInterrupt_ = par("msHarqInterrupt").boolValue();
        msClearRlcBuffer_ = par("msClearRlcBuffer").boolValue();
    }
    else if (stage == INITSTAGE_LAST)  // be sure that all UEs have been initialized
    {
        reuseD2D_ = par("reuseD2D");
        reuseD2DMulti_ = par("reuseD2DMulti");

        if (reuseD2D_ || reuseD2DMulti_)
        {
            conflictGraphUpdatePeriod_ = par("conflictGraphUpdatePeriod");

            CGType cgType = CG_DISTANCE;  // TODO make this parametric
            switch(cgType)
            {
                case CG_DISTANCE:
                {
                    conflictGraph_ = new DistanceBasedConflictGraph(this, reuseD2D_, reuseD2DMulti_, par("conflictGraphThreshold"));
                    check_and_cast<DistanceBasedConflictGraph*>(conflictGraph_)->setThresholds(par("conflictGraphD2DInterferenceRadius"), par("conflictGraphD2DMultiTxRadius"), par("conflictGraphD2DMultiInterferenceRadius"));
                    break;
                }
                default: { throw cRuntimeError("LteMacEnbD2D::initialize - CG type unknown. Aborting"); }
            }

            scheduleAt(NOW + 0.05, new cMessage("updateConflictGraph"));
        }
    }
}

void LteMacEnbD2D::macHandleFeedbackPkt(cPacket *pkt)
{
    LteFeedbackPkt* fb = check_and_cast<LteFeedbackPkt*>(pkt);
    std::map<MacNodeId, LteFeedbackDoubleVector> fbMapD2D = fb->getLteFeedbackDoubleVectorD2D();

    // skip if no D2D CQI has been reported
    if (!fbMapD2D.empty())
    {
        //get Source Node Id<
        MacNodeId id = fb->getSourceNodeId();
        std::map<MacNodeId, LteFeedbackDoubleVector>::iterator mapIt;
        LteFeedbackDoubleVector::iterator it;
        LteFeedbackVector::iterator jt;

        // extract feedback for D2D links
        for (mapIt = fbMapD2D.begin(); mapIt != fbMapD2D.end(); ++mapIt)
        {
            MacNodeId peerId = mapIt->first;
            for (it = mapIt->second.begin(); it != mapIt->second.end(); ++it)
            {
                for (jt = it->begin(); jt != it->end(); ++jt)
                {
                    if (!jt->isEmptyFeedback())
                    {
                        amc_->pushFeedbackD2D(id, (*jt), peerId);
                    }
                }
            }
        }
    }
    LteMacEnb::macHandleFeedbackPkt(pkt);
}

void LteMacEnbD2D::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage() && msg->isName("D2DModeSwitchNotification"))
    {
        cPacket* pkt = check_and_cast<cPacket*>(msg);
        macHandleD2DModeSwitch(pkt);
        delete pkt;
    }
    else if (msg->isSelfMessage() && msg->isName("updateConflictGraph"))
    {
        // compute conflict graph for resource allocation
        conflictGraph_->computeConflictGraph();

//        // debug
//        conflictGraph_->printConflictGraph();

        scheduleAt(NOW + conflictGraphUpdatePeriod_, msg);
    }
    else
        LteMacEnb::handleMessage(msg);
}


void LteMacEnbD2D::handleSelfMessage()
{
    // Call the eNodeB main loop
    LteMacEnb::handleSelfMessage();
}

void LteMacEnbD2D::macPduUnmake(cPacket* pkt)
{
    LteMacPdu* macPkt = check_and_cast<LteMacPdu*>(pkt);
    while (macPkt->hasSdu())
    {
        // Extract and send SDU
        cPacket* upPkt = macPkt->popSdu();
        take(upPkt);

        // TODO: upPkt->info()
        EV << "LteMacEnbD2D: pduUnmaker extracted SDU" << endl;

        // store descriptor for the incoming connection, if not already stored
        FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(upPkt->getControlInfo());
        MacNodeId senderId = lteInfo->getSourceId();
        LogicalCid lcid = lteInfo->getLcid();
        MacCid cid = idToMacCid(senderId, lcid);
        if (connDescIn_.find(cid) == connDescIn_.end())
        {
            FlowControlInfo toStore(*lteInfo);
            connDescIn_[cid] = toStore;
        }

        sendUpperPackets(upPkt);
    }

    while (macPkt->hasCe())
    {
        // Extract CE
        // TODO: vedere se   per cid o lcid
        MacBsr* bsr = check_and_cast<MacBsr*>(macPkt->popCe());
        UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(macPkt->getControlInfo());
        LogicalCid lcid = lteInfo->getLcid();  // one of SHORT_BSR or D2D_MULTI_SHORT_BSR

        MacCid cid = idToMacCid(lteInfo->getSourceId(), lcid); // this way, different connections from the same UE (e.g. one UL and one D2D)
                                                               // obtain different CIDs. With the inverse operation, you can get
                                                               // the LCID and discover if the connection is UL or D2D
        bufferizeBsr(bsr, cid);
    }

    delete macPkt;
}

void LteMacEnbD2D::sendGrants(LteMacScheduleList* scheduleList)
{
    EV << NOW << "LteMacEnbD2D::sendGrants " << endl;

    while (!scheduleList->empty())
    {
        LteMacScheduleList::iterator it, ot;
        it = scheduleList->begin();

        Codeword cw = it->first.second;
        Codeword otherCw = MAX_CODEWORDS - cw;

//        MacNodeId nodeId = it->first.first;
        MacCid cid = it->first.first;
        LogicalCid lcid = MacCidToLcid(cid);
        MacNodeId nodeId = MacCidToNodeId(cid);
        unsigned int granted = it->second;
        unsigned int codewords = 0;

        // removing visited element from scheduleList.
        scheduleList->erase(it);

        if (granted > 0)
        {
            // increment number of allocated Cw
            ++codewords;
        }
        else
        {
            // active cw becomes the "other one"
            cw = otherCw;
        }

        std::pair<unsigned int, Codeword> otherPair(nodeId, otherCw);

        if ((ot = (scheduleList->find(otherPair))) != (scheduleList->end()))
        {
            // increment number of allocated Cw
            ++codewords;

            // removing visited element from scheduleList.
            scheduleList->erase(ot);
        }

        if (granted == 0)
            continue; // avoiding transmission of 0 grant (0 grant should not be created)

        EV << NOW << " LteMacEnbD2D::sendGrants Node[" << getMacNodeId() << "] - "
           << granted << " blocks to grant for user " << nodeId << " on "
           << codewords << " codewords. CW[" << cw << "\\" << otherCw << "]" << endl;

        // get the direction of the grant, depending on which connection has been scheduled by the eNB
        Direction dir = (lcid == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : ((lcid == D2D_SHORT_BSR) ? D2D : UL);

        // TODO Grant is set aperiodic as default
        LteSchedulingGrant* grant = new LteSchedulingGrant("LteGrant");
        grant->setDirection(dir);
        grant->setCodewords(codewords);

        // set total granted blocks
        grant->setTotalGrantedBlocks(granted);

        UserControlInfo* uinfo = new UserControlInfo();
        uinfo->setSourceId(getMacNodeId());
        uinfo->setDestId(nodeId);
        uinfo->setFrameType(GRANTPKT);

        grant->setControlInfo(uinfo);

        const UserTxParams& ui = getAmc()->computeTxParams(nodeId, dir);
        UserTxParams* txPara = new UserTxParams(ui);
        // FIXME: possible memory leak
        grant->setUserTxParams(txPara);

        // acquiring remote antennas set from user info
        const std::set<Remote>& antennas = ui.readAntennaSet();
        std::set<Remote>::const_iterator antenna_it = antennas.begin(),
        antenna_et = antennas.end();
        const unsigned int logicalBands = cellInfo_->getNumBands();
        //  HANDLE MULTICW
        for (; cw < codewords; ++cw)
        {
            unsigned int grantedBytes = 0;

            for (Band b = 0; b < logicalBands; ++b)
            {
                unsigned int bandAllocatedBlocks = 0;
               // for (; antenna_it != antenna_et; ++antenna_it) // OLD FOR
               for (antenna_it = antennas.begin(); antenna_it != antenna_et; ++antenna_it)
               {
                   bandAllocatedBlocks += enbSchedulerUl_->readPerUeAllocatedBlocks(nodeId,*antenna_it, b);
               }
               grantedBytes += amc_->computeBytesOnNRbs(nodeId, b, cw, bandAllocatedBlocks, dir );
            }

            grant->setGrantedCwBytes(cw, grantedBytes);
            EV << NOW << " LteMacEnbD2D::sendGrants - granting " << grantedBytes << " on cw " << cw << endl;
        }
        RbMap map;

        enbSchedulerUl_->readRbOccupation(nodeId, map);

        grant->setGrantedBlocks(map);
        // send grant to PHY layer
        sendLowerPackets(grant);
    }
}

void LteMacEnbD2D::clearBsrBuffers(MacNodeId ueId)
{
    EV << NOW << "LteMacEnbD2D::clearBsrBuffers - Clear BSR buffers of UE " << ueId << endl;

    // empty all BSR buffers belonging to the UE
    LteMacBufferMap::iterator vit = bsrbuf_.begin();
    for (; vit != bsrbuf_.end(); ++vit)
    {
        MacCid cid = vit->first;
        // check if this buffer is for this UE
        if (MacCidToNodeId(cid) != ueId)
            continue;

        EV << NOW << "LteMacEnbD2D::clearBsrBuffers - Clear BSR buffer for cid " << cid << endl;

        // empty its BSR buffer
        LteMacBuffer* buf = vit->second;
        EV << NOW << "LteMacEnbD2D::clearBsrBuffers - Length was " << buf->getQueueOccupancy() << endl;

        while (!buf->isEmpty())
            buf->popFront();

        EV << NOW << "LteMacEnbD2D::clearBsrBuffers - New length is " << buf->getQueueOccupancy() << endl;

    }
}

HarqBuffersMirrorD2D* LteMacEnbD2D::getHarqBuffersMirrorD2D()
{
    return &harqBuffersMirrorD2D_;
}

void LteMacEnbD2D::deleteQueues(MacNodeId nodeId)
{
    LteMacEnb::deleteQueues(nodeId);
    deleteHarqBuffersMirrorD2D(nodeId);
}

void LteMacEnbD2D::deleteHarqBuffersMirrorD2D(MacNodeId nodeId)
{
    // delete all "mirror" buffers that have nodeId as sender or receiver
    HarqBuffersMirrorD2D::iterator it = harqBuffersMirrorD2D_.begin() , et=harqBuffersMirrorD2D_.end();
    for(; it != et;)
    {
        // get current nodeIDs
        MacNodeId senderId = (it->first).first; // Transmitter
        MacNodeId destId = (it->first).second;  // Receiver

        if (senderId == nodeId || destId == nodeId)
        {
            delete it->second;
            harqBuffersMirrorD2D_.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

void LteMacEnbD2D::deleteHarqBuffersMirrorD2D(MacNodeId txPeer, MacNodeId rxPeer)
{
    // delete all "mirror" buffers that have nodeId as sender or receiver
    HarqBuffersMirrorD2D::iterator it = harqBuffersMirrorD2D_.begin() , et=harqBuffersMirrorD2D_.end();
    for(; it != et;)
    {
        // get current nodeIDs
        MacNodeId senderId = (it->first).first; // Transmitter
        MacNodeId destId = (it->first).second;  // Receiver

        if (senderId == txPeer && destId == rxPeer)
        {
            delete it->second;
            harqBuffersMirrorD2D_.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

void LteMacEnbD2D::sendModeSwitchNotification(MacNodeId srcId, MacNodeId dstId, LteD2DMode oldMode, LteD2DMode newMode)
{
    Enter_Method_Silent("sendModeSwitchNotification");

    EV << NOW << " LteMacEnbD2D::sendModeSwitchNotification - " << srcId << " --> " << dstId << " going from " << d2dModeToA(oldMode) << " to " << d2dModeToA(newMode) << endl;

    // send switch notification to both the tx and rx side of the flow
    D2DModeSwitchNotification* switchPktTx = new D2DModeSwitchNotification("D2DModeSwitchNotification");
    switchPktTx->setTxSide(true);
    switchPktTx->setPeerId(dstId);
    switchPktTx->setOldMode(oldMode);
    switchPktTx->setNewMode(newMode);
    switchPktTx->setInterruptHarq(msHarqInterrupt_);
    switchPktTx->setClearRlcBuffer(msClearRlcBuffer_);
    UserControlInfo* uinfoTx = new UserControlInfo();
    uinfoTx->setSourceId(nodeId_);
    uinfoTx->setDestId(srcId);
    uinfoTx->setFrameType(D2DMODESWITCHPKT);
    switchPktTx->setControlInfo(uinfoTx);
    sendLowerPackets(switchPktTx);

    D2DModeSwitchNotification* switchPktRx = new D2DModeSwitchNotification("D2DModeSwitchNotification");
    switchPktRx->setTxSide(false);
    switchPktRx->setPeerId(srcId);
    switchPktRx->setOldMode(oldMode);
    switchPktRx->setNewMode(newMode);
    switchPktRx->setInterruptHarq(msHarqInterrupt_);
    switchPktTx->setClearRlcBuffer(msClearRlcBuffer_);
    UserControlInfo* uinfoRx = new UserControlInfo();
    uinfoRx->setSourceId(nodeId_);
    uinfoRx->setDestId(dstId);
    uinfoRx->setFrameType(D2DMODESWITCHPKT);
    switchPktRx->setControlInfo(uinfoRx);
    sendLowerPackets(switchPktRx);

    // schedule handling of the mode switch at the eNodeB (in one TTI)
    D2DModeSwitchNotification* switchPktTx_local = switchPktTx->dup();
    D2DModeSwitchNotification* switchPktRx_local = switchPktRx->dup();
    switchPktTx_local->setControlInfo(uinfoTx->dup());
    switchPktRx_local->setControlInfo(uinfoRx->dup());
    scheduleAt(NOW+TTI, switchPktTx_local);
    scheduleAt(NOW+TTI, switchPktRx_local);
}

void LteMacEnbD2D::macHandleD2DModeSwitch(cPacket* pkt)
{
    D2DModeSwitchNotification* switchPkt = check_and_cast<D2DModeSwitchNotification*>(pkt);
    UserControlInfo* uinfo = check_and_cast<UserControlInfo*>(switchPkt->getControlInfo());
    MacNodeId nodeId = uinfo->getDestId();
    LteD2DMode oldMode = switchPkt->getOldMode();

    if (!switchPkt->getTxSide())   // address the receiving endpoint of the D2D flow (tx entities at the eNB)
    {
        // get the outgoing connection corresponding to the DL connection for the RX endpoint of the D2D flow
        std::map<MacCid, FlowControlInfo>::iterator jt = connDesc_.begin();
        for (; jt != connDesc_.end(); ++jt)
        {
            MacCid cid = jt->first;
            FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(&(jt->second));
            if (MacCidToNodeId(cid) == nodeId)
            {
                EV << NOW << " LteMacEnbD2D::sendModeSwitchNotification - send signal for TX entity to upper layers in the eNB (cid=" << cid << ")" << endl;
                D2DModeSwitchNotification* switchPktTx = switchPkt->dup();
                switchPktTx->setTxSide(true);
                if (oldMode == IM)
                    switchPktTx->setOldConnection(true);
                else
                    switchPktTx->setOldConnection(false);
                switchPktTx->setControlInfo(lteInfo->dup());
                sendUpperPackets(switchPktTx);
                break;
            }
        }
    }
    else   // tx side: address the transmitting endpoint of the D2D flow (rx entities at the eNB)
    {
        // clear BSR buffers for the UE
        clearBsrBuffers(nodeId);

        // get the incoming connection corresponding to the UL connection for the TX endpoint of the D2D flow
        std::map<MacCid, FlowControlInfo>::iterator jt = connDescIn_.begin();
        for (; jt != connDescIn_.end(); ++jt)
        {
            MacCid cid = jt->first;
            FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(&(jt->second));
            if (MacCidToNodeId(cid) == nodeId)
            {
                if (msHarqInterrupt_) // interrupt H-ARQ processes for UL
                {
                    HarqRxBuffers::iterator hit = harqRxBuffers_.find(nodeId);
                    if (hit != harqRxBuffers_.end())
                    {
                        for (unsigned int proc = 0; proc < (unsigned int) ENB_RX_HARQ_PROCESSES; proc++)
                        {
                            unsigned int numUnits = hit->second->getProcess(proc)->getNumHarqUnits();
                            for (unsigned int i=0; i < numUnits; i++)
                            {
                                hit->second->getProcess(proc)->purgeCorruptedPdu(i); // delete contained PDU
                                hit->second->getProcess(proc)->resetCodeword(i);     // reset unit
                            }
                        }
                    }

                    // notify that this UE is switching during this TTI
                    resetHarq_[nodeId] = NOW;
                }

                EV << NOW << " LteMacEnbD2D::sendModeSwitchNotification - send signal for RX entity to upper layers in the eNB (cid=" << cid << ")" << endl;
                D2DModeSwitchNotification* switchPktRx = switchPkt->dup();
                switchPktRx->setTxSide(false);
                if (oldMode == IM)
                    switchPktRx->setOldConnection(true);
                else
                    switchPktRx->setOldConnection(false);
                switchPktRx->setControlInfo(lteInfo->dup());
                sendUpperPackets(switchPktRx);
                break;
            }
        }
    }
}

void LteMacEnbD2D::flushHarqBuffers()
{
    HarqTxBuffers::iterator it;
    for (it = harqTxBuffers_.begin(); it != harqTxBuffers_.end(); it++)
        it->second->sendSelectedDown();

    // flush mirror buffer
    HarqBuffersMirrorD2D::iterator mit;
    for (mit = harqBuffersMirrorD2D_.begin(); mit != harqBuffersMirrorD2D_.end(); mit++)
        mit->second->markSelectedAsWaiting();
}

/*
 * Lower layer handler
 */
void LteMacEnbD2D::fromPhy(cPacket *pkt)
{
    // TODO: harq test (comment fromPhy: it has only to pass pdus to proper rx buffer and
    // to manage H-ARQ feedback)
    UserControlInfo *userInfo = check_and_cast<UserControlInfo *>(pkt->getControlInfo());
    if (userInfo->getFrameType() == HARQPKT)
    {
        MacNodeId src = userInfo->getSourceId();

        // this feedback refers to a mirrored H-ARQ buffer
        LteHarqFeedback *hfbpkt = check_and_cast<LteHarqFeedback *>(pkt);
        if (!hfbpkt->getD2dFeedback())   // this is not a mirror feedback
        {
            LteMacBase::fromPhy(pkt);
            return;
        }

        // H-ARQ feedback, send it to mirror buffer of the D2D pair
        MacNodeId d2dSender = check_and_cast<LteHarqFeedbackMirror*>(hfbpkt)->getD2dSenderId();
        MacNodeId d2dReceiver = check_and_cast<LteHarqFeedbackMirror*>(hfbpkt)->getD2dReceiverId();
        D2DPair pair(d2dSender, d2dReceiver);
        HarqBuffersMirrorD2D::iterator hit = harqBuffersMirrorD2D_.find(pair);
        EV << NOW << "LteMacEnbD2D::fromPhy - node " << nodeId_ << " Received HARQ Feedback pkt (mirrored)" << endl;
        if (hit == harqBuffersMirrorD2D_.end())
        {
            // if a feedback arrives, a buffer should exists (unless it is an handover scenario
            // where the harq buffer was deleted but a feedback was in transit)
            // this case must be taken care of
            if (binder_->hasUeHandoverTriggered(src))
                return;

            // create buffer
            LteHarqBufferMirrorD2D* hb = new LteHarqBufferMirrorD2D((unsigned int) UE_TX_HARQ_PROCESSES, (unsigned char)par("maxHarqRtx"));
            harqBuffersMirrorD2D_[pair] = hb;
            hb->receiveHarqFeedback(check_and_cast<LteHarqFeedbackMirror*>(hfbpkt));
        }
        else
        {
            hit->second->receiveHarqFeedback(check_and_cast<LteHarqFeedbackMirror*>(hfbpkt));
        }
    }
    else
    {
        LteMacBase::fromPhy(pkt);
    }
}
