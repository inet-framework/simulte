
#include <cmath>
#include "apps/burst/BurstSender.h"

#define round(x) floor((x) + 0.5)

Define_Module(BurstSender);

BurstSender::BurstSender()
{
    initialized_ = false;
    initTraffic_ = NULL;
    selfSender_ = NULL;
    selfBurst_ = NULL;
    selfPacket_ = NULL;
}

BurstSender::~BurstSender()
{
    cancelAndDelete(selfBurst_);
    cancelAndDelete(selfPacket_);

}

void BurstSender::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    EV << "BurstSender initialize: stage " << stage << " - initialize=" << initialized_ << endl;

    if (stage == INITSTAGE_LOCAL)
    {
        selfBurst_ = new cMessage("selfBurst");
        selfPacket_ = new cMessage("selfPacket");
        idBurst_ = 0;
        idFrame_ = 0;
        timestamp_ = 0;
        size_ = par("packetSize");
        burstSize_ = par("burstSize");
        interBurstTime_ = par("interBurstTime");
        intraBurstTime_ = par("intraBurstTime");
        localPort_ = par("localPort");
        destPort_ = par("destPort");

        burstSentPkt_ = registerSignal("burstSentPkt");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        initTraffic_ = new cMessage("initTraffic");
        initTraffic();
    }
}

void BurstSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfBurst"))
            sendBurst();
        else if (!strcmp(msg->getName(), "selfPacket"))
            sendPacket();
        else
            initTraffic();
    }
}

void BurstSender::initTraffic()
{
    std::string destAddress = par("destAddress").stringValue();
    cModule* destModule = getModuleByPath(par("destAddress").stringValue());
    if (destModule == NULL)
    {
        // this might happen when users are created dynamically
        EV << simTime() << "BurstSender::initTraffic - destination " << destAddress << " not found" << endl;

        simtime_t offset = 0.01; // TODO check value
        scheduleAt(simTime()+offset, initTraffic_);
        EV << simTime() << "BurstSender::initTraffic - the node will retry to initialize traffic in " << offset << " seconds " << endl;
    }
    else
    {
        delete initTraffic_;

        destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());
        socket.setOutputGate(gate("udpOut"));
        socket.bind(localPort_);

        EV << simTime() << "BurstSender::initialize - binding to port: local:" << localPort_ << " , dest: " << destAddress_.str() << ":" << destPort_ << endl;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        // TODO maybe un-necesessary
        // this conversion is made in order to obtain ms-aligned start time, even in case of random generated ones
        simtime_t offset = (round(SIMTIME_DBL(startTime)*1000)/1000);

        scheduleAt(simTime()+startTime, selfBurst_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void BurstSender::sendBurst()
{
    idBurst_++;
    idFrame_ = 0;

    EV << simTime() << " BurstSender::sendBurst - Start sending burst[" << idBurst_ << "]" << endl;

    // send first packet
    sendPacket();

    scheduleAt(simTime() + interBurstTime_, selfBurst_);
}


void BurstSender::sendPacket()
{
    EV << "BurstSender::sendPacket - Sending frame[" << idFrame_ << "] of burst [" << idBurst_ << "], next packet at "<< simTime() + intraBurstTime_ << endl;

    //unsigned int msgId = (idBurst_ << 16) | idFrame_;
    unsigned int msgId = (idBurst_ * burstSize_) + idFrame_;

    BurstPacket* packet = new BurstPacket("Burst");
    packet->setMsgId(msgId);
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);

    socket.sendTo(packet, destAddress_, destPort_);

    emit(burstSentPkt_, (long)msgId);

    idFrame_++;
    if (idFrame_ < burstSize_)
        scheduleAt(simTime() + intraBurstTime_, selfPacket_);
}
