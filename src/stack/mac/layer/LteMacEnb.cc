//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/layer/LteMacUe.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/mac/scheduler/LteSchedulerEnbDl.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/amc/UserTxParams.h"
#include "stack/mac/packet/LteRac_m.h"
#include "common/LteCommon.h"

Define_Module( LteMacEnb);

/*********************
 * PUBLIC FUNCTIONS
 *********************/

LteMacEnb::LteMacEnb() :
    LteMacBase()
{
    deployer_ = NULL;
    amc_ = NULL;
    enbSchedulerDl_ = NULL;
    enbSchedulerUl_ = NULL;
    numAntennas_ = 0;
    bsrbuf_.clear();
    currentSubFrameType_ = NORMAL_FRAME_TYPE;
    nodeType_ = ENODEB;
    frameIndex_ = 0;
    lastTtiAllocatedRb_ = 0;
}

LteMacEnb::~LteMacEnb()
{
    delete amc_;
    delete enbSchedulerDl_;
    delete enbSchedulerUl_;

    LteMacBufferMap::iterator bit;
    for (bit = bsrbuf_.begin(); bit != bsrbuf_.end(); bit++)
        delete bit->second;
    bsrbuf_.clear();
}

/***********************
 * PROTECTED FUNCTIONS
 ***********************/

LteDeployer* LteMacEnb::getDeployer()
{
    // Get local deployer
    if (deployer_ != NULL)
        return deployer_;

    return check_and_cast<LteDeployer*>(getParentModule()-> // Stack
    getParentModule()-> // Enb
    getSubmodule("deployer")); // Deployer
}

int LteMacEnb::getNumAntennas()
{
    /* Get number of antennas: +1 is for MACRO */
    return deployer_->getNumRus() + 1;
}

SchedDiscipline LteMacEnb::getSchedDiscipline(Direction dir)
{
    if (dir == DL)
        return aToSchedDiscipline(
            par("schedulingDisciplineDl").stdstringValue());
    else if (dir == UL)
        return aToSchedDiscipline(
            par("schedulingDisciplineUl").stdstringValue());
    else
    {
        throw cRuntimeError("LteMacEnb::getSchedDiscipline(): unrecognized direction %d", (int) dir);
    }
}

double LteMacEnb::getLteAdeadline(LteTrafficClass tClass, Direction dir)
{
    std::string a("lteAdeadline");
    a.append(lteTrafficClassToA(tClass));
    a.append(dirToA(dir));
    return par(a.c_str());
}

double LteMacEnb::getLteAslackTerm(LteTrafficClass tClass, Direction dir)
{
    std::string a("lteAslackTerm");
    a.append(lteTrafficClassToA(tClass));
    a.append(dirToA(dir));
    return par(a.c_str());
}

int LteMacEnb::getLteAmaxUrgentBurst(LteTrafficClass tClass, Direction dir)
{
    std::string a("lteAmaxUrgentBurst");
    a.append(lteTrafficClassToA(tClass));
    a.append(dirToA(dir));
    return par(a.c_str());
}
int LteMacEnb::getLteAmaxFairnessBurst(LteTrafficClass tClass, Direction dir)
{
    std::string a("lteAmaxFairnessBurst");
    a.append(lteTrafficClassToA(tClass));
    a.append(dirToA(dir));
    return par(a.c_str());
}

int LteMacEnb::getLteAhistorySize(Direction dir)
{
    std::string a("lteAhistorySize");
    a.append(dirToA(dir));
    return par(a.c_str());
}

int LteMacEnb::getLteAgainHistoryTh(LteTrafficClass tClass, Direction dir)
{
    std::string a("lteAgainHistoryTh");
    a.append(lteTrafficClassToA(tClass));
    a.append(dirToA(dir));
    return par(a.c_str());
}

double LteMacEnb::getZeroLevel(Direction dir, LteSubFrameType type)
{
    std::string a("zeroLevel");
    a.append(SubFrameTypeToA(type));
    a.append(dirToA(dir));
    return par(a.c_str());
}

double LteMacEnb::getIdleLevel(Direction dir, LteSubFrameType type)
{
    std::string a("idleLevel");
    a.append(SubFrameTypeToA(type));
    a.append(dirToA(dir));
    return par(a.c_str());
}

double LteMacEnb::getPowerUnit(Direction dir, LteSubFrameType type)
{
    std::string a("powerUnit");
    a.append(SubFrameTypeToA(type));
    a.append(dirToA(dir));
    return par(a.c_str());
}
double LteMacEnb::getMaxPower(Direction dir, LteSubFrameType type)
{
    std::string a("maxPower");
    a.append(SubFrameTypeToA(type));
    a.append(dirToA(dir));
    return par(a.c_str());
}

unsigned int LteMacEnb::getAllocationRbs(Direction dir)
{
    if (dir == DL)
    {
        return par("lteAallocationRbsDl");
    }
    else
        return par("lteAallocationRbsUl");
}

bool LteMacEnb::getPfTmsAwareFlag(Direction dir)
{
    if (dir == DL)
        return par("pfTmsAwareDL");
    else
        return par("pfTmsAwareUL");
}

void LteMacEnb::deleteQueues(MacNodeId nodeId)
{
    LteMacBase::deleteQueues(nodeId);

    LteMacBufferMap::iterator bit;
    for (bit = bsrbuf_.begin(); bit != bsrbuf_.end();)
    {
        if (MacCidToNodeId(bit->first) == nodeId)
        {
            delete bit->second; // Delete Queue
            bsrbuf_.erase(bit++); // Delete Elem
        }
        else
        {
            ++bit;
        }
    }

    //update harq status in schedulers
//    enbSchedulerDl_->updateHarqDescs();
//    enbSchedulerUl_->updateHarqDescs();

    // remove active connections from the schedulers
    enbSchedulerDl_->removeActiveConnections(nodeId);
    enbSchedulerUl_->removeActiveConnections(nodeId);

    // remove pending RAC requests
    enbSchedulerUl_->removePendingRac(nodeId);
}

void LteMacEnb::initialize(int stage)
{
    LteMacBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
        nodeId_ = getAncestorPar("macNodeId");

        cellId_ = nodeId_;

        // TODO: read NED parameters, when will be present
        deployer_ = getDeployer();
        /* Get num RB Dl */
        numRbDl_ = deployer_->getNumRbDl();
        /* Get num RB Ul */
        numRbUl_ = deployer_->getNumRbUl();

        /* Get number of antennas */
        numAntennas_ = getNumAntennas();

        /* Create and initialize MAC Downlink scheduler */
        if (enbSchedulerDl_ == NULL)
        {
            enbSchedulerDl_ = new LteSchedulerEnbDl();
            enbSchedulerDl_->initialize(DL, this);
        }

        /* Create and initialize MAC Uplink scheduler */
        if (enbSchedulerUl_ == NULL)
        {
            enbSchedulerUl_ = new LteSchedulerEnbUl();
            enbSchedulerUl_->initialize(UL, this);
        }
        //Initialize the current sub frame type with the first subframe of the MBSFN pattern
        currentSubFrameType_ = NORMAL_FRAME_TYPE;

        activatedFrames_ = registerSignal("activatedFrames");
        sleepFrames_ = registerSignal("sleepFrames");
        wastedFrames_ = registerSignal("wastedFrames");

        eNodeBCount = par("eNodeBCount");
        WATCH(numAntennas_);
        WATCH_MAP(bsrbuf_);
    }
    else if (stage == 1)
    {
        /* Create and initialize AMC module */
        amc_ = new LteAmc(this, binder_, deployer_, numAntennas_);

        /* Insert EnbInfo in the Binder */
        EnbInfo* info = new EnbInfo();
        info->id = nodeId_;            // local mac ID
        info->type = MACRO_ENB;        // eNb Type
        info->init = false;            // flag for phy initialization
        info->eNodeB = this->getParentModule()->getParentModule();  // reference to the eNodeB module
        binder_->addEnbInfo(info);

        // register the pair <id,name> to the binder
        const char* moduleName = getParentModule()->getParentModule()->getName();
        binder_->registerName(nodeId_, moduleName);
    }
}

void LteMacEnb::handleMessage(cMessage *msg)
{
    LteMacBase::handleMessage(msg);
}


void LteMacEnb::bufferizeBsr(MacBsr* bsr, MacCid cid)
{
    PacketInfo vpkt(bsr->getSize(), bsr->getTimestamp());

    LteMacBufferMap::iterator it = bsrbuf_.find(cid);
    if (it == bsrbuf_.end())
    {
        // Queue not found for this cid: create
        LteMacBuffer* bsrqueue = new LteMacBuffer();

        bsrqueue->pushBack(vpkt);
        bsrbuf_[cid] = bsrqueue;

        EV << "LteBsrBuffers : Added new BSR buffer for node: "
           << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid)
           << " Current BSR size: " << bsr->getSize() << "\n";
    }
    else
    {
        // Found
        LteMacBuffer* bsrqueue = it->second;
        bsrqueue->pushBack(vpkt);

        EV << "LteBsrBuffers : Using old buffer for node: " << MacCidToNodeId(
            cid) << " for Lcid: " << MacCidToLcid(cid)
           << " Current BSR size: " << bsr->getSize() << "\n";
    }

        // signal backlog to Uplink scheduler
    enbSchedulerUl_->backlog(cid);
}

void LteMacEnb::sendGrants(LteMacScheduleList* scheduleList)
{
    EV << NOW << "LteMacEnb::sendGrants " << endl;

    while (!scheduleList->empty())
    {
        LteMacScheduleList::iterator it, ot;
        it = scheduleList->begin();

        Codeword cw = it->first.second;
        Codeword otherCw = MAX_CODEWORDS - cw;

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

        EV << NOW << " LteMacEnb::sendGrants Node[" << getMacNodeId() << "] - "
           << granted << " blocks to grant for user " << nodeId << " on "
           << codewords << " codewords. CW[" << cw << "\\" << otherCw << "]" << endl;

        // TODO Grant is set aperiodic as default
        LteSchedulingGrant* grant = new LteSchedulingGrant("LteGrant");

        grant->setDirection(UL);

        grant->setCodewords(codewords);

        // set total granted blocks
        grant->setTotalGrantedBlocks(granted);

        UserControlInfo* uinfo = new UserControlInfo();
        uinfo->setSourceId(getMacNodeId());
        uinfo->setDestId(nodeId);
        uinfo->setFrameType(GRANTPKT);

        grant->setControlInfo(uinfo);

        // get and set the user's UserTxParams
        const UserTxParams& ui = getAmc()->computeTxParams(nodeId, UL);
        UserTxParams* txPara = new UserTxParams(ui);
        grant->setUserTxParams(txPara);

        // acquiring remote antennas set from user info
        const std::set<Remote>& antennas = ui.readAntennaSet();
        std::set<Remote>::const_iterator antenna_it, antenna_et = antennas.end();
        const unsigned int logicalBands = deployer_->getNumBands();

        //  HANDLE MULTICW
        for (; cw < codewords; ++cw)
        {
            unsigned int grantedBytes = 0;

            for (Band b = 0; b < logicalBands; ++b)
            {
                unsigned int bandAllocatedBlocks = 0;

                for (antenna_it = antennas.begin(); antenna_it != antenna_et; ++antenna_it)
                {
                    bandAllocatedBlocks += enbSchedulerUl_->readPerUeAllocatedBlocks(nodeId,
                        *antenna_it, b);
                }

                grantedBytes += amc_->computeBytesOnNRbs(nodeId, b, cw,
                    bandAllocatedBlocks, UL);
            }

            grant->setGrantedCwBytes(cw, grantedBytes);
            EV << NOW << " LteMacEnb::sendGrants - granting " << grantedBytes << " on cw " << cw << endl;
        }

        RbMap map;

        enbSchedulerUl_->readRbOccupation(nodeId, map);

        grant->setGrantedBlocks(map);

        // send grant to PHY layer
        sendLowerPackets(grant);
    }
}

void LteMacEnb::macHandleRac(cPacket* pkt)
{
    EV << NOW << " LteMacEnb::macHandleRac" << endl;

    LteRac* racPkt = check_and_cast<LteRac*> (pkt);
    UserControlInfo* uinfo = check_and_cast<UserControlInfo*> (
        racPkt->getControlInfo());

    enbSchedulerUl_->signalRac(uinfo->getSourceId());

    // TODO all RACs are marked are successful
    racPkt->setSuccess(true);

    uinfo->setDestId(uinfo->getSourceId());
    uinfo->setSourceId(nodeId_);
    uinfo->setDirection(DL);

    sendLowerPackets(racPkt);
}

void LteMacEnb::macPduMake(LteMacScheduleList* scheduleList)
{
    EV << "----- START LteMacEnb::macPduMake -----\n";
    // Finalizes the scheduling decisions according to the schedule list,
    // detaching sdus from real buffers.

    macPduList_.clear();

    //  Build a MAC pdu for each scheduled user on each codeword
    LteMacScheduleList::const_iterator it;
    for (it = scheduleList->begin(); it != scheduleList->end(); it++)
    {
        LteMacPdu* macPkt;
        cPacket* pkt;
        MacCid destCid = it->first.first;
        Codeword cw = it->first.second;
        MacNodeId destId = MacCidToNodeId(destCid);
        std::pair<MacNodeId, Codeword> pktId = std::pair<MacNodeId, Codeword>(
            destId, cw);
        unsigned int sduPerCid = it->second;
        unsigned int grantedBlocks = 0;
        TxMode txmode;
        while (sduPerCid > 0)
        {
            if ((mbuf_[destCid]->getQueueLength()) < (int) sduPerCid)
            {
                throw cRuntimeError("Abnormal queue length detected while building MAC PDU for cid %d "
                    "Queue real SDU length is %d  while scheduled SDUs are %d",
                    destCid, mbuf_[destCid]->getQueueLength(), sduPerCid);
            }

            // Add SDU to PDU
            // FIXME *move outside cycle* Find Mac Pkt
            MacPduList::iterator pit = macPduList_.find(pktId);

            // No packets for this user on this codeword
            if (pit == macPduList_.end())
            {
                UserControlInfo* uinfo = new UserControlInfo();
                uinfo->setSourceId(getMacNodeId());
                uinfo->setDestId(destId);
                uinfo->setDirection(DL);

                const UserTxParams& txInfo = amc_->computeTxParams(destId, DL);

                UserTxParams* txPara = new UserTxParams(txInfo);

                uinfo->setUserTxParams(txPara);
                txmode = txInfo.readTxMode();
                RbMap rbMap;
                uinfo->setTxMode(txmode);
                uinfo->setCw(cw);
                grantedBlocks = enbSchedulerDl_->readRbOccupation(destId, rbMap);

                uinfo->setGrantedBlocks(rbMap);
                uinfo->setTotalGrantedBlocks(grantedBlocks);

                macPkt = new LteMacPdu("LteMacPdu");
                macPkt->setHeaderLength(MAC_HEADER);
                macPkt->setControlInfo(uinfo);
                macPkt->setTimestamp(NOW);
                macPduList_[pktId] = macPkt;
            }
            else
            {
                macPkt = pit->second;
            }
            if (mbuf_[destCid]->getQueueLength() == 0)
            {
                throw cRuntimeError("Abnormal queue length detected while building MAC PDU for cid %d "
                    "Queue real SDU length is %d  while scheduled SDUs are %d",
                    destCid, mbuf_[destCid]->getQueueLength(), sduPerCid);
            }
            pkt = mbuf_[destCid]->popFront();

            ASSERT(pkt != NULL);

            take(pkt);
            macPkt->pushSdu(pkt);
            sduPerCid--;
        }
    }

    MacPduList::iterator pit;
    for (pit = macPduList_.begin(); pit != macPduList_.end(); pit++)
    {
        MacNodeId destId = pit->first.first;
        Codeword cw = pit->first.second;

        LteHarqBufferTx* txBuf;
        HarqTxBuffers::iterator hit = harqTxBuffers_.find(destId);
        if (hit != harqTxBuffers_.end())
        {
            txBuf = hit->second;
        }
        else
        {
            // FIXME: possible memory leak
            LteHarqBufferTx* hb = new LteHarqBufferTx(ENB_TX_HARQ_PROCESSES,
                this,(LteMacBase*)getMacUe(destId));
            harqTxBuffers_[destId] = hb;
            txBuf = hb;
        }
        UnitList txList = (txBuf->firstAvailable());

        LteMacPdu* macPkt = pit->second;
        EV << "LteMacBase: pduMaker created PDU: " << macPkt->info() << endl;

        // pdu transmission here (if any)
        if (txList.second.empty())
        {
            EV << "macPduMake() : no available process for this MAC pdu in TxHarqBuffer" << endl;
            delete macPkt;
        }
        else
        {
            if (txList.first == HARQ_NONE)
            throw cRuntimeError("LteMacBase: pduMaker sending to uncorrect void H-ARQ process. Aborting");
            txBuf->insertPdu(txList.first, cw, macPkt);
        }
    }
    EV << "------ END LteMacEnb::macPduMake ------\n";
}

void LteMacEnb::macPduUnmake(cPacket* pkt)
{
    LteMacPdu* macPkt = check_and_cast<LteMacPdu*>(pkt);
    while (macPkt->hasSdu())
    {
        // Extract and send SDU
        cPacket* upPkt = macPkt->popSdu();
        take(upPkt);

        // TODO: upPkt->info()
        EV << "LteMacBase: pduUnmaker extracted SDU" << endl;
        sendUpperPackets(upPkt);
    }

    while (macPkt->hasCe())
    {
        // Extract CE
        // TODO: vedere se bsr  per cid o lcid
        MacBsr* bsr = check_and_cast<MacBsr*>(macPkt->popCe());
        UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(macPkt->getControlInfo());
        MacCid cid = idToMacCid(lteInfo->getSourceId(), 0);
        bufferizeBsr(bsr, cid);
        delete bsr;
    }

    ASSERT(macPkt->getOwner() == this);
    delete macPkt;

}

int LteMacEnb::getNumRbDl()
{
    return numRbDl_;
}

int LteMacEnb::getNumRbUl()
{
    return numRbUl_;
}

bool LteMacEnb::bufferizePacket(cPacket* pkt)
{
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());
    MacCid cid = idToMacCid(lteInfo->getDestId(), lteInfo->getLcid());
    OmnetId id = getBinder()->getOmnetId(lteInfo->getDestId());
    if(id == 0){
        // destination UE has left the simulation
        delete pkt;
        return false;
    }
    bool ret = false;

    if ((ret = LteMacBase::bufferizePacket(pkt)))
    {
        enbSchedulerDl_->backlog(cid);
    }
    return ret;
}

void LteMacEnb::handleUpperMessage(cPacket* pkt)
{
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());
    MacCid cid = idToMacCid(lteInfo->getDestId(), lteInfo->getLcid());
    if (LteMacBase::bufferizePacket(pkt))
    {
        enbSchedulerDl_->backlog(cid);
    }
}

void LteMacEnb::handleSelfMessage()
{
    /***************
     *  MAIN LOOP  *
     ***************/
//    std::cout << "TTI: " << NOW << endl;

    int nodeCount = binder_->getNodeCount();
    if(nodeCount <= eNodeBCount)
        return;

    EnbType nodeType = deployer_->getEnbType();

    EV << "-----" << ((nodeType==MACRO_ENB)?"MACRO":"MICRO") << " ENB MAIN LOOP -----" << endl;

    /*************
     * END DEBUG
     *************/

    /* Reception */

    // extract pdus from all harqrxbuffers and pass them to unmaker
    HarqRxBuffers::iterator hit = harqRxBuffers_.begin();
    HarqRxBuffers::iterator het = harqRxBuffers_.end();
    LteMacPdu *pdu = NULL;
    std::list<LteMacPdu*> pduList;

    for (; hit != het; hit++)
    {
        pduList = hit->second->extractCorrectPdus();
        while (!pduList.empty())
        {
            pdu = pduList.front();
            pduList.pop_front();
            macPduUnmake(pdu);
        }
    }

    /*UPLINK*/
    EV << "============================================== UPLINK ==============================================" << endl;
    //TODO enable sleep mode also for UPLINK???
    (enbSchedulerUl_->resourceBlocks()) = getNumRbUl();

    enbSchedulerUl_->updateHarqDescs();

    LteMacScheduleList* scheduleListUl = enbSchedulerUl_->schedule();
    // send uplink grants to PHY layer
    sendGrants(scheduleListUl);
    EV << "============================================ END UPLINK ============================================" << endl;

    EV << "============================================ DOWNLINK ==============================================" << endl;
    /*DOWNLINK*/
    // Set current available OFDM space
    (enbSchedulerDl_->resourceBlocks()) = getNumRbDl();

    // use this flag to enable/disable scheduling...don't look at me, this is very useful!!!
    bool activation = true;

    if (activation)
    {
        // perform Downlink scheduling
        LteMacScheduleList* scheduleListDl = enbSchedulerDl_->schedule();
        // creates pdus from schedule list and puts them in harq buffers
        macPduMake(scheduleListDl);
    }
    EV << "========================================== END DOWNLINK ============================================" << endl;

    // purge from corrupted PDUs all Rx H-HARQ buffers for all users
    for (hit = harqRxBuffers_.begin(); hit != het; hit++)
    {
        hit->second->purgeCorruptedPdus();
    }

    // flush Tx H-ARQ buffers for all users
    HarqTxBuffers::iterator it;
    for (it = harqTxBuffers_.begin(); it != harqTxBuffers_.end(); it++)
        it->second->sendSelectedDown();

    EV << "--- END " << ((nodeType==MACRO_ENB)?"MACRO":"MICRO") << " ENB MAIN LOOP ---" << endl;
}

void LteMacEnb::macHandleFeedbackPkt(cPacket *pkt)
{
    LteFeedbackPkt* fb = check_and_cast<LteFeedbackPkt*>(pkt);
    LteFeedbackDoubleVector fbMapDl = fb->getLteFeedbackDoubleVectorDl();
    LteFeedbackDoubleVector fbMapUl = fb->getLteFeedbackDoubleVectorUl();
    //get Source Node Id<
    MacNodeId id = fb->getSourceNodeId();
    LteFeedbackDoubleVector::iterator it;
    LteFeedbackVector::iterator jt;

    for (it = fbMapDl.begin(); it != fbMapDl.end(); ++it)
    {
        unsigned int i = 0;
        for (jt = it->begin(); jt != it->end(); ++jt)
        {
            //            TxMode rx=(TxMode)i;
            if (!jt->isEmptyFeedback())
            {
                amc_->pushFeedback(id, DL, (*jt));
                LteMacUe* macUe = check_and_cast<LteMacUe*>(getMacByMacNodeId(id));
                macUe->collectCqiStatistics(id, DL, (*jt));
            }
            i++;
        }
    }
    for (it = fbMapUl.begin(); it != fbMapUl.end(); ++it)
    {
        for (jt = it->begin(); jt != it->end(); ++jt)
        {
            if (!jt->isEmptyFeedback())
                amc_->pushFeedback(id, UL, (*jt));
        }
    }
    delete fb;
}

void LteMacEnb::updateUserTxParam(cPacket* pkt)
{
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(
        pkt->getControlInfo());

    if (lteInfo->getFrameType() != DATAPKT)
        return; // TODO check if this should be removed.

    Direction dir = (Direction) lteInfo->getDirection();

    const UserTxParams& newParam = amc_->computeTxParams(lteInfo->getDestId(), dir);
    UserTxParams* tmp = new UserTxParams(newParam);

    lteInfo->setUserTxParams(tmp);
    RbMap rbMap;
    lteInfo->setTxMode(newParam.readTxMode());
    LteSchedulerEnb* scheduler = ((dir == DL) ? (LteSchedulerEnb*) enbSchedulerDl_ : (LteSchedulerEnb*) enbSchedulerUl_);

    int grantedBlocks = scheduler->readRbOccupation(lteInfo->getDestId(), rbMap);

    lteInfo->setGrantedBlocks(rbMap);
    lteInfo->setTotalGrantedBlocks(grantedBlocks);
}
ActiveSet LteMacEnb::getActiveSet(Direction dir)
{
    if (dir == DL)
        return enbSchedulerDl_->readActiveConnections();
    else
        return enbSchedulerUl_->readActiveConnections();
}
void LteMacEnb::allocatedRB(unsigned int rb)
{
    lastTtiAllocatedRb_ = rb;
}

unsigned int LteMacEnb::getBandStatus(Band b)
{
    unsigned int i = enbSchedulerDl_->readPerBandAllocatedBlocks(MAIN_PLANE, MACRO, b);
    return i;
}

unsigned int LteMacEnb::getPrevBandStatus(Band b)
{
    unsigned int i = enbSchedulerDl_->getInterferringBlocks(MAIN_PLANE, MACRO, b);
    return i;
}

