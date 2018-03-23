//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <cmath>
#include <fstream>
#include "apps/d2dMultihop/MultihopD2D.h"
#include "apps/d2dMultihop/TrickleTimerMsg_m.h"
#include "stack/mac/layer/LteMacBase.h"
#include "inet/common/ModuleAccess.h"  // for multicast support

#define round(x) floor((x) + 0.5)

Define_Module(MultihopD2D);

uint16_t MultihopD2D::numMultihopD2DApps = 0;

MultihopD2D::MultihopD2D()
{
    senderAppId_ = numMultihopD2DApps++;
    selfSender_ = NULL;
    localMsgId_ = 0;
}

MultihopD2D::~MultihopD2D()
{
    cancelAndDelete(selfSender_);

    if (trickleEnabled_)
    {
        std::map<unsigned int, MultihopD2DPacket*>::iterator it = last_.begin();
        for (; it != last_.end(); ++it)
            if (it->second != NULL)
                delete it->second;
        last_.clear();
    }
}

void MultihopD2D::initialize(int stage)
{
    // avoid multiple initializations
    if (stage == INITSTAGE_APPLICATION_LAYER )
    {
        EV << "MultihopD2D initialize: stage " << stage << endl;

        localPort_ = par("localPort");
        destPort_ = par("destPort");
        destAddress_ = L3AddressResolver().resolve(par("destAddress").stringValue());

        msgSize_ = par("msgSize");
        maxBroadcastRadius_ = par("maxBroadcastRadius");
        ttl_ = par("ttl");
        maxTransmissionDelay_ = par("maxTransmissionDelay");
        selfishProbability_ = par("selfishProbability");

        trickleEnabled_ = par("trickle").boolValue();
        if (trickleEnabled_)
        {
            I_ = par("I");
            k_ = par("k");
            if (k_ <= 0)
                throw cRuntimeError("Bad value for k. It must be greater than zero");
        }

        EV << "MultihopD2D::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
        socket.setOutputGate(gate("udpOut"));
        socket.bind(localPort_);

        // for multicast support
        inet::IInterfaceTable *ift = inet::getModuleFromPar< inet::IInterfaceTable >(par("interfaceTableModule"), this);
        InterfaceEntry *ie = ift->getInterfaceByName("wlan");
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named wlan");
        inet::MulticastGroupList mgl = ift->collectMulticastGroups();
        socket.joinLocalMulticastGroups(mgl);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
        // -------------------- //

        selfSender_ = new cMessage("selfSender");

        // get references to LTE entities
        ltePhy_ = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("lteNic")->getSubmodule("phy"));
        LteMacBase* mac = check_and_cast<LteMacBase*>(getParentModule()->getSubmodule("lteNic")->getSubmodule("mac"));
        lteNodeId_ = mac->getMacNodeId();
        lteCellId_ = mac->getMacCellId();

        // register to the event generator
        eventGen_ = check_and_cast<EventGenerator*>(getModuleByPath("eventGenerator"));
        eventGen_->registerNode(this, lteNodeId_);

        // local statistics
        d2dMultihopGeneratedMsg_ = registerSignal("d2dMultihopGeneratedMsg");
        d2dMultihopSentMsg_ = registerSignal("d2dMultihopSentMsg");
        d2dMultihopRcvdMsg_ = registerSignal("d2dMultihopRcvdMsg");
        d2dMultihopRcvdDupMsg_ = registerSignal("d2dMultihopRcvdDupMsg");
        if (trickleEnabled_)
            d2dMultihopTrickleSuppressedMsg_ = registerSignal("d2dMultihopTrickleSuppressedMsg");

        // global statistics recorder
        stat_ = check_and_cast<MultihopD2DStatistics*>(getModuleByPath("d2dMultihopStatistics"));
    }
}

void MultihopD2D::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendPacket();
        else if (!strcmp(msg->getName(), "MultihopD2DPacket"))
            relayPacket(msg);
        else if (!strcmp(msg->getName(), "trickleTimer"))
            handleTrickleTimer(msg);
        else
            throw cRuntimeError("Unrecognized self message");
    }
    else
    {
        if (!strcmp(msg->getName(), "MultihopD2DPacket"))
            handleRcvdPacket(msg);
        else
            throw cRuntimeError("Unrecognized self message");
    }
}

void MultihopD2D::handleEvent(unsigned int eventId)
{
    Enter_Method_Silent("MultihopD2D::handleEvent()");
    EV << simTime() << " MultihopD2D::handleEvent - Received event notification " << endl;

    // the event id will be part of the message id
    localMsgId_ = eventId;

    simtime_t delay = 0.0;
    if (maxTransmissionDelay_ > 0)
        delay = uniform(0, maxTransmissionDelay_);

    simtime_t t = simTime() + (round(SIMTIME_DBL(delay)*1000)/1000);
    scheduleAt(t, selfSender_);
}

void MultihopD2D::sendPacket()
{
    // build the global identifier
    uint32_t msgId = ((uint32_t)senderAppId_ << 16) | localMsgId_;

    // create new packet
    MultihopD2DPacket* packet = new MultihopD2DPacket("MultihopD2DPacket");
    packet->setMsgid(msgId);
    packet->setSrcId(lteNodeId_);
    packet->setTimestamp(simTime());
    packet->setByteLength(msgSize_);
    packet->setTtl(ttl_-1);
    packet->setHops(1);                // first hop
    packet->setLastHopSenderId(lteNodeId_);
    if (maxBroadcastRadius_ > 0)
    {
        packet->setSrcCoord(ltePhy_->getCoord());
        packet->setMaxRadius(maxBroadcastRadius_);
    }

    EV << "MultihopD2D::sendPacket - Sending msg "<< packet->getMsgid() <<  endl;
    socket.sendTo(packet, destAddress_, destPort_);

    std::set<MacNodeId> targetSet;
    eventGen_->computeTargetNodeSet(targetSet, lteNodeId_, maxBroadcastRadius_);
    stat_->recordNewBroadcast(msgId,targetSet);
    stat_->recordReception(lteNodeId_,msgId, 0.0, 0);
    markAsRelayed(msgId);

    emit(d2dMultihopGeneratedMsg_, (long)1);
    emit(d2dMultihopSentMsg_, (long)1);
}

void MultihopD2D::handleRcvdPacket(cMessage* msg)
{
    EV << "MultihopD2D::handleRcvdPacket - Received packet from lower layer" << endl;

    MultihopD2DPacket* pkt = check_and_cast<MultihopD2DPacket*>(msg);
    pkt->removeControlInfo();
    uint32_t msgId = pkt->getMsgid();

    // check if this is a duplicate
    if (isAlreadyReceived(msgId))
    {
        if (trickleEnabled_)
            counter_[msgId]++;

        // do not need to relay the message again
        EV << "MultihopD2D::handleRcvdPacket - The message has already been received, counter = " << counter_[msgId] << endl;

        emit(d2dMultihopRcvdDupMsg_, (long)1);
        stat_->recordDuplicateReception(msgId);
        delete pkt;
    }
    else
    {
        // this is a new packet

        // mark the message as received
        markAsReceived(msgId);

        if (trickleEnabled_)
        {
            counter_[msgId] = 1;

            // store a copy of the packet
            if (last_.find(msgId) != last_.end() && last_[msgId] != NULL)
            {
                delete last_[msgId];
                last_[msgId] = NULL;
            }
            last_[msgId] = pkt->dup();
        }

        // emit statistics
        simtime_t delay = simTime() - pkt->getTimestamp();
        emit(d2dMultihopRcvdMsg_, (long)1);
        stat_->recordReception(lteNodeId_, msgId, delay, pkt->getHops());

        // === decide whether to relay the message === //

        // check for selfish behavior of the user
        if (uniform(0.0,1.0) < selfishProbability_)
        {
            // selfish user, do not relay
            EV << "MultihopD2D::handleRcvdPacket - The user is selfish, do not forward the message. " << endl;
            delete pkt;
        }
        else if (pkt->getMaxRadius() > 0 && !isWithinBroadcastArea(pkt->getSrcCoord(), pkt->getMaxRadius()))
        {
            EV << "MultihopD2D::handleRcvdPacket - The node is outside the broadcast area. Do not forward it. " << endl;
            delete pkt;
        }
        else if (pkt->getTtl() == 0)
        {
            // TTL expired
            EV << "MultihopD2D::handleRcvdPacket - The TTL for this message has expired. Do not forward it. " << endl;
            delete pkt;
        }
        else
        {
            if (trickleEnabled_)
            {
                // start Trickle interval timer
                TrickleTimerMsg* timer = new TrickleTimerMsg("trickleTimer");
                timer->setMsgid(msgId);

                simtime_t t = uniform(I_/2 , I_);
                t = round(SIMTIME_DBL(t)*1000)/1000;
                EV << "MultihopD2D::handleRcvdPacket - start Trickle interval, duration[" << t << "s]" << endl;

                scheduleAt(simTime() + t, timer);
                delete pkt;
            }
            else
            {
                // relay the message after some random backoff time
                simtime_t offset = 0.0;
                if (maxTransmissionDelay_ > 0)
                    offset = uniform(0,maxTransmissionDelay_);

                offset = round(SIMTIME_DBL(offset)*1000)/1000;
                scheduleAt(simTime() + offset, pkt);
                EV << "MultihopD2D::handleRcvdPacket - will relay the message in " << offset << "s" << endl;
            }
        }
    }
}

void MultihopD2D::handleTrickleTimer(cMessage* msg)
{
    TrickleTimerMsg* timer = check_and_cast<TrickleTimerMsg*>(msg);
    unsigned int msgId = timer->getMsgid();
    if (counter_[msgId] < k_)
    {
        EV << "MultihopD2D::handleTrickleTimer - relay the message, counter[" << counter_[msgId] << "] k[" << k_ << "]" << endl;
        relayPacket(last_[msgId]->dup());
    }
    else
    {
        EV << "MultihopD2D::handleTrickleTimer - suppressed message, counter[" << counter_[msgId] << "] k[" << k_ << "]" << endl;
        stat_->recordSuppressedMessage(msgId);
        emit(d2dMultihopTrickleSuppressedMsg_, (long)1);
    }
    delete msg;
}


void MultihopD2D::relayPacket(cMessage* msg)
{
    MultihopD2DPacket* pkt = check_and_cast<MultihopD2DPacket*>(msg);

    // increase the number of hops
    unsigned int hops = pkt->getHops();
    pkt->setHops(++hops);

    // decrease TTL
    if (pkt->getTtl() > 0)
    {
        int ttl = pkt->getTtl();
        pkt->setTtl(--ttl);
    }

    pkt->setLastHopSenderId(lteNodeId_);

    EV << "MultihopD2D::relayPacket - Relay msg " << pkt->getMsgid() << " to address " << destAddress_ << endl;
    socket.sendTo(pkt, destAddress_, destPort_);

    markAsRelayed(pkt->getMsgid());    // mark the message as relayed

    emit(d2dMultihopSentMsg_, (long)1);
    stat_->recordSentMessage(pkt->getMsgid());
}

void MultihopD2D::markAsReceived(uint32_t msgId)
{
    std::pair<uint32_t,bool> p(msgId,false);
    relayedMsgMap_.insert(p);
}

bool MultihopD2D::isAlreadyReceived(uint32_t msgId)
{
    if (relayedMsgMap_.find(msgId) == relayedMsgMap_.end())
        return false;
    return true;
}

void MultihopD2D::markAsRelayed(uint32_t msgId)
{
    relayedMsgMap_[msgId] = true;
}

bool MultihopD2D::isAlreadyRelayed(uint32_t msgId)
{
    if (relayedMsgMap_.find(msgId) == relayedMsgMap_.end()) // the message has not been received
        return false;
    if (!relayedMsgMap_[msgId])    // the message has been received but not relayed yet
        return false;

    return true;
}

bool MultihopD2D::isWithinBroadcastArea(Coord srcCoord, double maxRadius)
{
    Coord myCoord = ltePhy_->getCoord();
    double dist = myCoord.distance(srcCoord);
    if (dist < maxRadius)
        return true;

    return false;
}

void MultihopD2D::finish()
{
    // unregister from the event generator
    eventGen_->unregisterNode(this, lteNodeId_);
}
