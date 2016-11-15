//
//                           SimuLTE
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
    tSample_ = NULL;
    tSampleCell_ = NULL;
}

UmRxQueue::~UmRxQueue()
{
    if(tSampleCell_)
        delete tSampleCell_;
    if(tSample_)
        delete tSample_;
}

bool UmRxQueue::defragment(cPacket* pkt)
{
    EV << NOW << "UmRxQueue::defragment" << endl;

    Enter_Method("defragment()"); // Direct Method Call
    take(pkt); // Take ownership

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

    pduDelay = (NOW - pkt->getCreationTime()).dbl();
    bool first = fragbuf_.insert(pktId, totFrag, fragSno, fragSize, lteInfo);
    tSample_->sample_ = pkt->getByteLength();

    MacNodeId ueId = ctrlInfoToUeId(lteInfo);
    if (ue_ == NULL)
    {
        ue_ = getRlcByMacNodeId(ueId, UM);
        if(ue_ == NULL) {
            // UE has left the simulation
            fragbuf_.remove(pktId);
            delete lteInfo;
            delete pkt;
            return false;
        }
    }
    tSample_->id_ = ueId;
    tSample_->module_=ue_;

    if (lteInfo->getDirection() != D2D)
    {
        ue_->emit(rlcPduThroughput_, tSample_);
        tSample_->sample_ = pduDelay;
        ue_->emit(rlcPduDelay_, tSample_);
        tSample_->sample_ = 0;
        ue_->emit(rlcPduPacketLoss_, tSample_);
    }
    else
    {
        ue_->emit(rlcPduThroughputD2D_, tSample_);
        tSample_->sample_ = pduDelay;
        ue_->emit(rlcPduDelayD2D_, tSample_);
        tSample_->sample_ = 0;
        ue_->emit(rlcPduPacketLossD2D_, tSample_);
    }

    // Insert and check if all fragments have been gathered
    if (first)
    {
        // First fragment for this pktID
        timer_.add(timeout_, pktId);// Start timer
    }
    if (!fragbuf_.check(pktId))
    {
        EV << "UmRxBuffer : Fragments missing... continue\n";
        // we must keep the first lteInfo since it is stored in the fragment-buffer
        if(!first)
            delete lteInfo;
        delete pkt;
        return false; // Missing fragments
    }
    else
    {
        EV << "\tUmRxBuffer - All fragments gathered: defragment" << endl;

        // All fragments gathered: defragment
        timer_.remove(pktId);// Stop timer
        LteRlcUm* lte_rlc = check_and_cast<LteRlcUm *>(
            getParentModule()->getSubmodule("um"));
        int origPktSize = fragbuf_.remove(pktId);
        // Set original packet size before decapsulating
        pkt->setByteLength(origPktSize);

        LteRlcSdu* rlcPkt = check_and_cast<LteRlcSdu*>(fragment->decapsulate());
        size = rlcPkt->getByteLength();
        delay = (NOW - rlcPkt->getCreationTime()).dbl();
        LtePdcpPdu* pdcpPkt = check_and_cast<LtePdcpPdu*>(
            rlcPkt->decapsulate());
        pdcpPkt->setControlInfo(lteInfo);

        ASSERT(rlcPkt->getOwner() == this);
        delete rlcPkt;

        drop(pdcpPkt);
        lte_rlc->sendDefragmented(pdcpPkt);
    }

    delete pkt;

    tSample_->sample_ = size;
    tSampleCell_->sample_ = size;

    if (lteInfo->getDirection() != D2D)
    {
        nodeB_->emit(rlcCellThroughput_, tSampleCell_);
        ue_->emit(rlcThroughput_, tSample_);

        tSample_->sample_ = delay;
        ue_->emit(rlcDelay_, tSample_);

        tSample_->sample_ = 0;
        tSampleCell_->sample_ = 0;
        ue_->emit(rlcPacketLoss_, tSample_);
        nodeB_->emit(rlcCellPacketLoss_, tSampleCell_);
    }
    else
    {
        nodeB_->emit(rlcCellThroughputD2D_, tSampleCell_);
        ue_->emit(rlcThroughputD2D_, tSample_);

        tSample_->sample_ = delay;
        ue_->emit(rlcDelayD2D_, tSample_);

        tSample_->sample_ = 0;
        tSampleCell_->sample_ = 0;
        ue_->emit(rlcPacketLossD2D_, tSample_);
        nodeB_->emit(rlcCellPacketLossD2D_, tSampleCell_);
    }

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

    nodeB_ = getRlcByMacNodeId(mac->getMacCellId(), UM);

    tSampleCell_->id_ = mac->getMacCellId();
    tSampleCell_->module_ = nodeB_;

    if (mac->getNodeType() == ENODEB)
    {
        ue_ = NULL;

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
        ue_ = parent;

        rlcPacketLoss_ = parent->registerSignal("rlcPacketLossDl");
        rlcPduPacketLoss_ = parent->registerSignal("rlcPduPacketLossDl");
        rlcDelay_ = parent->registerSignal("rlcDelayDl");
        rlcThroughput_ = parent->registerSignal("rlcThroughputDl");
        rlcPduDelay_ = parent->registerSignal("rlcPduDelayDl");
        rlcPduThroughput_ = parent->registerSignal("rlcPduThroughputDl");

        rlcCellThroughput_ = nodeB_->registerSignal("rlcCellThroughputDl");
        rlcCellPacketLoss_ = nodeB_->registerSignal("rlcCellPacketLossDl");

        if (mac->isD2DCapable())
        {
            rlcPacketLossD2D_ = parent->registerSignal("rlcPacketLossD2D");
            rlcPduPacketLossD2D_ = parent->registerSignal("rlcPduPacketLossD2D");
            rlcDelayD2D_ = parent->registerSignal("rlcDelayD2D");
            rlcThroughputD2D_ = parent->registerSignal("rlcThroughputD2D");
            rlcPduDelayD2D_ = parent->registerSignal("rlcPduDelayD2D");
            rlcPduThroughputD2D_ = parent->registerSignal("rlcPduThroughputD2D");

            rlcCellThroughputD2D_ = nodeB_->registerSignal("rlcCellThroughputD2D");
            rlcCellPacketLossD2D_ = nodeB_->registerSignal("rlcCellPacketLossD2D");
        }
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

        // source or destination might have been removed from the simulation
        if(getBinder()->getOmnetId(srcId) == 0 || getBinder()->getOmnetId(dstId) == 0){
                // remove from fagment buffer (includes deletion of lteInfo)
                fragbuf_.remove(tmsg->getEvent());
                delete tmsg;
                return;
        }

        unsigned short dir = lteInfo->getDirection();
        tSample_->sample_ = 1;
        tSampleCell_->sample_ = 1;

        MacNodeId ueId = ctrlInfoToUeId(lteInfo);
        if (ue_ == NULL)
        {
            ue_ = getRlcByMacNodeId(ueId, UM);
        }
        tSampleCell_->module_ = nodeB_;
        tSample_->id_ = ueId;
        tSample_->module_ = ue_;

        if (dir != D2D)
        {
            ue_->emit(rlcPacketLoss_, tSample_);
            nodeB_->emit(rlcCellPacketLoss_, tSampleCell_);
        }
        else
        {
            ue_->emit(rlcPacketLossD2D_, tSample_);
            nodeB_->emit(rlcCellPacketLossD2D_, tSampleCell_);
        }


	// remove from fagment buffer (includes deletion of lteInfo)
        fragbuf_.remove(tmsg->getEvent());
        timer_.handle(tmsg->getEvent()); // Stop timer

        delete tmsg;
    }
}
