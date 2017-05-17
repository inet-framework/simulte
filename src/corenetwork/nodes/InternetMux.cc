//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "corenetwork/nodes/InternetMux.h"
#include "corenetwork/binder/LteBinder.h"
#include "common/LteControlInfo_m.h"

Define_Module(InternetMux);

InternetMux::InternetMux()
{
    // TODO Auto-generated constructor stub
}

InternetMux::~InternetMux()
{
    // TODO Auto-generated destructor stub
}

void
InternetMux::initialize()
{
}

void
InternetMux::handleMessage(cMessage *msg)
{
    cGate* incoming = msg->getArrivalGate();
    if (incoming == gate("inMux"))
    {
        // message coming from LteIP;
        // finding next hop for destination node
        FlowControlInfo* controlInfo = check_and_cast<FlowControlInfo*>(msg->getControlInfo());

        MacNodeId dest = getBinder()->getMacNodeId(IPv4Address(controlInfo->getDstAddr()));

        MacNodeId nextHop = getBinder()->getNextHop(dest);

        send(msg, routingTable_.at(nextHop)); // send either to nodeB or relay node.
    }
    else
    {
        // message coming from any connected nodeB
        send(msg, gate("outMux")); // send to LteIP
    }
    return;
}
