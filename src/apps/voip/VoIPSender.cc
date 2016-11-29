//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <cmath>
#include "apps/voip/VoIPSender.h"

#define round(x) floor((x) + 0.5)

Define_Module(VoIPSender);

VoIPSender::VoIPSender()
{
    selfSource_ = NULL;
    selfSender_ = NULL;
}

VoIPSender::~VoIPSender()
{
    cancelAndDelete(selfSource_);
    cancelAndDelete(selfSender_);
}

void VoIPSender::initialize(int stage)
{
    EV << "VoIP Sender initialize: stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    durTalk_ = 0;
    durSil_ = 0;
    selfSource_ = new cMessage("selfSource");
    scaleTalk_ = par("scale_talk");
    shapeTalk_ = par("shape_talk");
    scaleSil_ = par("scale_sil");
    shapeSil_ = par("shape_sil");
    isTalk_ = par("is_talk");
    iDtalk_ = 0;
    nframes_ = 0;
    nframesTmp_ = 0;
    iDframe_ = 0;
    timestamp_ = 0;
    size_ = par("PacketSize");
    sampling_time = par("sampling_time");
    selfSender_ = new cMessage("selfSender");
    localPort_ = par("localPort");
    destPort_ = par("destPort");
    silences_ = par("silences");

    initTraffic_ = new cMessage("initTraffic");
    initTraffic();
}

void VoIPSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendVoIPPacket();
        else if (!strcmp(msg->getName(), "selfSource"))
            selectPeriodTime();
        else
            initTraffic();
    }
}

void VoIPSender::initTraffic()
{
    std::string destAddress = par("destAddress").stringValue();
    cModule* destModule = getModuleByPath(par("destAddress").stringValue());
    if (destModule == NULL)
    {
        // this might happen when users are created dynamically
        EV << simTime() << "VoIPSender::initTraffic - destination " << destAddress << " not found" << endl;

        simtime_t offset = 0.01; // TODO check value
        scheduleAt(simTime()+offset, initTraffic_);
        EV << simTime() << "VoIPSender::initTraffic - the node will retry to initialize traffic in " << offset << " seconds " << endl;
    }
    else
    {
        delete initTraffic_;

        destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());
        socket.setOutputGate(gate("udpOut"));
        socket.bind(localPort_);

        EV << simTime() << "VoIPSender::initialize - binding to port: local:" << localPort_ << " , dest: " << destAddress_.str() << ":" << destPort_ << endl;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        // TODO maybe un-necesessary
        // this conversion is made in order to obtain ms-aligned start time, even in case of random generated ones
        simtime_t offset = (round(SIMTIME_DBL(startTime)*1000)/1000);

        scheduleAt(simTime()+startTime, selfSource_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void VoIPSender::talkspurt(simtime_t dur)
{
    iDtalk_++;
    nframes_ = (ceil(dur / sampling_time));

    // a talkspurt must be at least 1 frame long
    if (nframes_ == 0)
        nframes_ = 1;

    EV << "VoIPSender::talkspurt - TALKSPURT[" << iDtalk_-1 << "] - Will be created[" << nframes_ << "] frames\n\n";

    iDframe_ = 0;
    nframesTmp_ = nframes_;
    scheduleAt(simTime(), selfSender_);
}

void VoIPSender::selectPeriodTime()
{
    if (!isTalk_)
    {
        double durSil2;
        if(silences_)
        {
            durSil_ = weibull(scaleSil_, shapeSil_);
            durSil2 = round(SIMTIME_DBL(durSil_)*1000) / 1000;
        }
        else
        {
            durSil_ = durSil2 = 0;
        }

        EV << "VoIPSender::selectPeriodTime - Silence Period: " << "Duration[" << durSil_ << "/" << durSil2 << "] seconds\n";
//        durSil_ = durSil2;
        scheduleAt(simTime() + durSil_, selfSource_);
        isTalk_ = true;
    }
    else
    {
        durTalk_ = weibull(scaleTalk_, shapeTalk_);
        double durTalk2 = round(SIMTIME_DBL(durTalk_)*1000) / 1000;
        EV << "VoIPSender::selectPeriodTime - Talkspurt[" << iDtalk_ << "] - Duration[" << durTalk_ << "/" << durTalk2 << "] seconds\n";
//        durTalk_ = durTalk2;
        talkspurt(durTalk_);
        scheduleAt(simTime() + durTalk_, selfSource_);
        isTalk_ = false;
    }
}

void VoIPSender::sendVoIPPacket()
{
    VoipPacket* packet = new VoipPacket("VoIP");
    packet->setIDtalk(iDtalk_ - 1);
    packet->setNframes(nframes_);
    packet->setIDframe(iDframe_);
    packet->setTimestamp(simTime());
    //packet->setSize(size);
    packet->setByteLength(size_);
    EV << "VoIPSender::sendVoIPPacket - Talkspurt[" << iDtalk_-1 << "] - Sending frame[" << iDframe_ << "]\n";

    socket.sendTo(packet, destAddress_, destPort_);
    --nframesTmp_;
    ++iDframe_;

    if (nframesTmp_ > 0)
        scheduleAt(simTime() + sampling_time, selfSender_);
}
