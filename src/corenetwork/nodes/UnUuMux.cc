//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "corenetwork/nodes/UnUuMux.h"

Define_Module(UnUuMux);

void UnUuMux::handleUpperMessage(cMessage *msg)
{
    send(msg, down_[OUT]);
}

void UnUuMux::handleLowerMessage(cMessage *msg)
{
    UserControlInfo *ci = check_and_cast<UserControlInfo *>(msg->getControlInfo());
    MacNodeId src = ci->getSourceId();
    if (getNodeTypeById(src) == ENODEB)
    {
        // message from DeNB --> send to Un interface
        send(msg, upUn_[OUT]);
    }
    else
    {
        send(msg, upUu_[OUT]);
    }
}

void UnUuMux::initialize()
{
    upUn_[IN] = gate("Un$i");
    upUn_[OUT] = gate("Un$o");
    upUu_[IN] = gate("Uu$o");
    upUu_[OUT] = gate("Uu$o");
    down_[IN] = gate("lowerGate$i");
    down_[OUT] = gate("lowerGate$o");
}

void UnUuMux::handleMessage(cMessage *msg)
{
    cGate* incoming = msg->getArrivalGate();
    if (incoming == down_[IN])
    {
        handleLowerMessage(msg);
    }
    else
    {
        handleUpperMessage(msg);
    }
    return;
}
