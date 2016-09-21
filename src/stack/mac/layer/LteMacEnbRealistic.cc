//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteMacEnbRealistic.h"
#include "LteHarqBufferRx.h"
#include "LteMacBuffer.h"
#include "LteMacQueue.h"
#include "LteFeedbackPkt.h"
#include "LteSchedulerEnbDl.h"
#include "LteSchedulerEnbDlRealistic.h"
#include "LteSchedulerEnbUl.h"
#include "LteSchedulingGrant.h"
#include "LteAllocationModule.h"
#include "LteAmc.h"
#include "UserTxParams.h"
#include "LteRac_m.h"
#include "LteCommon.h"
#include "LteMacSduRequest.h"

Define_Module( LteMacEnbRealistic);

/*********************
 * PUBLIC FUNCTIONS
 *********************/

LteMacEnbRealistic::LteMacEnbRealistic() :
    LteMacEnb()
{
    scheduleListDl_ = NULL;
}

LteMacEnbRealistic::~LteMacEnbRealistic()
{
}

/***********************
 * PROTECTED FUNCTIONS
 ***********************/

void LteMacEnbRealistic::initialize(int stage)
{
    if (stage == 0)
    {
        // check the RLC module type: if it is not "realistic", abort simulation
        // TODO do the same for RLC AM
        std::string rlcUmType = getParentModule()->getSubmodule("rlc")->par("LteRlcUmType").stdstringValue();
        std::string macType = getParentModule()->par("LteMacType").stdstringValue();
        if (macType.compare("LteMacEnbRealistic") == 0 && rlcUmType.compare("LteRlcUmRealistic") != 0)
            throw cRuntimeError("LteMacEnbRealistic::initialize - %s module found, must be LteRlcUmRealistic. Aborting", rlcUmType.c_str());

        /* Create and initialize MAC Downlink scheduler */
        enbSchedulerDl_ = new LteSchedulerEnbDlRealistic();
        enbSchedulerDl_->initialize(DL, this);

        /* Create and initialize MAC Uplink scheduler */
        enbSchedulerUl_ = new LteSchedulerEnbUl();
        enbSchedulerUl_->initialize(UL, this);
    }
    LteMacEnb::initialize(stage);
}

void LteMacEnbRealistic::handleMessage(cMessage* msg)
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

void LteMacEnbRealistic::macSduRequest()
{
    EV << "----- START LteMacEnbRealistic::macSduRequest -----\n";

    // Ask for a MAC sdu for each scheduled user on each codeword
    LteMacScheduleList::const_iterator it;
    for (it = scheduleListDl_->begin(); it != scheduleListDl_->end(); it++)
    {
        MacCid destCid = it->first.first;
        Codeword cw = it->first.second;
        MacNodeId destId = MacCidToNodeId(destCid);

        // for each band, count the number of bytes allocated for this ue (dovrebbe essere per cid)
        unsigned int allocatedBytes = 0;
        int numBands = deployer_->getNumBands();
        for (Band b=0; b < numBands; b++)
        {
            // get the number of bytes allocated to this connection
            // (this represents the MAC PDU size)
            allocatedBytes += enbSchedulerDl_->allocator_->getBytes(MACRO,b,destId);
        }

        // send the request message to the upper layer
        LteMacSduRequest* macSduRequest = new LteMacSduRequest("LteMacSduRequest");
        macSduRequest->setUeId(destId);
        macSduRequest->setLcid(MacCidToLcid(destCid));
        macSduRequest->setSduSize(allocatedBytes - MAC_HEADER);    // do not consider MAC header size
        macSduRequest->setControlInfo((&connDesc_[destCid])->dup());
        sendUpperPackets(macSduRequest);
    }

    EV << "------ END LteMacEnbRealistic::macSduRequest ------\n";
}

void LteMacEnbRealistic::macPduMake(MacCid cid)
{
    EV << "----- START LteMacEnbRealistic::macPduMake -----\n";
    // Finalizes the scheduling decisions according to the schedule list,
    // detaching sdus from real buffers.

    macPduList_.clear();

    //  Build a MAC pdu for each scheduled user on each codeword
    LteMacScheduleList::const_iterator it;
    for (it = scheduleListDl_->begin(); it != scheduleListDl_->end(); it++)
    {
        LteMacPdu* macPkt;
        cPacket* pkt;
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

            drop(pkt);
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
    EV << "------ END LteMacEnbRealistic::macPduMake ------\n";
}

bool LteMacEnbRealistic::bufferizePacket(cPacket* pkt)
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
            tSample_->id_=nodeId_;
            tSample_->sample_=pkt->getByteLength();
            if (lteInfo->getDirection()==DL)
            {
                emit(macBufferOverflowDl_,tSample_);
            }
            else
            {
                emit(macBufferOverflowUl_,tSample_);
            }

            EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
            delete pkt;
            return false;
        }

        EV << "LteMacBuffers : Using old buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }

    return false; // do not need to notify the activation of the connection (already done when received newDataPkt)
}

void LteMacEnbRealistic::handleUpperMessage(cPacket* pkt)
{
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());
    MacCid cid = idToMacCid(lteInfo->getDestId(), lteInfo->getLcid());

    if (bufferizePacket(pkt))
    {
        enbSchedulerDl_->backlog(cid);
    }

    if (strcmp(pkt->getName(), "lteRlcFragment") == 0)
    {
        // new MAC SDU has been received
        if (pkt->getByteLength() == 0)
            delete pkt;
        else         // creates pdus from schedule list and puts them in harq buffers
            macPduMake(cid);
    }
    else
    {
        delete pkt;
    }
}

void LteMacEnbRealistic::handleSelfMessage()
{
    /***************
     *  MAIN LOOP  *
     ***************/
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
        // clear previous schedule list
        if (scheduleListDl_ != NULL)
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

    EV << "--- END " << ((nodeType==MACRO_ENB)?"MACRO":"MICRO") << " ENB MAIN LOOP ---" << endl;
}

void LteMacEnbRealistic::flushHarqBuffers()
{
    HarqTxBuffers::iterator it;
    for (it = harqTxBuffers_.begin(); it != harqTxBuffers_.end(); it++)
        it->second->sendSelectedDown();
}


