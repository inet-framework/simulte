//
//                           SimuLTE
// Copyright (C) 2012 Antonio Virdis, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "UmRxQueue.h"
#include "LteMacBase.h"
#include "LteMacEnb.h"
#include "LteRlcUm.h"

Define_Module(UmRxQueue);

UmRxQueue::UmRxQueue() :
    timer_(this)
{
}

UmRxQueue::~UmRxQueue()
{
}

bool UmRxQueue::defragment(cPacket* pkt)
{
    EV << NOW << "UmRxQueue::defragment" << endl;

    Enter_Method("defragment()"); // Direct Method Call
    take(pkt);// Take ownership

    // Get fragment info
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
        pkt->removeControlInfo());
    LteRlcPdu* fragment = check_and_cast<LteRlcPdu *>(pkt);
    unsigned short dir = lteInfo->getDirection();
    unsigned int size;
    double delay, pduDelay;
    MacNodeId srcId = lteInfo->getSourceId();
    MacNodeId dstId = lteInfo->getDestId();
    unsigned int pktId = fragment->getSnoMainPacket();
    unsigned int fragSno = fragment->getSnoFragment();
    unsigned int totFrag = fragment->getTotalFragments();
    int fragSize = fragment->getByteLength();
    LogicalCid lcid = lteInfo->getLcid();

    EV << "UmRxBuffer : Processing fragment " << fragSno << "(size " << fragSize
       << ")" << " of packet " << pktId << " with LCID: " << lcid << " [src=" << srcId << "]-[dst=" << dstId << "]\n";
//    std::cout << "UmRxBuffer : Processing fragment " << fragSno << "(size " << fragSize
//            << ")" << " of packet " << pktId << " with LCID: " << lcid << " [src=" << srcId << "]-[dst=" << dstId << "]\n";

    pduDelay = (NOW - pkt->getCreationTime()).dbl();
    bool first = fragbuf_.insert(pktId, totFrag, fragSno, fragSize, lteInfo);
    tSample_->sample_ = pkt->getByteLength();

    cModule* nodeb = NULL;
    cModule* ue = NULL;

    if (dir == DL)
    {
        tSampleCell_->id_ = srcId;
        tSample_->id_ = dstId;
        nodeb = getRlcByMacNodeId(srcId, UM);
        ue = getRlcByMacNodeId(dstId, UM);
    }
    else if (dir == UL)
    {
        tSampleCell_->id_ = dstId;
        tSample_->id_ = srcId;
        nodeb = getRlcByMacNodeId(dstId, UM);
        ue = getRlcByMacNodeId(srcId, UM);
    }

    tSampleCell_->module_=nodeb;
    tSample_->module_=ue;

    ue->emit(rlcPduThroughput_, tSample_);
    tSample_->sample_ = pduDelay;
    ue->emit(rlcPduDelay_, tSample_);
    tSample_->sample_ = 0;
    ue->emit(rlcPduPacketLoss_, tSample_);

    // Insert and check if all fragments have been gathered
    if (first)
    {
        // First fragment for this pktID
        timer_.add(timeout_, pktId);// Start timer
    }
    if (!fragbuf_.check(pktId))
    {
        EV << "UmRxBuffer : Fragments missing... continue\n";
        if (!first)
        delete lteInfo;
        delete fragment;
        return false; // Missing fragments
    }
    else
    {
//        std::cout << "\tUmRxBuffer - All fragments gathered: defragment" << endl;
        EV << "\tUmRxBuffer - All fragments gathered: defragment" << endl;
        // All fragments gathered: defragment
        timer_.remove(pktId);// Stop timer
        LteRlcUm* lte_rlc = check_and_cast<LteRlcUm *>(
            getParentModule()->getSubmodule("um"));
        int origPktSize = fragbuf_.remove(pktId);
        pkt->setByteLength(origPktSize);// Set original packet size before decapsulating

        LteRlcSdu* rlcPkt = check_and_cast<LteRlcSdu*>(fragment->decapsulate());
        size = rlcPkt->getByteLength();
        delay = (NOW - rlcPkt->getCreationTime()).dbl();
        LtePdcpPdu* pdcpPkt = check_and_cast<LtePdcpPdu*>(
            rlcPkt->decapsulate());
        delete rlcPkt;
        delete fragment;
        pdcpPkt->setControlInfo(lteInfo);

        drop(pdcpPkt);
        lte_rlc->sendDefragmented(pdcpPkt);
    }

    tSample_->sample_ = size;
    tSampleCell_->sample_ = size;

    nodeb->emit(rlcCellThroughput_, tSampleCell_);
    ue->emit(rlcThroughput_, tSample_);

    tSample_->sample_ = delay;
    ue->emit(rlcDelay_, tSample_);

    tSample_->sample_ = 0;
    tSampleCell_->sample_ = 0;
    ue->emit(rlcPacketLoss_, tSample_);
    nodeb->emit(rlcCellPacketLoss_, tSampleCell_);

    EV << "UmRxBuffer : Reassembled packet " << pktId << " with LCID: " << lcid
       << "\n";
    return true;
}

    /*
     * Main Functions
     */

void UmRxQueue::initialize()
{
    timeout_ = par("timeout").doubleValue();
    tSampleCell_ = new TaggedSample();
    tSample_ = new TaggedSample();

    cModule* parent = check_and_cast<LteRlcUm*>(
        getParentModule()->getSubmodule("um"));
    //statistics
    LteMacBase* mac = check_and_cast<LteMacBase*>(
        getParentModule()->getParentModule()->getSubmodule("mac"));

    if (mac->getNodeType() == ENODEB)
    {
        rlcCellPacketLoss_ = parent->registerSignal("rlcCellPacketLossUl");
        rlcPacketLoss_ = parent->registerSignal("rlcPacketLossUl");
        rlcPduPacketLoss_ = parent->registerSignal("rlcPduPacketLossUl");
        rlcDelay_ = parent->registerSignal("rlcDelayUl");
        rlcThroughput_ = parent->registerSignal("rlcThroughputUl");
        rlcPduDelay_ = parent->registerSignal("rlcPduDelayUl");
        rlcPduThroughput_ = parent->registerSignal("rlcPduThroughputUl");
        rlcCellThroughput_ = parent->registerSignal("rlcCellThroughputUl");
    }
    else
    {
        cModule* nodeB = getRlcByMacNodeId(mac->getMacCellId(), UM);

        rlcPacketLoss_ = parent->registerSignal("rlcPacketLossDl");
        rlcPduPacketLoss_ = parent->registerSignal("rlcPduPacketLossDl");
        rlcDelay_ = parent->registerSignal("rlcDelayDl");
        rlcThroughput_ = parent->registerSignal("rlcThroughputDl");
        rlcPduDelay_ = parent->registerSignal("rlcPduDelayDl");
        rlcPduThroughput_ = parent->registerSignal("rlcPduThroughputDl");

        rlcCellThroughput_ = nodeB->registerSignal("rlcCellThroughputDl");
        rlcCellPacketLoss_ = nodeB->registerSignal("rlcCellPacketLossDl");
    }

    WATCH(timeout_);
}

void UmRxQueue::handleMessage(cMessage* msg)
{
    if (msg->isName("timer"))
    {
        // Timeout for this main packet: discard fragments
        TMultiTimerMsg* tmsg = check_and_cast<TMultiTimerMsg *>(msg);

        EV << NOW << " UmRxQueue::handleMessage : Timeout triggered for packet "
           << tmsg->getEvent() << endl;

        FlowControlInfo* lteInfo = fragbuf_.getLteInfo(tmsg->getEvent());
        MacNodeId srcId = lteInfo->getSourceId();
        MacNodeId dstId = lteInfo->getDestId();
        unsigned short dir = lteInfo->getDirection();
        tSample_->sample_ = 1;
        tSampleCell_->sample_ = 1;

        if (dir == DL)
        {
            tSample_->id_ = dstId;
            tSampleCell_->id_ = srcId;
        }
        else if (dir == UL)
        {
            tSample_->id_ = srcId;
            tSampleCell_->id_ = dstId;
        }

        // UE module
        cModule* ue = getRlcByMacNodeId((dir == DL ? dstId : srcId), UM);
        // NODEB
        cModule* nodeb = getRlcByMacNodeId((dir == DL ? srcId : dstId), UM);

        tSampleCell_->module_ = nodeb;
        tSample_->module_ = ue;

        ue->emit(rlcPacketLoss_, tSample_);
        nodeb->emit(rlcCellPacketLoss_, tSampleCell_);

        fragbuf_.remove(tmsg->getEvent());
        timer_.handle(tmsg->getEvent()); // Stop timer
        // delete lteInfo
        delete lteInfo;
        delete tmsg;
    }
}
