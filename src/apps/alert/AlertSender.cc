//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <cmath>
#include "apps/alert/AlertSender.h"
#include "inet/common/ModuleAccess.h"  // for multicast support

#define round(x) floor((x) + 0.5)

Define_Module(AlertSender);

AlertSender::AlertSender()
{
    selfSender_ = NULL;
    nextSno_ = 0;
}

AlertSender::~AlertSender()
{
    cancelAndDelete(selfSender_);
}

void AlertSender::initialize(int stage)
{
    EV << "AlertSender::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    selfSender_ = new cMessage("selfSender");

    size_ = par("packetSize");
    period_ = par("period");
    localPort_ = par("localPort");
    destPort_ = par("destPort");
    destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());

    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort_);

    // for multicast support
    inet::IInterfaceTable *ift = inet::getModuleFromPar< inet::IInterfaceTable >(par("interfaceTableModule"), this);
    inet::MulticastGroupList mgl = ift->collectMulticastGroups();
    socket.joinLocalMulticastGroups(mgl);
    inet::InterfaceEntry *ie = ift->getInterfaceByName("wlan");
    if (!ie)
        throw cRuntimeError("Wrong multicastInterface setting: no interface named wlan");
    socket.setMulticastOutputInterface(ie->getInterfaceId());
    // -------------------- //

    alertSentMsg_ = registerSignal("alertSentMsg");

    EV << "AlertSender::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;

    // calculating traffic starting time
    simtime_t startTime = par("startTime");

    // TODO maybe un-necesessary
    // this conversion is made in order to obtain ms-aligned start time, even in case of random generated ones
    simtime_t offset = (round(SIMTIME_DBL(startTime)*1000)/1000)+simTime();

    scheduleAt(offset,selfSender_);
    EV << "\t starting traffic in " << offset << " seconds " << endl;
}

void AlertSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendAlertPacket();
        else
            throw cRuntimeError("AlertSender::handleMessage - Unrecognized self message");
    }
}

void AlertSender::sendAlertPacket()
{
    AlertPacket* packet = new AlertPacket("Alert");
    packet->setSno(nextSno_);
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);
    EV << "AlertSender::sendAlertPacket - Sending message [" << nextSno_ << "]\n";

    socket.sendTo(packet, destAddress_, destPort_);
    nextSno_++;

    emit(alertSentMsg_, (long)1);

    scheduleAt(simTime() + period_, selfSender_);
}
