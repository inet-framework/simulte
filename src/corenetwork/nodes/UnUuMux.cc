//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "corenetwork/nodes/UnUuMux.h"
#include "common/LteCommon.h"

Define_Module(UnUuMux);
using namespace omnetpp;

void UnUuMux::handleUpperMessage(cMessage *msg)
{
    send(msg, down_[OUT_GATE]);
}

void UnUuMux::handleLowerMessage(cMessage *msg)
{
    UserControlInfo *ci = check_and_cast<UserControlInfo *>(msg->getControlInfo());
    MacNodeId src = ci->getSourceId();
    if (getNodeTypeById(src) == ENODEB)
    {
        // message from DeNB --> send to Un interface
        send(msg, upUn_[OUT_GATE]);
    }
    else
    {
        send(msg, upUu_[OUT_GATE]);
    }
}

void UnUuMux::initialize()
{
    upUn_[IN_GATE] = gate("Un$i");
    upUn_[OUT_GATE] = gate("Un$o");
    upUu_[IN_GATE] = gate("Uu$o");
    upUu_[OUT_GATE] = gate("Uu$o");
    down_[IN_GATE] = gate("lowerGate$i");
    down_[OUT_GATE] = gate("lowerGate$o");
}

void UnUuMux::handleMessage(cMessage *msg)
{
    cGate* incoming = msg->getArrivalGate();
    if (incoming == down_[IN_GATE])
    {
        handleLowerMessage(msg);
    }
    else
    {
        handleUpperMessage(msg);
    }
    return;
}
