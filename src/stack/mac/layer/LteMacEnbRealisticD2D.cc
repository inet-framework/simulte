//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteMacEnbRealisticD2D.h"
#include "LteFeedbackPkt.h"
#include "LteMacUeRealisticD2D.h"
#include "LteHarqBufferRx.h"
#include "AmcPilotD2D.h"
#include "LteSchedulerEnbUl.h"
#include "LteSchedulingGrant.h"

Define_Module(LteMacEnbRealisticD2D);

LteMacEnbRealisticD2D::LteMacEnbRealisticD2D() :
    LteMacEnbRealistic()
{
}

LteMacEnbRealisticD2D::~LteMacEnbRealisticD2D()
{
}

void LteMacEnbRealisticD2D::initialize(int stage)
{
    LteMacEnbRealistic::initialize(stage);
    if (stage == 0)
    {
        cModule* rlc = getParentModule()->getSubmodule("rlc");
        std::string rlcUmType = rlc->par("LteRlcUmType").stdstringValue();
        bool rlcD2dCapable = rlc->par("d2dCapable").boolValue();
        if (rlcUmType.compare("LteRlcUmRealistic") != 0 || !rlcD2dCapable)
            throw cRuntimeError("LteMacEnbRealisticD2D::initialize - %s module found, must be LteRlcUmRealisticD2D. Aborting", rlcUmType.c_str());
    }
    if (stage == 1)
    {
        usePreconfiguredTxParams_ = par("usePreconfiguredTxParams");
        Cqi d2dCqi = par("d2dCqi");
        if (usePreconfiguredTxParams_)
            check_and_cast<AmcPilotD2D*>(amc_->getPilot())->setPreconfiguredTxParams(d2dCqi);
    }
}

void LteMacEnbRealisticD2D::macHandleFeedbackPkt(cPacket *pkt)
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

void LteMacEnbRealisticD2D::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage() && msg->isName("D2DModeSwitchNotification"))
    {
        cPacket* pkt = check_and_cast<cPacket*>(msg);
        macHandleD2DModeSwitch(pkt);
        delete pkt;
    }
    else
        LteMacEnbRealistic::handleMessage(msg);
}


void LteMacEnbRealisticD2D::handleSelfMessage()
{
    // TODO compute conflict graph for resource allocation

    // Call the eNodeB main loop
    LteMacEnbRealistic::handleSelfMessage();
}

void LteMacEnbRealisticD2D::macPduUnmake(cPacket* pkt)
{
    LteMacPdu* macPkt = check_and_cast<LteMacPdu*>(pkt);
    while (macPkt->hasSdu())
    {
        // Extract and send SDU
        cPacket* upPkt = macPkt->popSdu();
        take(upPkt);

        // TODO: upPkt->info()
        EV << "LteMacBase: pduUnmaker extracted SDU" << endl;

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

void LteMacEnbRealisticD2D::sendGrants(LteMacScheduleList* scheduleList)
{
    EV << NOW << "LteMacEnbRealisticD2D::sendGrants " << endl;

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

        EV << NOW << " LteMacEnbRealisticD2D::sendGrants Node[" << getMacNodeId() << "] - "
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
        const unsigned int logicalBands = deployer_->getNumBands();
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
            EV << NOW << " LteMacEnbRealisticD2D::sendGrants - granting " << grantedBytes << " on cw " << cw << endl;
        }
        RbMap map;

        enbSchedulerUl_->readRbOccupation(nodeId, map);

        grant->setGrantedBlocks(map);
        // send grant to PHY layer
        sendLowerPackets(grant);
    }
}

void LteMacEnbRealisticD2D::clearBsrBuffers(MacNodeId ueId)
{
    EV << NOW << "LteMacEnbRealisticD2D::clearBsrBuffers - Clear BSR buffers of UE " << ueId << endl;

    // empty all BSR buffers belonging to the UE
    LteMacBufferMap::iterator vit = bsrbuf_.begin();
    for (; vit != bsrbuf_.end(); ++vit)
    {
        MacCid cid = vit->first;
        // check if this buffer is for this UE
        if (MacCidToNodeId(cid) != ueId)
            continue;

        EV << NOW << "LteMacEnbRealisticD2D::clearBsrBuffers - Clear BSR buffer for cid " << cid << endl;

        // empty its BSR buffer
        LteMacBuffer* buf = vit->second;
        EV << NOW << "LteMacEnbRealisticD2D::clearBsrBuffers - Length was " << buf->getQueueOccupancy() << endl;

        while (!buf->isEmpty())
            buf->popFront();

        EV << NOW << "LteMacEnbRealisticD2D::clearBsrBuffers - New length is " << buf->getQueueOccupancy() << endl;

    }
}

void LteMacEnbRealisticD2D::storeRxHarqBufferMirror(MacNodeId id, LteHarqBufferRxD2DMirror* mirbuff)
{
    // TODO optimize

    if ( harqRxBuffersD2DMirror_.find(id) != harqRxBuffersD2DMirror_.end() )
        delete harqRxBuffersD2DMirror_[id];
    harqRxBuffersD2DMirror_[id] = mirbuff;
}

HarqRxBuffersMirror* LteMacEnbRealisticD2D::getRxHarqBufferMirror()
{
    return &harqRxBuffersD2DMirror_;
}

void LteMacEnbRealisticD2D::deleteRxHarqBufferMirror(MacNodeId id)
{
    HarqRxBuffersMirror::iterator it = harqRxBuffersD2DMirror_.begin() , et=harqRxBuffersD2DMirror_.end();
    for(; it != et;)
    {
        // get current nodeIDs
        MacNodeId senderId = it->second->peerId_; // Transmitter
        MacNodeId destId = it->first;             // Receiver

        if (senderId == id || destId == id)
        {
            harqRxBuffersD2DMirror_.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

void LteMacEnbRealisticD2D::sendModeSwitchNotification(MacNodeId srcId, MacNodeId dstId, LteD2DMode oldMode, LteD2DMode newMode)
{
    Enter_Method("sendModeSwitchNotification");

    // send switch notification to both the tx and rx side of the flow
    D2DModeSwitchNotification* switchPktTx = new D2DModeSwitchNotification("D2DModeSwitchNotification");
    switchPktTx->setTxSide(true);
    switchPktTx->setPeerId(dstId);
    switchPktTx->setOldMode(oldMode);
    switchPktTx->setNewMode(newMode);
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

void LteMacEnbRealisticD2D::macHandleD2DModeSwitch(cPacket* pkt)
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
                EV << NOW << " LteMacEnbRealisticD2D::sendModeSwitchNotification - send signal for TX entity to upper layers in the eNB (cid=" << cid << ")" << endl;
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
                // interrupt H-ARQ processes for UL
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

                EV << NOW << " LteMacEnbRealisticD2D::sendModeSwitchNotification - send signal for RX entity to upper layers in the eNB (cid=" << cid << ")" << endl;
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
