//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/layer/LteMacUe.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/scheduler/LteSchedulerUeUl.h"
#include "stack/mac/packet/LteRac_m.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/amc/UserTxParams.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/common/ModuleAccess.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "corenetwork/binder/LteBinder.h"
#include "stack/phy/layer/LtePhyBase.h"
#include "stack/mac/packet/LteMacSduRequest.h"

Define_Module(LteMacUe);

LteMacUe::LteMacUe() :
    LteMacBase()
{
    firstTx = false;
    lcgScheduler_ = NULL;
    schedulingGrant_ = NULL;
    currentHarq_ = 0;
    periodCounter_ = 0;
    expirationCounter_ = 0;
    racRequested_ = false;
    bsrTriggered_ = false;
    requestedSdus_ = 0;
    scheduleList_ = NULL;
    debugHarq_ = false;

    // TODO setup from NED
    racBackoffTimer_ = 0;
    maxRacTryouts_ = 0;
    currentRacTry_ = 0;
    minRacBackoff_ = 0;
    maxRacBackoff_ = 1;
    raRespTimer_ = 0;
    raRespWinStart_ = 3;
    nodeType_ = UE;

    // KLUDGE: this was unitialized, this is just a guess
    harqProcesses_ = 8;
}

LteMacUe::~LteMacUe()
{
    delete lcgScheduler_;

    if (schedulingGrant_!=NULL)
    {
        delete schedulingGrant_;
        schedulingGrant_ = NULL;
    }
}

void LteMacUe::initialize(int stage)
{
    LteMacBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        lcgScheduler_ = new LteSchedulerUeUl(this);

        cqiDlMuMimo0_ = registerSignal("cqiDlMuMimo0");
        cqiDlMuMimo1_ = registerSignal("cqiDlMuMimo1");
        cqiDlMuMimo2_ = registerSignal("cqiDlMuMimo2");
        cqiDlMuMimo3_ = registerSignal("cqiDlMuMimo3");
        cqiDlMuMimo4_ = registerSignal("cqiDlMuMimo4");

        cqiDlTxDiv0_ = registerSignal("cqiDlTxDiv0");
        cqiDlTxDiv1_ = registerSignal("cqiDlTxDiv1");
        cqiDlTxDiv2_ = registerSignal("cqiDlTxDiv2");
        cqiDlTxDiv3_ = registerSignal("cqiDlTxDiv3");
        cqiDlTxDiv4_ = registerSignal("cqiDlTxDiv4");

        cqiDlSpmux0_ = registerSignal("cqiDlSpmux0");
        cqiDlSpmux1_ = registerSignal("cqiDlSpmux1");
        cqiDlSpmux2_ = registerSignal("cqiDlSpmux2");
        cqiDlSpmux3_ = registerSignal("cqiDlSpmux3");
        cqiDlSpmux4_ = registerSignal("cqiDlSpmux4");

        cqiDlSiso0_ = registerSignal("cqiDlSiso0");
        cqiDlSiso1_ = registerSignal("cqiDlSiso1");
        cqiDlSiso2_ = registerSignal("cqiDlSiso2");
        cqiDlSiso3_ = registerSignal("cqiDlSiso3");
        cqiDlSiso4_ = registerSignal("cqiDlSiso4");
    }
    else if (stage == INITSTAGE_LINK_LAYER)
    {
        cellId_ = getAncestorPar("masterId");
    }
    else if (stage == INITSTAGE_NETWORK_LAYER_3)
    {
        nodeId_ = getAncestorPar("macNodeId");

        /* Insert UeInfo in the Binder */
        UeInfo* info = new UeInfo();
        info->id = nodeId_;            // local mac ID
        info->cellId = cellId_;        // cell ID
        info->init = false;            // flag for phy initialization
        info->ue = this->getParentModule()->getParentModule();  // reference to the UE module

        // Get the Physical Channel reference of the node
        info->phy = check_and_cast<LtePhyBase*>(info->ue->getSubmodule("lteNic")->getSubmodule("phy"));

        binder_->addUeInfo(info);

        // only for UEs that have been added dynamically to the simulation
        LteAmc *amc = check_and_cast<LteMacEnb *>(getSimulation()->getModule(binder_->getOmnetId(cellId_))->getSubmodule("lteNic")->getSubmodule("mac"))->getAmc();
        amc->attachUser(nodeId_, UL);
        amc->attachUser(nodeId_, DL);

        // find interface entry and use its address
        IInterfaceTable *interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        // TODO: how do we find the LTE interface?
        InterfaceEntry * interfaceEntry = interfaceTable->getInterfaceByName("wlan");

        IPv4InterfaceData* ipv4if = interfaceEntry->ipv4Data();
        if(ipv4if == NULL)
            throw new cRuntimeError("no IPv4 interface data - cannot bind node %i", nodeId_);
        binder_->setMacNodeId(ipv4if->getIPAddress(), nodeId_);
    }
}

void LteMacUe::handleMessage(cMessage* msg)
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

int LteMacUe::macSduRequest()
{
    EV << "----- START LteMacUe::macSduRequest -----\n";
    int numRequestedSdus = 0;

    // get the number of granted bytes for each codeword
    std::vector<unsigned int> allocatedBytes;
    for (int cw=0; cw<schedulingGrant_->getGrantedCwBytesArraySize(); cw++)
        allocatedBytes.push_back(schedulingGrant_->getGrantedCwBytes(cw));

    LteMacScheduleList* scheduledBytesList = lcgScheduler_->getScheduledBytesList();
    bool firstSdu = true;

    // Ask for a MAC sdu for each scheduled user on each codeword
    LteMacScheduleList::const_iterator it;
    for (it = scheduleList_->begin(); it != scheduleList_->end(); it++)
    {
        MacCid destCid = it->first.first;
        Codeword cw = it->first.second;
        MacNodeId destId = MacCidToNodeId(destCid);

        std::pair<MacCid,Codeword> key(destCid, cw);
        LteMacScheduleList::const_iterator bit = scheduledBytesList->find(key);

        unsigned int sduSize = bit->second;
        if (firstSdu)
        {
            sduSize -= MAC_HEADER;    // do not consider MAC header size
            firstSdu = false;
        }

        // consume bytes on this codeword
        allocatedBytes[cw] -= sduSize;

        EV << NOW <<" LteMacUe::macSduRequest - cid[" << destCid << "] - sdu size[" << sduSize<< "B] - " << allocatedBytes[cw] << " bytes left on codeword " << cw << endl;

        // send the request message to the upper layer
        LteMacSduRequest* macSduRequest = new LteMacSduRequest("LteMacSduRequest");
        macSduRequest->setUeId(destId);
        macSduRequest->setLcid(MacCidToLcid(destCid));
        macSduRequest->setSduSize(sduSize);
        macSduRequest->setControlInfo((&connDesc_[destCid])->dup());
        sendUpperPackets(macSduRequest);

        numRequestedSdus++;
    }

    EV << "------ END LteMacUe::macSduRequest ------\n";
    return numRequestedSdus;
}

bool LteMacUe::bufferizePacket(cPacket* pkt)
{
    if (pkt->getByteLength() == 0)
        return false;

    pkt->setTimestamp();        // Add timestamp with current time to packet

    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

    // obtain the cid from the packet informations
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // this packet is used to signal the arrival of new data in the RLC buffers
    if (strcmp(pkt->getName(), "newDataPkt") == 0)
    {
        // update the virtual buffer for this connection

        // build the virtual packet corresponding to this incoming packet
        PacketInfo vpkt(pkt->getByteLength(), pkt->getTimestamp());

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

        return true;    // notify the activation of the connection
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
            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
            if (lteInfo->getDirection()==DL)
            {
                emit(macBufferOverflowDl_,sample);
            }
            else
            {
                emit(macBufferOverflowUl_,sample);
            }

            EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
            delete pkt;
            return false;
        }

        EV << "LteMacBuffers : Using old buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << "(cid: " << cid << "), Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }

    return false; // do not need to notify the activation of the connection (already done when received newDataPkt)
}

void LteMacUe::macPduMake(MacCid cid)
{
    int64 size = 0;

    macPduList_.clear();

    //  Build a MAC pdu for each scheduled user on each codeword
    LteMacScheduleList::const_iterator it;
    for (it = scheduleList_->begin(); it != scheduleList_->end(); it++)
    {
        LteMacPdu* macPkt;
        cPacket* pkt;

        MacCid destCid = it->first.first;
        Codeword cw = it->first.second;

        // from an UE perspective, the destId is always the one of the eNb
        MacNodeId destId = getMacCellId();

        std::pair<MacNodeId, Codeword> pktId = std::pair<MacNodeId, Codeword>(destId, cw);
        unsigned int sduPerCid = it->second;

        MacPduList::iterator pit = macPduList_.find(pktId);

        if (sduPerCid == 0 && !bsrTriggered_)
        {
            continue;
        }

        // No packets for this user on this codeword
        if (pit == macPduList_.end())
        {
            UserControlInfo* uinfo = new UserControlInfo();
            uinfo->setSourceId(getMacNodeId());
            uinfo->setDestId(destId);
            uinfo->setDirection(UL);
            uinfo->setUserTxParams(schedulingGrant_->getUserTxParams()->dup());
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

        while (sduPerCid > 0)
        {
            // Add SDU to PDU
            // Find Mac Pkt

            if (mbuf_.find(destCid) == mbuf_.end())
                throw cRuntimeError("Unable to find mac buffer for cid %d", destCid);

            if (mbuf_[destCid]->isEmpty())
                throw cRuntimeError("Empty buffer for cid %d, while expected SDUs were %d", destCid, sduPerCid);

            pkt = mbuf_[destCid]->popFront();
            drop(pkt);
            macPkt->pushSdu(pkt);
            sduPerCid--;
        }
        // consider virtual buffers to compute BSR size
        size += macBuffers_[destCid]->getQueueOccupancy();
    }

    //  Put MAC pdus in H-ARQ buffers

    MacPduList::iterator pit;
    for (pit = macPduList_.begin(); pit != macPduList_.end(); pit++)
    {
        MacNodeId destId = pit->first.first;
        Codeword cw = pit->first.second;

        LteHarqBufferTx* txBuf;
        HarqTxBuffers::iterator hit = harqTxBuffers_.find(destId);
        if (hit != harqTxBuffers_.end())
        {
            // the tx buffer already exists
            txBuf = hit->second;
        }
        else
        {
            // the tx buffer does not exist yet for this mac node id, create one
            // FIXME: hb is never deleted
            LteHarqBufferTx* hb = new LteHarqBufferTx((unsigned int) ENB_TX_HARQ_PROCESSES, this,
                (LteMacBase*) getMacByMacNodeId(cellId_));
            harqTxBuffers_[destId] = hb;
            txBuf = hb;
        }
        //
//        UnitList txList = (txBuf->firstAvailable());
//        LteHarqProcessTx * currProc = txBuf->getProcess(currentHarq_);

        // search for an empty unit within current harq process
        UnitList txList = txBuf->getEmptyUnits(currentHarq_);
//        EV << "LteMacUe::macPduMake - [Used Acid=" << (unsigned int)txList.first << "] , [curr=" << (unsigned int)currentHarq_ << "]" << endl;

        LteMacPdu* macPkt = pit->second;

        // BSR related operations

        // according to the TS 36.321 v8.7.0, when there are uplink resources assigned to the UE, a BSR
        // has to be send even if there is no data in the user's queues. In few words, a BSR is always
        // triggered and has to be send when there are enough resources

        // TODO implement differentiated BSR attach
        //
        //            // if there's enough space for a LONG BSR, send it
        //            if( (availableBytes >= LONG_BSR_SIZE) ) {
        //                // Create a PDU if data were not scheduled
        //                if (pdu==0)
        //                    pdu = new LteMacPdu();
        //
        //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
        //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - Node %d, sending a Long BSR...\n",NOW,nodeId);
        //
        //                // create a full BSR
        //                pdu->ctrlPush(fullBufferStatusReport());
        //
        //                // do not reset BSR flag
        //                mac_->bsrTriggered() = true;
        //
        //                availableBytes -= LONG_BSR_SIZE;
        //
        //            }
        //
        //            // if there's space only for a SHORT BSR and there are scheduled flows, send it
        //            else if( (mac_->bsrTriggered() == true) && (availableBytes >= SHORT_BSR_SIZE) && (highestBackloggedFlow != -1) ) {
        //
        //                // Create a PDU if data were not scheduled
        //                if (pdu==0)
        //                    pdu = new LteMacPdu();
        //
        //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
        //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - Node %d, sending a Short/Truncated BSR...\n",NOW,nodeId);
        //
        //                // create a short BSR
        //                pdu->ctrlPush(shortBufferStatusReport(highestBackloggedFlow));
        //
        //                // do not reset BSR flag
        //                mac_->bsrTriggered() = true;
        //
        //                availableBytes -= SHORT_BSR_SIZE;
        //
        //            }
        //            // if there's a BSR triggered but there's not enough space, collect the appropriate statistic
        //            else if(availableBytes < SHORT_BSR_SIZE && availableBytes < LONG_BSR_SIZE) {
        //                Stat::put(LTE_BSR_SUPPRESSED_NODE,nodeId,1.0);
        //                Stat::put(LTE_BSR_SUPPRESSED_CELL,mac_->cellId(),1.0);
        //            }
        //            Stat::put (LTE_GRANT_WASTED_BYTES_UL, nodeId, availableBytes);
        //        }
        //
        //        // 4) PDU creation
        //
        //        if (pdu!=0) {
        //
        //            pdu->cellId() = mac_->cellId();
        //            pdu->nodeId() = nodeId;
        //            pdu->direction() = mac::UL;
        //            pdu->error() = false;
        //
        //            if(LteDebug::trace("LteSchedulerUeUl::schedule"))
        //                fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - Node %d, creating uplink PDU.\n", NOW, nodeId);
        //
        //        }

        if (bsrTriggered_)
        {
            MacBsr* bsr = new MacBsr();
            bsr->setTimestamp(simTime().dbl());
            bsr->setSize(size);
            macPkt->pushCe(bsr);
            bsrTriggered_ = false;
            EV << "LteMacUe::macPduMake - BSR with size " << size << "created" << endl;
        }

        EV << "LteMacUe: pduMaker created PDU: " << macPkt->info() << endl;

        // TODO: harq test
        // pdu transmission here (if any)
        // txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
        if (txList.second.empty())
        {
            EV << "macPduMake() : no available process for this MAC pdu in TxHarqBuffer" << endl;
            delete macPkt;
        }
        else
        {
            txBuf->insertPdu(txList.first,cw, macPkt);
        }
    }
}

void LteMacUe::macPduUnmake(cPacket* pkt)
{
    LteMacPdu* macPkt = check_and_cast<LteMacPdu*>(pkt);
    while (macPkt->hasSdu())
    {
        // Extract and send SDU
        cPacket* upPkt = macPkt->popSdu();
        take(upPkt);

        /* TODO: upPkt->info() */
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

    ASSERT(macPkt->getOwner() == this);
    delete macPkt;
}

void LteMacUe::handleUpperMessage(cPacket* pkt)
{
    // bufferize packet
    bufferizePacket(pkt);

    if (strcmp(pkt->getName(), "lteRlcFragment") == 0)
    {
        // new MAC SDU has been received
        if (pkt->getByteLength() == 0)
            delete pkt;

        // build a MAC PDU only after all MAC SDUs have been received from RLC
        requestedSdus_--;
        if (requestedSdus_ == 0)
        {
            macPduMake();
            // update current harq process id
            EV << NOW << " LteMacUe::handleMessage - incrementing counter for HARQ processes " << (unsigned int)currentHarq_ << " --> " << (currentHarq_+1)%harqProcesses_ << endl;
            currentHarq_ = (currentHarq_+1) % harqProcesses_;
        }
    }
    else
    {
        delete pkt;
    }
}

void LteMacUe::handleSelfMessage()
{
    EV << "----- UE MAIN LOOP -----" << endl;

    // extract pdus from all harqrxbuffers and pass them to unmaker
    HarqRxBuffers::iterator hit = harqRxBuffers_.begin();
    HarqRxBuffers::iterator het = harqRxBuffers_.end();
    LteMacPdu *pdu = NULL;
    std::list<LteMacPdu*> pduList;

    for (; hit != het; ++hit)
    {
        pduList=hit->second->extractCorrectPdus();
        while (! pduList.empty())
        {
            pdu=pduList.front();
            pduList.pop_front();
            macPduUnmake(pdu);
        }
    }

    EV << NOW << "LteMacUe::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;
    // updating current HARQ process for next TTI

    //unsigned char currentHarq = currentHarq_;

    // no grant available - if user has backlogged data, it will trigger scheduling request
    // no harq counter is updated since no transmission is sent.

    if (schedulingGrant_==NULL)
    {
        EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;

//        if (!bsrTriggered_)
//        {
        // if necessary, a RAC request will be sent to obtain a grant
        checkRAC();
//        }
        // TODO ensure all operations done  before return ( i.e. move H-ARQ rx purge before this point)
    }
    else if (schedulingGrant_->getPeriodic())
    {
        // Periodic checks
        if(--expirationCounter_ < 0)
        {
            // Periodic grant is expired
            delete schedulingGrant_;
            schedulingGrant_ = NULL;
            // if necessary, a RAC request will be sent to obtain a grant
            checkRAC();
            //return;
        }
        else if (--periodCounter_>0)
        {
            return;
        }
        else
        {
            // resetting grant period
            periodCounter_=schedulingGrant_->getPeriod();
            // this is periodic grant TTI - continue with frame sending
        }
    }

    requestedSdus_ = 0;
    if (schedulingGrant_!=NULL) // if a grant is configured
    {
        if(!firstTx)
        {
            EV << "\t currentHarq_ counter initialized " << endl;
            firstTx=true;
            // the eNb will receive the first pdu in 2 TTI, thus initializing acid to 0
//            currentHarq_ = harqRxBuffers_.begin()->second->getProcesses() - 2;
            currentHarq_ = UE_TX_HARQ_PROCESSES - 2;
        }
        EV << "\t " << schedulingGrant_ << endl;

//        //! \TEST  Grant Synchronization check
//        if (!(schedulingGrant_->getPeriodic()))
//        {
//            if ( false /* TODO currentHarq!=grant_->getAcid()*/)
//            {
//                EV << NOW << "FATAL! Ue " << nodeId_ << " Current Process is " << (int)currentHarq << " while Stored grant refers to acid " << /*(int)grant_->getAcid() << */  ". Aborting.   " << endl;
//                abort();
//            }
//        }

        // TODO check if current grant is "NEW TRANSMISSION" or "RETRANSMIT" (periodic grants shall always be "newtx"
//        if ( false/*!grant_->isNewTx() && harqQueue_->rtx(currentHarq) */)
//        {
        //        if ( LteDebug:r:trace("LteMacUe::newSubFrame") )
        //            fprintf (stderr,"%.9f UE: [%d] Triggering retransmission for acid %d\n",NOW,nodeId_,currentHarq);
        //        // triggering retransmission --- nothing to do here, really!
//        } else {
        // buffer drop should occour here.
//        scheduleList = ueScheduler_->buildSchedList();

        EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        HarqTxBuffers::iterator it2;
        LteHarqBufferTx * currHarq;
        for(it2 = harqTxBuffers_.begin(); it2 != harqTxBuffers_.end(); it2++)
        {
            EV << "\t Looking for retx in acid " << (unsigned int)currentHarq_ << endl;
            currHarq = it2->second;

            // check if the current process has unit ready for retx
            retx = currHarq->getProcess(currentHarq_)->hasReadyUnits();
            CwList cwListRetx = currHarq->getProcess(currentHarq_)->readyUnitsIds();

            EV << "\t [process=" << (unsigned int)currentHarq_ << "] , [retx=" << ((retx)?"true":"false")
               << "] , [n=" << cwListRetx.size() << "]" << endl;

            // if a retransmission is needed
            if(retx)
            {
                UnitList signal;
                signal.first=currentHarq_;
                signal.second = cwListRetx;
                currHarq->markSelected(signal,schedulingGrant_->getUserTxParams()->getLayers().size());
            }
        }
        // if no retx is needed, proceed with normal scheduling
        if(!retx)
        {
            scheduleList_ = lcgScheduler_->schedule();
            requestedSdus_ = macSduRequest();
            if (requestedSdus_ == 0)
            {
                // no data to send, but if bsrTriggered is set, send a BSR
                macPduMake();
            }

        }

        // Message that triggers flushing of Tx H-ARQ buffers for all users
        // This way, flushing is performed after the (possible) reception of new MAC PDUs
        cMessage* flushHarqMsg = new cMessage("flushHarqMsg");
        flushHarqMsg->setSchedulingPriority(1);        // after other messages
        scheduleAt(NOW, flushHarqMsg);

//        // deleting non-periodic grant
//        if (!schedulingGrant_->getPeriodic())
//        {
//            delete schedulingGrant_;
//            schedulingGrant_=NULL;
//        }


    }

    //============================ DEBUG ==========================
    if (debugHarq_)
    {
        // TODO make this part optional to save computations
        HarqTxBuffers::iterator it;

        EV << "\n htxbuf.size " << harqTxBuffers_.size() << endl;

        int cntOuter = 0;
        int cntInner = 0;
        for(it = harqTxBuffers_.begin(); it != harqTxBuffers_.end(); it++)
        {
            LteHarqBufferTx* currHarq = it->second;
            BufferStatus harqStatus = currHarq->getBufferStatus();
            BufferStatus::iterator jt = harqStatus.begin(), jet= harqStatus.end();

            EV << "\t cicloOuter " << cntOuter << " - bufferStatus.size=" << harqStatus.size() << endl;
            for(; jt != jet; ++jt)
            {
                EV << "\t\t cicloInner " << cntInner << " - jt->size=" << jt->size()
                   << " - statusCw(0/1)=" << jt->at(0).second << "/" << jt->at(1).second << endl;
            }
        }
    }
    //======================== END DEBUG ==========================

    unsigned int purged =0;
    // purge from corrupted PDUs all Rx H-HARQ buffers
    for (hit= harqRxBuffers_.begin(); hit != het; ++hit)
    {
        purged += hit->second->purgeCorruptedPdus();
    }
    EV << NOW << " LteMacUe::handleSelfMessage Purged " << purged << " PDUS" << endl;

    if (requestedSdus_ == 0)
    {
        // update current harq process id
        currentHarq_ = (currentHarq_+1) % harqProcesses_;
    }

    EV << "--- END UE MAIN LOOP ---" << endl;
}

void
LteMacUe::macHandleGrant(cPacket* pkt)
{
    EV << NOW << " LteMacUe::macHandleGrant - UE [" << nodeId_ << "] - Grant received" << endl;
    // delete old grant
    LteSchedulingGrant* grant = check_and_cast<LteSchedulingGrant*>(pkt);
    EV << NOW << " LteMacUe::macHandleGrant - Direction: " << dirToA(grant->getDirection()) << endl;

    //Codeword cw = grant->getCodeword();

    if (schedulingGrant_!=NULL)
    {
        delete schedulingGrant_;
        schedulingGrant_ = NULL;
    }

    // store received grant
    schedulingGrant_=grant;

    if (grant->getPeriodic())
    {
        periodCounter_=grant->getPeriod();
        expirationCounter_=grant->getExpiration();
    }

    EV << NOW << "Node " << nodeId_ << " received grant of blocks " << grant->getTotalGrantedBlocks()
       << ", bytes " << grant->getGrantedCwBytes(0) << endl;
//        TODO if (!grant_->isNewTx())
//            {
//            }

    // clearing pending RAC requests
    racRequested_=false;
}

void
LteMacUe::macHandleRac(cPacket* pkt)
{
    LteRac* racPkt = check_and_cast<LteRac*>(pkt);

    if (racPkt->getSuccess())
    {
        EV << "LteMacUe::macHandleRac - Ue " << nodeId_ << " won RAC" << endl;
        // is RAC is won, BSR has to be sent
        bsrTriggered_=true;
        // reset RAC counter
        currentRacTry_=0;
        //reset RAC backoff timer
        racBackoffTimer_=0;
    }
    else
    {
        // RAC has failed
        if (++currentRacTry_ >= maxRacTryouts_)
        {
            EV << NOW << " Ue " << nodeId_ << ", RAC reached max attempts : " << currentRacTry_ << endl;
            // no more RAC allowed
            //! TODO flush all buffers here
            //reset RAC counter
            currentRacTry_=0;
            //reset RAC backoff timer
            racBackoffTimer_=0;
        }
        else
        {
            // recompute backoff timer
            racBackoffTimer_= uniform(minRacBackoff_,maxRacBackoff_);
            EV << NOW << " Ue " << nodeId_ << " RAC attempt failed, backoff extracted : " << racBackoffTimer_ << endl;
        }
    }
    delete racPkt;
}

void
LteMacUe::checkRAC()
{
    EV << NOW << " LteMacUe::checkRAC , Ue  " << nodeId_ << ", racTimer : " << racBackoffTimer_ << " maxRacTryOuts : " << maxRacTryouts_
       << ", raRespTimer:" << raRespTimer_ << endl;

    if (racBackoffTimer_>0)
    {
        racBackoffTimer_--;
        return;
    }

    if(raRespTimer_>0)
    {
        // decrease RAC response timer
        raRespTimer_--;
        EV << NOW << " LteMacUe::checkRAC - waiting for previous RAC requests to complete (timer=" << raRespTimer_ << ")" << endl;
        return;
    }

    //     Avoids double requests whithin same TTI window
    if (racRequested_)
    {
        EV << NOW << " LteMacUe::checkRAC - double RAC request" << endl;
        racRequested_=false;
        return;
    }

    bool trigger=false;

    LteMacBufferMap::const_iterator it;

    for (it = macBuffers_.begin(); it!=macBuffers_.end();++it)
    {
        if (!(it->second->isEmpty()))
        {
            trigger = true;
            break;
        }
    }

    if (!trigger)
    EV << NOW << "Ue " << nodeId_ << ",RAC aborted, no data in queues " << endl;

    if ((racRequested_=trigger))
    {
        LteRac* racReq = new LteRac("RacRequest");
        UserControlInfo* uinfo = new UserControlInfo();
        uinfo->setSourceId(getMacNodeId());
        uinfo->setDestId(getMacCellId());
        uinfo->setDirection(UL);
        uinfo->setFrameType(RACPKT);
        racReq->setControlInfo(uinfo);

        sendLowerPackets(racReq);

        EV << NOW << " Ue  " << nodeId_ << " cell " << cellId_ << " ,RAC request sent to PHY " << endl;

        // wait at least  "raRespWinStart_" TTIs before another RAC request
        raRespTimer_ = raRespWinStart_;
    }
}

void
LteMacUe::updateUserTxParam(cPacket* pkt)
{
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>
        (pkt->getControlInfo());

    if (lteInfo->getFrameType() != DATAPKT)
        return;

    lteInfo->setUserTxParams(schedulingGrant_->getUserTxParams()->dup());

    lteInfo->setTxMode(schedulingGrant_->getUserTxParams()->readTxMode());

    int grantedBlocks = schedulingGrant_->getTotalGrantedBlocks();

    lteInfo->setGrantedBlocks(schedulingGrant_->getGrantedBlocks());
    lteInfo->setTotalGrantedBlocks(grantedBlocks);
}

void LteMacUe::flushHarqBuffers()
{
    // send the selected units to lower layers
    HarqTxBuffers::iterator it2;
    for(it2 = harqTxBuffers_.begin(); it2 != harqTxBuffers_.end(); it2++)
        it2->second->sendSelectedDown();

    // deleting non-periodic grant
    if (schedulingGrant_ != NULL && !schedulingGrant_->getPeriodic())
    {
        delete schedulingGrant_;
        schedulingGrant_=NULL;
    }
}

bool
LteMacUe::getHighestBackloggedFlow(MacCid& cid, unsigned int& priority)
{
    // TODO : optimize if inefficient
    // TODO : implement priorities and LCGs
    // search in virtual buffer structures

    LteMacBufferMap::const_iterator it = macBuffers_.begin(), et = macBuffers_.end();
    for (; it != et; ++it)
    {
        if (!it->second->isEmpty())
        {
            cid = it->first;
            // TODO priority = something;
            return true;
        }
    }
    return false;
}

bool
LteMacUe::getLowestBackloggedFlow(MacCid& cid, unsigned int& priority)
{
    // TODO : optimize if inefficient
    // TODO : implement priorities and LCGs
    LteMacBufferMap::const_reverse_iterator it = macBuffers_.rbegin(), et = macBuffers_.rend();
    for (; it != et; ++it)
    {
        if (!it->second->isEmpty())
        {
            cid = it->first;
            // TODO priority = something;
            return true;
        }
    }

    return false;
}

void LteMacUe::doHandover(MacNodeId targetEnb)
{
    cellId_ = targetEnb;
}

void LteMacUe::deleteQueues(MacNodeId nodeId)
{
    LteMacBuffers::iterator mit;
    LteMacBufferMap::iterator vit;
    for (mit = mbuf_.begin(); mit != mbuf_.end(); )
    {
        while (!mit->second->isEmpty())
        {
            cPacket* pkt = mit->second->popFront();
            delete pkt;
        }
        delete mit->second;        // Delete Queue
        mbuf_.erase(mit++);        // Delete Elem
    }
    for (vit = macBuffers_.begin(); vit != macBuffers_.end(); )
    {
        while (!vit->second->isEmpty())
            vit->second->popFront();
        delete vit->second;                  // Delete Queue
        macBuffers_.erase(vit++);           // Delete Elem
    }

    // delete H-ARQ buffers
    HarqTxBuffers::iterator hit;
    for (hit = harqTxBuffers_.begin(); hit != harqTxBuffers_.end(); )
    {
        delete hit->second; // Delete Queue
        harqTxBuffers_.erase(hit++); // Delete Elem
    }
    HarqRxBuffers::iterator hit2;
    for (hit2 = harqRxBuffers_.begin(); hit2 != harqRxBuffers_.end();)
    {
         delete hit2->second; // Delete Queue
         harqRxBuffers_.erase(hit2++); // Delete Elem
    }

    // remove traffic descriptor and lcg entry
    lcgMap_.clear();
    connDesc_.clear();
}
