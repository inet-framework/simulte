//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/alert/AlertReceiver.h"
#include <inet/common/ModuleAccess.h>  // for multicast support

Define_Module(AlertReceiver);
using namespace inet;

void AlertReceiver::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    int port = par("localPort");
    EV << "AlertReceiver::initialize - binding to port: local:" << port << endl;
    if (port != -1)
    {
        socket.setOutputGate(gate("socketOut"));
        socket.bind(port);

        // for multicast support
        inet::IInterfaceTable *ift = inet::getModuleFromPar<inet::IInterfaceTable>(par("interfaceTableModule"), this);
        inet::MulticastGroupList mgl = ift->collectMulticastGroups();
        socket.joinLocalMulticastGroups(mgl);
        inet::InterfaceEntry *ie = ift->getInterfaceByName("wlan");
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named wlan");
        socket.setMulticastOutputInterface(ie->getInterfaceId());
        // -------------------- //
    }

    alertDelay_ = registerSignal("alertDelay");
    alertRcvdMsg_ = registerSignal("alertRcvdMsg");
}

void AlertReceiver::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;

    Packet* pPacket = check_and_cast<Packet*>(msg);

    // read Alert header
    auto alert = pPacket->popAtFront<AlertPacket>();

    // emit statistics
    simtime_t delay = simTime() - alert->getTimestamp();
    emit(alertDelay_, delay);
    emit(alertRcvdMsg_, (long)1);

    EV << "AlertReceiver::handleMessage - Packet received: SeqNo[" << alert->getSno() << "] Delay[" << delay << "]" << endl;

    delete msg;
}

