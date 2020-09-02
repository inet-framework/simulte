//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


#include "stack/mac/layer/LteMacBase.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "common/LteControlInfo.h"
#include "corenetwork/binder/LteBinder.h"
#include "corenetwork/lteCellInfo/LteCellInfo.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "assert.h"

using namespace omnetpp;

LteMacBase::LteMacBase()
{
    mbuf_.clear();
    macBuffers_.clear();
}

LteMacBase::~LteMacBase()
{
    LteMacBuffers::iterator mit;
    LteMacBufferMap::iterator vit;
    for (mit = mbuf_.begin(); mit != mbuf_.end(); mit++)
        delete mit->second;
    for (vit = macBuffers_.begin(); vit != macBuffers_.end(); vit++)
        delete vit->second;
    mbuf_.clear();
    macBuffers_.clear();

    HarqTxBuffers::iterator htit;
    HarqRxBuffers::iterator hrit;
    for (htit = harqTxBuffers_.begin(); htit != harqTxBuffers_.end(); ++htit)
        delete htit->second;
    for (hrit = harqRxBuffers_.begin(); hrit != harqRxBuffers_.end(); ++hrit)
        delete hrit->second;
    harqTxBuffers_.clear();
    harqRxBuffers_.clear();
}

void LteMacBase::sendUpperPackets(cPacket* pkt)
{
    EV << NOW << " LteMacBase::sendUpperPackets, Sending packet " << pkt->getName() << " on port MAC_to_RLC\n";
    // Send message
    send(pkt,up_[OUT_GATE]);
    nrToUpper_++;
    emit(sentPacketToUpperLayer, pkt);
}

void LteMacBase::sendLowerPackets(cPacket* pkt)
{
    EV << NOW << "LteMacBase::sendLowerPackets, Sending packet " << pkt->getName() << " on port MAC_to_PHY\n";
    // Send message
    updateUserTxParam(pkt);
    send(pkt,down_[OUT_GATE]);
    nrToLower_++;
    emit(sentPacketToLowerLayer, pkt);
}

/*
 * Ue with nodeId left the simulation. Ensure that no
 * signales will be emitted via the deleted node.
 */
void LteMacBase::unregisterHarqBufferRx(MacNodeId nodeId){
    HarqRxBuffers::iterator it = harqRxBuffers_.find(nodeId);
    if (it != harqRxBuffers_.end()){
        it->second->unregister_macUe();
    }
}

/*
 * Upper layer handler
 */
void LteMacBase::fromRlc(cPacket *pkt)
{
    handleUpperMessage(pkt);
}

/*
 * Lower layer handler
 */
void LteMacBase::fromPhy(cPacket *pktAux)
{
    // TODO: harq test (comment fromPhy: it has only to pass pdus to proper rx buffer and
    // to manage H-ARQ feedback)

    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto userInfo = pkt->getTag<UserControlInfo>();

    MacNodeId src = userInfo->getSourceId();

    if (userInfo->getFrameType() == HARQPKT)
    {
        // H-ARQ feedback, send it to TX buffer of source
        HarqTxBuffers::iterator htit = harqTxBuffers_.find(src);
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received HARQ Feedback pkt" << endl;
        if (htit == harqTxBuffers_.end())
        {
            // if a feedback arrives, a tx buffer must exists (unless it is an handover scenario
            // where the harq buffer was deleted but a feedback was in transit)
            // this case must be taken care of

            if (binder_->hasUeHandoverTriggered(nodeId_) || binder_->hasUeHandoverTriggered(src))
                return;

            throw cRuntimeError("Mac::fromPhy(): Received feedback for an unexisting H-ARQ tx buffer");
        }

        auto hfbpkt = pkt->peekAtFront<LteHarqFeedback>();
        htit->second->receiveHarqFeedback(pkt);
    }
    else if (userInfo->getFrameType() == FEEDBACKPKT)
    {
        //Feedback pkt
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received feedback pkt" << endl;
        macHandleFeedbackPkt(pkt);
    }
    else if (userInfo->getFrameType()==GRANTPKT)
    {
        //Scheduling Grant
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received Scheduling Grant pkt" << endl;
        macHandleGrant(pkt);
    }
    else if(userInfo->getFrameType() == DATAPKT)
    {
        // data packet: insert in proper rx buffer
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received DATA packet" << endl;

        auto pduAux = pkt->peekAtFront<LteMacPdu>();
        auto pdu = pkt;
        Codeword cw = userInfo->getCw();
        HarqRxBuffers::iterator hrit = harqRxBuffers_.find(src);
        if (hrit != harqRxBuffers_.end())
        {
            hrit->second->insertPdu(cw,pdu);
        }
        else
        {
            // FIXME: possible memory leak
            LteHarqBufferRx *hrb;
            if (userInfo->getDirection() == DL || userInfo->getDirection() == UL)
                hrb = new LteHarqBufferRx(ENB_RX_HARQ_PROCESSES, this,src);
            else // D2D
                hrb = new LteHarqBufferRxD2D(ENB_RX_HARQ_PROCESSES, this,src, (userInfo->getDirection() == D2D_MULTI) );

            harqRxBuffers_[src] = hrb;
            hrb->insertPdu(cw,pdu);
        }
    }
    else if (userInfo->getFrameType() == RACPKT)
    {
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received RAC packet" << endl;
        macHandleRac(pkt);
    }
    else
    {
        throw cRuntimeError("Unknown packet type %d", (int)userInfo->getFrameType());
    }
}

bool LteMacBase::bufferizePacket(cPacket* pkt)
{
    pkt->setTimestamp();        // Add timestamp with current time to packet

    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

    // obtain the cid from the packet informations
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // build the virtual packet corresponding to this incoming packet
    PacketInfo vpkt(pkt->getByteLength(), pkt->getTimestamp());

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end())
    {
        // Queue not found for this cid: create
        LteMacQueue* queue = new LteMacQueue(queueSize_);
        take(queue);
        LteMacBuffer* vqueue = new LteMacBuffer();

        queue->pushBack(pkt);
        vqueue->pushBack(vpkt);
        mbuf_[cid] = queue;
        macBuffers_[cid] = vqueue;

        // make a copy of lte control info and store it to traffic descriptors map
        FlowControlInfo toStore(*lteInfo);
        connDesc_[cid] = toStore;
        // register connection to lcg map.
        LteTrafficClass tClass = (LteTrafficClass) lteInfo->getTraffic();

        lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));

        EV << "LteMacBuffers : Using new buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }
    else
    {
        // Found
        LteMacQueue* queue = it->second;
        LteMacBuffer* vqueue = macBuffers_.find(cid)->second;
        if (!queue->pushBack(pkt))
        {
            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
            if (lteInfo->getDirection()==DL)
            {
                emit(macBufferOverflowDl_,sample);
            }
            else if (lteInfo->getDirection()==UL)
            {
                emit(macBufferOverflowUl_,sample);
            }
            else // D2D
            {
                emit(macBufferOverflowD2D_,sample);
            }

            EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
            delete pkt;
            return false;
        }
        vqueue->pushBack(vpkt);

        EV << "LteMacBuffers : Using old buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }
        /// After bufferization buffers must be synchronized
    assert(mbuf_[cid]->getQueueLength() == macBuffers_[cid]->getQueueLength());
    return true;
}

void LteMacBase::deleteQueues(MacNodeId nodeId)
{
    LteMacBuffers::iterator mit;
    LteMacBufferMap::iterator vit;
    for (mit = mbuf_.begin(); mit != mbuf_.end(); )
    {
        if (MacCidToNodeId(mit->first) == nodeId)
        {
            while (!mit->second->isEmpty())
            {
                cPacket* pkt = mit->second->popFront();
                delete pkt;
            }
            delete mit->second;        // Delete Queue
            mbuf_.erase(mit++);        // Delete Elem
        }
        else
        {
            ++mit;
        }
    }
    for (vit = macBuffers_.begin(); vit != macBuffers_.end(); )
    {
        if (MacCidToNodeId(vit->first) == nodeId)
        {
            while (!vit->second->isEmpty())
                vit->second->popFront();
            delete vit->second;        // Delete Queue
            macBuffers_.erase(vit++);        // Delete Elem
        }
        else
        {
            ++vit;
        }
    }

    // delete H-ARQ buffers
    HarqTxBuffers::iterator hit;
    for (hit = harqTxBuffers_.begin(); hit != harqTxBuffers_.end(); )
    {
        if (hit->first == nodeId)
        {
            delete hit->second; // Delete Queue
            harqTxBuffers_.erase(hit++); // Delete Elem
        }
        else
        {
            ++hit;
        }
    }
    HarqRxBuffers::iterator hit2;
    for (hit2 = harqRxBuffers_.begin(); hit2 != harqRxBuffers_.end();)
    {
        if (hit2->first == nodeId)
        {
            delete hit2->second; // Delete Queue
            harqRxBuffers_.erase(hit2++); // Delete Elem
        }
        else
        {
            ++hit2;
        }
    }

    // TODO remove traffic descriptor and lcg entry
}


/*
 * Main functions
 */
void LteMacBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        /* Gates initialization */
        up_[IN_GATE] = gate("RLC_to_MAC");
        up_[OUT_GATE] = gate("MAC_to_RLC");
        down_[IN_GATE] = gate("PHY_to_MAC");
        down_[OUT_GATE] = gate("MAC_to_PHY");

        /* Create buffers */
        queueSize_ = par("queueSize");

        /* Get reference to binder */
        binder_ = getBinder();

        /* Set The MAC MIB */

        muMimo_ = par("muMimo");

        harqProcesses_ = par("harqProcesses");

        /* Start TTI tick */
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);        // TTI TICK after other messages
        scheduleAt(NOW + TTI, ttiTick_);

        /* statistics */
        statDisplay_ = par("statDisplay");
        totalOverflowedBytes_ = 0;
        nrFromUpper_ = 0;
        nrFromLower_ = 0;
        nrToUpper_ = 0;
        nrToLower_ = 0;

        /* register signals */
        macBufferOverflowDl_ = registerSignal("macBufferOverFlowDl");
        macBufferOverflowUl_ = registerSignal("macBufferOverFlowUl");
        if (isD2DCapable())
            macBufferOverflowD2D_ = registerSignal("macBufferOverFlowD2D");
        receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
        receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
        sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
        sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

        measuredItbs_ = registerSignal("measuredItbs");
        WATCH(queueSize_);
        WATCH(nodeId_);
        WATCH_MAP(mbuf_);
        WATCH_MAP(macBuffers_);
    }
}

void LteMacBase::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage())
    {
        handleSelfMessage();
        scheduleAt(NOW + TTI, ttiTick_);
        return;
    }

    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LteMacBase : Received packet " << pkt->getName() <<
    " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();

    if (incoming == down_[IN_GATE])
    {
        // message from PHY_to_MAC gate (from lower layer)
        emit(receivedPacketFromLowerLayer, pkt);
        nrFromLower_++;
        fromPhy(pkt);
    }
    else
    {
        // message from RLC_to_MAC gate (from upper layer)
        emit(receivedPacketFromUpperLayer, pkt);
        nrFromUpper_++;
        fromRlc(pkt);
    }
    return;
}

void LteMacBase::finish()
{
}

void LteMacBase::deleteModule(){
    cancelAndDelete(ttiTick_);
    cSimpleModule::deleteModule();
}

void LteMacBase::refreshDisplay() const
{
    if(statDisplay_){
        char buf[80];

        sprintf(buf, "hl: %ld in, %ld out\nll: %ld in, %ld out", nrFromUpper_, nrToUpper_, nrFromLower_, nrToLower_);

        getDisplayString().setTagArg("t", 0, buf);
        getDisplayString().setTagArg("bgtt", 0, "Number of packets in and ouf the higher layer (hl) and the lower layer (ll).");
    }
}
