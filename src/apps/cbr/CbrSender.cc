

#include <cmath>
#include "CbrSender.h"

#define round(x) floor((x) + 0.5)

Define_Module(CbrSender);

simsignal_t CbrSender::cbrGeneratedThroughtputSignal_ = registerSignal("cbrGeneratedThroughtputSignal");
simsignal_t CbrSender::cbrGeneratedBytesSignal_ = registerSignal("cbrGeneratedBytesSignal");
simsignal_t CbrSender::cbrSentPktSignal_ = registerSignal("cbrSentPktSignal");

CbrSender::CbrSender()
{
    initialized_ = false;
    selfSource_ = NULL;
    selfSender_ = NULL;
}

CbrSender::~CbrSender()
{
    cancelAndDelete(selfSource_);
}

void CbrSender::initialize(int stage)
{

    cSimpleModule::initialize(stage);
    EV << "CBR Sender initialize: stage " << stage << " - initialize=" << initialized_ << endl;

    if (stage == INITSTAGE_LOCAL)
    {
        selfSource_ = new cMessage("selfSource");
        nframes_ = 0;
        nframesTmp_ = 0;
        iDframe_ = 0;
        timestamp_ = 0;
        size_ = par("PacketSize");
        sampling_time = par("sampling_time");
        localPort_ = par("localPort");
        destPort_ = par("destPort");

        txBytes_ = 0;

    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        // calculating traffic starting time
        startTime_ = par("startTime");
        finishTime_ = par("finishTime");

        EV << " finish time " << finishTime_ << endl;
        nframes_ = (finishTime_ - startTime_) / sampling_time;

        initTraffic_ = new cMessage("initTraffic");
        initTraffic();
    }
}

void CbrSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSource"))
        {
            EV << "CbrSender::handleMessage - now[" << simTime() << "] <= finish[" << finishTime_ <<"]" <<endl;
            if( simTime() <= finishTime_ || finishTime_ == 0 )
                sendCbrPacket();
        }
        else
            initTraffic();
    }
}

void CbrSender::initTraffic()
{
    std::string destAddress = par("destAddress").stringValue();
    cModule* destModule = getModuleByPath(par("destAddress").stringValue());
    if (destModule == NULL)
    {
        // this might happen when users are created dynamically
        EV << simTime() << "CbrSender::initTraffic - destination " << destAddress << " not found" << endl;

        simtime_t offset = 0.01; // TODO check value
        scheduleAt(simTime()+offset, initTraffic_);
        EV << simTime() << "CbrSender::initTraffic - the node will retry to initialize traffic in " << offset << " seconds " << endl;
    }
    else
    {
        delete initTraffic_;

        destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());
        socket.setOutputGate(gate("udpOut"));
        socket.bind(localPort_);

        EV << simTime() << "CbrSender::initialize - binding to port: local:" << localPort_ << " , dest: " << destAddress_.str() << ":" << destPort_ << endl;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        // TODO maybe un-necesessary
        // this conversion is made in order to obtain ms-aligned start time, even in case of random generated ones
        simtime_t offset = (round(SIMTIME_DBL(startTime)*1000)/1000);

        scheduleAt(simTime()+startTime, selfSource_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void CbrSender::sendCbrPacket()
{
    EV << "CbrSender::sendCbrPacket - Sending frame[" << iDframe_ << "/" << nframes_ << "], next packet at "<< simTime() + sampling_time << endl;

    emit(cbrSentPktSignal_, (long)iDframe_);

    CbrPacket* packet = new CbrPacket("Cbr");
    packet->setNframes(nframes_);
    packet->setIDframe(iDframe_++);
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);

    emit(cbrGeneratedBytesSignal_,size_);

    if( simTime() > getSimulation()->getWarmupPeriod() )
    {
        txBytes_ += size_;
    }
    socket.sendTo(packet, destAddress_, destPort_);

    scheduleAt(simTime() + sampling_time, selfSource_);
}

void CbrSender::finish()
{
    simtime_t elapsedTime = simTime() - getSimulation()->getWarmupPeriod();
    emit( cbrGeneratedThroughtputSignal_, txBytes_ / elapsedTime );
}
