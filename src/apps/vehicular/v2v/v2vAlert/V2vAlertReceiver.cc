//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <cmath>
#include "V2vAlertReceiver.h"
#include "inet/common/ModuleAccess.h"  // for multicast support

Define_Module(V2vAlertReceiver);

void V2vAlertReceiver::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    int port = par("localPort");
    EV << "V2vAlertReceiver::initialize - binding to port: local:" << port << endl;
    if (port != -1)
    {
        socket.setOutputGate(gate("udpOut"));
        socket.bind(port);

        // for multicast support
        /*
        inet::IInterfaceTable *ift = inet::getModuleFromPar<inet::IInterfaceTable>(par("interfaceTableModule"), this);
        inet::MulticastGroupList mgl = ift->collectMulticastGroups();
        socket.joinLocalMulticastGroups(mgl);
        inet::InterfaceEntry *ie = ift->getInterfaceByName("wlan");
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named wlan");
        socket.setMulticastOutputInterface(ie->getInterfaceId());
        */
    }

    v2vAlertDelay_ = registerSignal("v2vAlertDelay");
    v2vAlertRcvdMsg_ = registerSignal("v2vAlertRcvdMsg");
}

void V2vAlertReceiver::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;

    V2vAlertPacket* pkt = check_and_cast<V2vAlertPacket*>(msg);

    if (pkt == 0)
        throw cRuntimeError("V2vAlertReceiver::handleMessage - FATAL! Error when casting to V2vAlertPacket");

    // emit statistics
    simtime_t delay = simTime() - pkt->getTimestamp();
    emit(v2vAlertDelay_, delay);
    emit(v2vAlertRcvdMsg_, (long)1);

    EV << "V2vAlertReceiver::handleMessage - Packet received: SeqNo[" << pkt->getSno() << "] Delay[" << delay << "]" << endl;

    delete msg;
}
