
#include "CbrReceiver.h"

Define_Module(CbrReceiver);

simsignal_t CbrReceiver::cbrFrameLossSignal_ = registerSignal("cbrFrameLossSignal");
simsignal_t CbrReceiver::cbrFrameDelaySignal_ = registerSignal("cbrFrameDelaySignal");
simsignal_t CbrReceiver::cbrJitterSignal_ = registerSignal("cbrJitterSignal");
simsignal_t CbrReceiver::cbrReceivedThroughtput_ = registerSignal("cbrReceivedThroughtputSignal");
//simsignal_t CbrReceiver::cbrReceivedThroughtput_rate_ = registerSignal("cbrReceivedThroughtput_rateSignal");

CbrReceiver::~CbrReceiver()
{

}

void CbrReceiver::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        mInit_ = true;

        numReceived_ = 0;

        recvBytes_ = 0;
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        int port = par("localPort");
        EV << "CbrReceiver::initialize - binding to port: local:" << port << endl;
        if (port != -1)
        {
            socket.setOutputGate(gate("udpOut"));
            socket.bind(port);
        }
    }
}

void CbrReceiver::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;

    CbrPacket* pPacket = check_and_cast<CbrPacket*>(msg);

    if (pPacket == 0)
    {
        throw cRuntimeError("CbrReceiver::handleMessage - FATAL! Error when casting to Cbr packet");
    }

    numReceived_++;
    totFrames_ = pPacket->getNframes(); // XXX this value can be written just once
    int pktSize = (int)pPacket->getByteLength();

    // just to make sure we do not update recvBytes AND we avoid dividing by 0
    if( simTime() > getSimulation()->getWarmupPeriod() )
    {
        recvBytes_ += pktSize;
        simtime_t elapsedTime = simTime() - getSimulation()->getWarmupPeriod();
        emit( cbrReceivedThroughtput_, recvBytes_ / elapsedTime );
    }

    simtime_t delay = simTime()-pPacket->getTimestamp();
    emit(cbrFrameDelaySignal_,delay );

    EV << "CbrReceiver::handleMessage - Packet received: FRAME[" << pPacket->getIDframe() << "/" << pPacket->getNframes() << "] with delay["<< delay << "]" << endl;

    delete msg;
}

void CbrReceiver::finish()
{
    double lossRate = 0;
    if(totFrames_ > 0)
        lossRate = numReceived_/(totFrames_*1.0);

    emit(cbrFrameLossSignal_,lossRate);
}



