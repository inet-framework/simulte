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
#include "stack/mac/packet/LteMacSduRequest.h"
#include "common/LteCommon.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/rlc/am/packet/LteRlcAmPdu_m.h"

Define_Module( LteMacEnb);

using namespace omnetpp;

/*********************
 * PUBLIC FUNCTIONS
 *********************/

LteMacEnb::LteMacEnb() :
    LteMacBase()
{
    cellInfo_ = nullptr;
    amc_ = nullptr;
    enbSchedulerDl_ = nullptr;
    enbSchedulerUl_ = nullptr;
    numAntennas_ = 0;
    bsrbuf_.clear();
    currentSubFrameType_ = NORMAL_FRAME_TYPE;
    nodeType_ = ENODEB;
    frameIndex_ = 0;
    lastTtiAllocatedRb_ = 0;
    scheduleListDl_ = nullptr;
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

LteCellInfo* LteMacEnb::getCellInfo()
{
    // Get local cellInfo
    if (cellInfo_ != nullptr)
        return cellInfo_;

    return check_and_cast<LteCellInfo*>(getParentModule()-> // Stack
    getParentModule()-> // Enb
    getSubmodule("cellInfo")); // cellInfo
}

int LteMacEnb::getNumAntennas()
{
    /* Get number of antennas: +1 is for MACRO */
    return cellInfo_->getNumRus() + 1;
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
        cellInfo_ = getCellInfo();
        /* Get num RB Dl */
        numRbDl_ = cellInfo_->getNumRbDl();
        /* Get num RB Ul */
        numRbUl_ = cellInfo_->getNumRbUl();

        /* Get number of antennas */
        numAntennas_ = getNumAntennas();

        /* Create and initialize MAC Downlink scheduler */
        if (enbSchedulerDl_ == nullptr)
        {
            enbSchedulerDl_ = new LteSchedulerEnbDl();
            enbSchedulerDl_->initialize(DL, this);
        }

        /* Create and initialize MAC Uplink scheduler */
        if (enbSchedulerUl_ == nullptr)
        {
            enbSchedulerUl_ = new LteSchedulerEnbUl();
            enbSchedulerUl_->initialize(UL, this);
        }
        //Initialize the current sub frame type with the first subframe of the MBSFN pattern
        currentSubFrameType_ = NORMAL_FRAME_TYPE;

        eNodeBCount = par("eNodeBCount");
        WATCH(numAntennas_);
        WATCH_MAP(bsrbuf_);
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        /* Create and initialize AMC module */
        amc_ = new LteAmc(this, binder_, cellInfo_, numAntennas_);

        /* Insert EnbInfo in the Binder */
        EnbInfo* info = new EnbInfo();
        info->id = nodeId_;            // local mac ID
        info->type = MACRO_ENB;        // eNb Type
        info->init = false;            // flag for phy initialization
        info->eNodeB = this->getParentModule()->getParentModule();  // reference to the eNodeB module
        binder_->addEnbInfo(info);

        // register the pair <id,name> to the binder
        const char* moduleName = getParentModule()->getParentModule()->getFullName();
        binder_->registerName(nodeId_, moduleName);
    }
}

void LteMacEnb::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (strcmp(msg->getName(), "flushHarqMsg") == 0)
        {
            flushHarqBuffers();
            delete msg;
            return;
        }
    }
    LteMacBase::handleMessage(msg);
}

void LteMacEnb::macSduRequest()
{
    EV << "----- START LteMacEnb::macSduRequest -----\n";

    // Ask for a MAC sdu for each scheduled user on each codeword
    LteMacScheduleList::const_iterator it;
    for (it = scheduleListDl_->begin(); it != scheduleListDl_->end(); it++)
    {
        MacCid destCid = it->first.first;
        // Codeword cw = it->first.second;
        MacNodeId destId = MacCidToNodeId(destCid);

        // for each band, count the number of bytes allocated for this ue (dovrebbe essere per cid)
        unsigned int allocatedBytes = 0;
        int numBands = cellInfo_->getNumBands();
        for (Band b=0; b < numBands; b++)
        {
            // get the number of bytes allocated to this connection
            // (this represents the MAC PDU size)
            allocatedBytes += enbSchedulerDl_->allocator_->getBytes(MACRO,b,destId);
        }

        // send the request message to the upper layer
        auto pkt = new Packet("LteMacSduRequest");
        auto macSduRequest = makeShared<LteMacSduRequest>();
        macSduRequest->setChunkLength(b(1)); // TODO: should be 0
        macSduRequest->setUeId(destId);
        macSduRequest->setLcid(MacCidToLcid(destCid));
        macSduRequest->setSduSize(allocatedBytes - MAC_HEADER);    // do not consider MAC header size
        pkt->insertAtFront(macSduRequest);

        if (queueSize_ != 0 && queueSize_ < macSduRequest->getSduSize()) {
            throw cRuntimeError("LteMacEnb::macSduRequest: configured queueSize too low - requested SDU will not fit in queue!"
                    " (queue size: %d, sdu request requires: %d)", queueSize_, macSduRequest->getSduSize());
        }

        auto tag = pkt->addTag<FlowControlInfo>();
        *tag = connDesc_[destCid];

        sendUpperPackets(pkt);
    }

    EV << "------ END LteMacEnb::macSduRequest ------\n";
}

void LteMacEnb::bufferizeBsr(MacBsr* bsr, MacCid cid)
{
    LteMacBufferMap::iterator it = bsrbuf_.find(cid);
    if (it == bsrbuf_.end())
    {
        if (bsr->getSize() > 0)
        {
            // Queue not found for this cid: create
            LteMacBuffer* bsrqueue = new LteMacBuffer();

            PacketInfo vpkt(bsr->getSize(), bsr->getTimestamp());
            bsrqueue->pushBack(vpkt);
            bsrbuf_[cid] = bsrqueue;

            EV << "LteBsrBuffers : Added new BSR buffer for node: "
               << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid)
               << " Current BSR size: " << bsr->getSize() << "\n";

            // signal backlog to Uplink scheduler
            enbSchedulerUl_->backlog(cid);
        }
        // do not store if BSR size = 0
    }
    else
    {
        // Found
        LteMacBuffer* bsrqueue = it->second;
        if (bsr->getSize() > 0)
        {
            // update buffer
            PacketInfo queuedBsr;
            if (!bsrqueue->isEmpty())
                queuedBsr = bsrqueue->popFront();

            queuedBsr.first = bsr->getSize();
            queuedBsr.second = bsr->getTimestamp();
            bsrqueue->pushBack(queuedBsr);

            EV << "LteBsrBuffers : Using old buffer for node: " << MacCidToNodeId(
                cid) << " for Lcid: " << MacCidToLcid(cid)
               << " Current BSR size: " << bsr->getSize() << "\n";

            // signal backlog to Uplink scheduler
            enbSchedulerUl_->backlog(cid);
        }
        else
        {
            // the UE has no backlog, remove BSR
            if (!bsrqueue->isEmpty())
                bsrqueue->popFront();

            EV << "LteBsrBuffers : Using old buffer for node: " << MacCidToNodeId(
                cid) << " for Lcid: " << MacCidToLcid(cid)
               << " - now empty" << "\n";
        }
    }
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

        // TODO: change to tag instead chunk
        // TODO Grant is set aperiodic as default
        auto pkt = new Packet("LteGrant");

        auto grant = makeShared<LteSchedulingGrant>();
        grant->setDirection(UL);

        grant->setCodewords(codewords);

        // set total granted blocks
        grant->setTotalGrantedBlocks(granted);

        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(nodeId);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(GRANTPKT);

        // get and set the user's UserTxParams
        const UserTxParams& ui = getAmc()->computeTxParams(nodeId, UL);
        UserTxParams* txPara = new UserTxParams(ui);
        grant->setUserTxParams(txPara);

        // acquiring remote antennas set from user info
        const std::set<Remote>& antennas = ui.readAntennaSet();
        std::set<Remote>::const_iterator antenna_it, antenna_et = antennas.end();
        const unsigned int logicalBands = cellInfo_->getNumBands();

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
        pkt->insertAtFront(grant);
        // send grant to PHY layer
        sendLowerPackets(pkt);
    }
}

void LteMacEnb::macHandleRac(cPacket* pktAux)
{
    EV << NOW << " LteMacEnb::macHandleRac" << endl;

    auto pkt = check_and_cast<Packet *>(pktAux);

    auto racPkt = pkt->removeAtFront<LteRac>();
    auto uinfo = pkt->getTag<UserControlInfo>();
    
    enbSchedulerUl_->signalRac(uinfo->getSourceId());

    // TODO all RACs are marked are successful
    racPkt->setSuccess(true);
    pkt->insertAtFront(racPkt);

    uinfo->setDestId(uinfo->getSourceId());
    uinfo->setSourceId(nodeId_);
    uinfo->setDirection(DL);

    sendLowerPackets(pkt);
}

void LteMacEnb::macPduMake(MacCid cid)
{
    EV << "----- START LteMacEnb::macPduMake -----\n";
    // Finalizes the scheduling decisions according to the schedule list,
    // detaching sdus from real buffers.

    macPduList_.clear();

    //  Build a MAC pdu for each scheduled user on each codeword
    LteMacScheduleList::const_iterator it;
    for (it = scheduleListDl_->begin(); it != scheduleListDl_->end(); it++)
    {
        Packet *macPacket = nullptr;
        MacCid destCid = it->first.first;

        if (destCid != cid)
            continue;

        // check whether the RLC has sent some data. If not, skip
        // (e.g. because the size of the MAC PDU would contain only MAC header - MAC SDU requested size = 0B)
        if (mbuf_[destCid]->getQueueLength() == 0)
            break;

        Codeword cw = it->first.second;
        MacNodeId destId = MacCidToNodeId(destCid);
        std::pair<MacNodeId, Codeword> pktId = std::pair<MacNodeId, Codeword>(destId, cw);
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
            auto pit = macPduList_.find(pktId);

            // No packets for this user on this codeword
            if (pit == macPduList_.end())
            {

                auto pkt = new Packet("LteMacPdu");
                pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                pkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                pkt->addTagIfAbsent<UserControlInfo>()->setDirection(DL);

                const UserTxParams& txInfo = amc_->computeTxParams(destId, DL);

                UserTxParams* txPara = new UserTxParams(txInfo);

                pkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(txPara);
                txmode = txInfo.readTxMode();
                RbMap rbMap;

                pkt->addTagIfAbsent<UserControlInfo>()->setTxMode(txmode);
                pkt->addTagIfAbsent<UserControlInfo>()->setCw(cw);

                grantedBlocks = enbSchedulerDl_->readRbOccupation(destId, rbMap);

                pkt->addTagIfAbsent<UserControlInfo>()->setGrantedBlocks(rbMap);
                pkt->addTagIfAbsent<UserControlInfo>()->setTotalGrantedBlocks(grantedBlocks);
                macPacket = pkt;

                auto macPkt = makeShared<LteMacPdu>();
                macPkt->setHeaderLength(MAC_HEADER);
                macPacket->insertAtFront(macPkt);
                macPduList_[pktId] = macPacket;
            }
            else
            {
                macPacket = pit->second;
            }
            if (mbuf_[destCid]->getQueueLength() == 0)
            {
                throw cRuntimeError("Abnormal queue length detected while building MAC PDU for cid %d "
                    "Queue real SDU length is %d  while scheduled SDUs are %d",
                    destCid, mbuf_[destCid]->getQueueLength(), sduPerCid);
            }
            auto pkt = check_and_cast<Packet *>(mbuf_[destCid]->popFront());

            ASSERT(pkt != nullptr);

            drop(pkt);
            auto macPkt =  macPacket->removeAtFront<LteMacPdu>();
            macPkt->pushSdu(pkt);
            macPacket->insertAtFront(macPkt);
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

        auto macPacket = pit->second;
        auto header = macPacket->peekAtFront<LteMacPdu>();

        EV << "LteMacBase: pduMaker created PDU: " << header->str() << endl;

        // pdu transmission here (if any)
        if (txList.second.empty())
        {
            EV << "macPduMake() : no available process for this MAC pdu in TxHarqBuffer" << endl;
            delete macPacket;
        }
        else
        {
            if (txList.first == HARQ_NONE)
            throw cRuntimeError("LteMacBase: pduMaker sending to uncorrect void H-ARQ process. Aborting");
            txBuf->insertPdu(txList.first, cw, macPacket);
        }
    }
    EV << "------ END LteMacEnb::macPduMake ------\n";
}

void LteMacEnb::macPduUnmake(cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet*>(pktAux);
    auto macPkt = pkt->removeAtFront<LteMacPdu>();

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
        auto lteInfo = pkt->getTag<UserControlInfo>();
        MacCid cid = idToMacCid(lteInfo->getSourceId(), 0);
        bufferizeBsr(bsr, cid);
        delete bsr;
    }
    pkt->insertAtFront(macPkt);

    ASSERT(pkt->getOwner() == this);
    delete pkt;

}

int LteMacEnb::getNumRbDl()
{
    return numRbDl_;
}

int LteMacEnb::getNumRbUl()
{
    return numRbUl_;
}

bool LteMacEnb::bufferizePacket(cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    if (pkt->getBitLength() <= 1) { // no data in this packet
        delete pktAux;
        return false;
    }

    pkt->setTimestamp();        // Add timestamp with current time to packet

    auto lteInfo = pkt->getTag<FlowControlInfo>();

    // obtain the cid from the packet informations
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // this packet is used to signal the arrival of new data in the RLC buffers
    if (checkIfHeaderType<LteRlcPduNewData>(pkt))
    {
        // update the virtual buffer for this connection

        // build the virtual packet corresponding to this incoming packet
        pkt->popAtFront<LteRlcPduNewData>();
        auto rlcSdu = pkt->peekAtFront<LteRlcSdu>();
        PacketInfo vpkt(rlcSdu->getLengthMainPacket(), pkt->getTimestamp());

        LteMacBufferMap::iterator it = macBuffers_.find(cid);
        if (it == macBuffers_.end())
        {
            LteMacBuffer* vqueue = new LteMacBuffer();
            vqueue->pushBack(vpkt);
            macBuffers_[cid] = vqueue;

            // make a copy of lte control info and store it to traffic descriptors map
            FlowControlInfo toStore(*lteInfo);
            connDesc_[cid] = toStore;
            // register connection to lcg map.
            LteTrafficClass tClass = (LteTrafficClass) lteInfo->getTraffic();

            lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));

            EV << "LteMacBuffers : Using new buffer on node: " <<
            MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Bytes in the Queue: " <<
            vqueue->getQueueOccupancy() << "\n";
        }
        else
        {
            LteMacBuffer* vqueue = macBuffers_.find(cid)->second;
            vqueue->pushBack(vpkt);

            EV << "LteMacBuffers : Using old buffer on node: " <<
            MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
            vqueue->getQueueOccupancy() << "\n";
        }

        delete pkt;
        return true; // this is only a new packet indication - only buffered in virtual queue
    }

    // this is a MAC SDU, bufferize it in the MAC buffer

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end())
    {
        // Queue not found for this cid: create
        LteMacQueue* queue = new LteMacQueue(queueSize_);

        queue->pushBack(pkt);

        mbuf_[cid] = queue;

        EV << "LteMacBuffers : Using new buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }
    else
    {
        // Found
        LteMacQueue* queue = it->second;

        if (!queue->pushBack(pkt))
        {
            // unable to buffer packet (packet is not enqueued and will be dropped): update statistics
            EV << "LteMacBuffers : queue" << cid << " is full - cannot buffer packet " << pkt->getId()<< "\n";

            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());

            if (lteInfo->getDirection() == DL)
                emit(macBufferOverflowDl_,sample);
            else
                emit(macBufferOverflowUl_,sample);
        }

        EV << "LteMacBuffers : Using old buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }

    return true;
}

void LteMacEnb::handleUpperMessage(cPacket* pktAux)
{
	auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    MacCid cid = idToMacCid(lteInfo->getDestId(), lteInfo->getLcid());

    bool isLteRlcPduNewData = checkIfHeaderType<LteRlcPduNewData>(pkt);

	bool packetIsBuffered = bufferizePacket(pkt);  // will buffer (or destroy if queue is full)

    if(!isLteRlcPduNewData && packetIsBuffered) {
        // new MAC SDU has been received (was requested by MAC, no need to notify scheduler)
        // creates pdus from schedule list and puts them in harq buffers
        macPduMake(cid);
    } else if (isLteRlcPduNewData) {
        // new data - inform scheduler of active connection
        enbSchedulerDl_->backlog(cid);
    }
}


void LteMacEnb::handleSelfMessage()
{
    /***************
     *  MAIN LOOP  *
     ***************/

    EV << "-----" << "ENB MAIN LOOP -----" << endl;

    /*************
     * END DEBUG
     *************/

    /* Reception */

    // extract pdus from all harqrxbuffers and pass them to unmaker
    HarqRxBuffers::iterator hit = harqRxBuffers_.begin();
    HarqRxBuffers::iterator het = harqRxBuffers_.end();

    for (; hit != het; hit++)
    {
        auto pduList = hit->second->extractCorrectPdus();
        while (!pduList.empty())
        {
            auto pktPdu = pduList.front();
            auto pdu = pktPdu->peekAtFront<LteMacPdu>();
            pduList.pop_front();
            macPduUnmake(pktPdu);
        }
    }

    /*UPLINK*/
    EV << "============================================== UPLINK ==============================================" << endl;
    // init and reset global allocation information
    if (binder_->getLastUpdateUlTransmissionInfo() < NOW)  // once per TTI, even in case of multicell scenarios
        binder_->initAndResetUlTransmissionInfo();

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
        // clear previous schedule list
        if (scheduleListDl_ != nullptr)
            scheduleListDl_->clear();

        // perform Downlink scheduling
        scheduleListDl_ = enbSchedulerDl_->schedule();

        // requests SDUs to the RLC layer
        macSduRequest();
    }
    EV << "========================================== END DOWNLINK ============================================" << endl;

    // purge from corrupted PDUs all Rx H-HARQ buffers for all users
    for (; hit != het; hit++)
    {
        hit->second->purgeCorruptedPdus();
    }

    // Message that triggers flushing of Tx H-ARQ buffers for all users
    // This way, flushing is performed after the (possible) reception of new MAC PDUs
    cMessage* flushHarqMsg = new cMessage("flushHarqMsg");
    flushHarqMsg->setSchedulingPriority(1);        // after other messages
    scheduleAt(NOW, flushHarqMsg);

    EV << "--- END ENB MAIN LOOP ---" << endl;
}

void LteMacEnb::flushHarqBuffers()
{
    HarqTxBuffers::iterator it;
    for (it = harqTxBuffers_.begin(); it != harqTxBuffers_.end(); it++)
        it->second->sendSelectedDown();
}

void LteMacEnb::macHandleFeedbackPkt(cPacket *pktAux)
{


    auto pkt = check_and_cast<Packet *>(pktAux);
    auto fb = pkt->peekAtFront<LteFeedbackPkt>();

    //LteFeedbackPkt* fb = check_and_cast<LteFeedbackPkt*>(pkt);
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
                // LteMacUe* macUe = check_and_cast<LteMacUe*>(getMacByMacNodeId(id));
                //  macUe->collectCqiStatistics(id, DL, (*jt));
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
    delete pkt;
}

void LteMacEnb::updateUserTxParam(cPacket* pktAux)
{

    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTag<UserControlInfo>();

    if (lteInfo->getFrameType() != DATAPKT)
        return; // TODO check if this should be removed.

    auto dir = (Direction) lteInfo->getDirection();

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

unsigned int LteMacEnb::getDlBandStatus(Band b)
{
    unsigned int i = enbSchedulerDl_->readPerBandAllocatedBlocks(MAIN_PLANE, MACRO, b);
    return i;
}

unsigned int LteMacEnb::getDlPrevBandStatus(Band b)
{
    unsigned int i = enbSchedulerDl_->getInterferringBlocks(MAIN_PLANE, MACRO, b);
    return i;
}

ConflictGraph* LteMacEnb::getConflictGraph()
{
    return nullptr;
}
