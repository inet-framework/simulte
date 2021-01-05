//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/burst/BurstReceiver.h"

Define_Module(BurstReceiver);

void BurstReceiver::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        mInit_ = true;

        numReceived_ = 0;

        recvBytes_ = 0;

        burstRcvdPkt_ = registerSignal("burstRcvdPkt");
        burstPktDelay_ = registerSignal("burstPktDelay");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        int port = par("localPort");
        EV << "BurstReceiver::initialize - binding to port: local:" << port << endl;
        if (port != -1)
        {
            socket.setOutputGate(gate("socketOut"));
            socket.bind(port);
        }
    }
}

void BurstReceiver::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;

    Packet* pPacket = check_and_cast<Packet*>(msg);
    if (pPacket == nullptr)
            throw cRuntimeError("BurstReceiver::handleMessage - FATAL! Error when casting to inet::Packet");

    auto burstHeader = pPacket->popAtFront<BurstPacket>();

    numReceived_++;

    simtime_t delay = simTime()-burstHeader->getPayloadTimestamp();
    EV << "BurstReceiver::handleMessage - Packet received: FRAME[" << burstHeader->getMsgId() << "] with delay["<< delay << "]" << endl;

    emit(burstPktDelay_, delay);
    emit(burstRcvdPkt_, (long)burstHeader->getMsgId());

    delete msg;
}


