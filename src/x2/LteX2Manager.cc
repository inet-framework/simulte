//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#define DATAPORT_OUT "dataPort$o"
#define DATAPORT_IN "dataPort$i"

#include "x2/LteX2Manager.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/networklayer/configurator/ipv4/IPv4NetworkConfigurator.h"

Define_Module(LteX2Manager);

LteX2Manager::LteX2Manager() {
}

LteX2Manager::~LteX2Manager() {
}

void LteX2Manager::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        // get the node id
        nodeId_ = getAncestorPar("macCellId");
    }
    else if (stage == inet::INITSTAGE_NETWORK_LAYER_3)
    {
        // find x2ppp interface entries and register their IP addresses to the binder
        // IP addresses will be used in the next init stage to get the X2 id of the peer
        IPv4NetworkConfigurator* configurator = check_and_cast<IPv4NetworkConfigurator*>(getModuleByPath("configurator"));
        IInterfaceTable *interfaceTable =  configurator->findInterfaceTableOf(getParentModule()->getParentModule());
        for (int i=0; i<interfaceTable->getNumInterfaces(); i++)
        {
            // look for x2ppp interfaces in the interface table
            InterfaceEntry * interfaceEntry = interfaceTable->getInterface(i);
            const char* ifName = interfaceEntry->getName();
            if (strstr(ifName,"x2ppp") != NULL)
            {
                IPv4Address addr = interfaceEntry->ipv4Data()->getIPAddress();
                getBinder()->setX2NodeId(interfaceEntry->ipv4Data()->getIPAddress(), nodeId_);
            }
        }
    }
    else if (stage == inet::INITSTAGE_NETWORK_LAYER_3+1)
    {
        // for each X2App, get the client submodule and set connection parameters (connectPort)
        for (int i=0; i<gateSize("x2$i"); i++)
        {
            // client of the X2Apps is connected to the input sides of "x2" gate
            cGate* inGate = gate("x2$i",i);

            // get the X2App client connected to this gate
            //                                                  x2  -> X2App.x2ManagerIn ->  X2App.client
            X2AppClient* client = check_and_cast<X2AppClient*>(inGate->getPathStartGate()->getOwnerModule());

            // get the connectAddress for the X2App client and the corresponding X2 id
            L3Address addr = L3AddressResolver().resolve(client->par("connectAddress").stringValue());
            X2NodeId peerId = getBinder()->getX2NodeId(addr.toIPv4());

            // bind the peerId to the output gate
            x2InterfaceTable_[peerId] = i;
        }
    }
}

void LteX2Manager::handleMessage(cMessage *msg)
{
    cPacket* pkt = check_and_cast<cPacket*>(msg);
    cGate* incoming = pkt->getArrivalGate();

    // the incoming gate is part of a gate vector, so get the base name
    if (strcmp(incoming->getBaseName(), "dataPort") == 0)
    {
        // incoming data from LTE stack
        EV << "LteX2Manager::handleMessage - Received message from LTE stack" << endl;

        fromStack(pkt);
    }
    else  // from X2
    {
        // the incoming gate belongs to a gate vector, so get its index
        int gateIndex = incoming->getIndex();

        // incoming data from X2
        EV << "LteX2Manager::handleMessage - Received message from X2, gate " << gateIndex << endl;

        // call handler
        fromX2(pkt);
    }
}

void LteX2Manager::fromStack(cPacket* pkt)
{
    LteX2Message* x2msg = check_and_cast<LteX2Message*>(pkt);
    X2ControlInfo* x2Info = check_and_cast<X2ControlInfo*>(x2msg->removeControlInfo());

    if (x2Info->getInit())
    {
        // gate initialization
        LteX2MessageType msgType = x2msg->getType();
        int gateIndex = x2msg->getArrivalGate()->getIndex();
        dataInterfaceTable_[msgType] = gateIndex;

        delete x2Info;
        delete x2msg;
        return;
    }

    // If the message is a HandoverDataMsg, send to the GTPUserX2 module
    if (x2msg->getType() == X2_HANDOVER_DATA_MSG)
    {
        // GTPUserX2 module will tunnel this datagram towards the target eNB
        DestinationIdList destList = x2Info->getDestIdList();
        DestinationIdList::iterator it = destList.begin();
        for (; it != destList.end(); ++it)
        {
            X2NodeId targetEnb = *it;
            x2msg->setSourceId(nodeId_);
            x2msg->setDestinationId(targetEnb);

            // send to the gate connected to the GTPUser module
            cGate* outputGate = gate("x2Gtp$o");
            send(x2msg, outputGate);
        }
        delete x2Info;
    }
    else  // X2 control messages
    {
        DestinationIdList destList = x2Info->getDestIdList();
        DestinationIdList::iterator it = destList.begin();
        for (; it != destList.end(); ++it)
        {
            // send a X2 message to each destination eNodeB
            LteX2Message* x2msg_dup = x2msg->dup();
            x2msg_dup->setSourceId(nodeId_);
            x2msg_dup->setDestinationId(*it);

            // select the index for the output gate (it belongs to a vector)
            int gateIndex = x2InterfaceTable_[*it];
            cGate* outputGate = gate("x2$o",gateIndex);
            send(x2msg_dup, outputGate);
        }
        delete x2Info;
        delete x2msg;
    }

}

void LteX2Manager::fromX2(cPacket* pkt)
{
    LteX2Message* x2msg = check_and_cast<LteX2Message*>(pkt);
    LteX2MessageType msgType = x2msg->getType();

    if (msgType == X2_UNKNOWN_MSG)
    {
        EV << " LteX2Manager::fromX2 - Unknown type of the X2 message. Discard." << endl;
        return;
    }

    // get the correct output gate for the message
    int gateIndex = dataInterfaceTable_[msgType];
    cGate* outGate = gate(DATAPORT_OUT, gateIndex);

    // send X2 msg to stack
    EV << "LteX2Manager::fromX2 - send X2MSG to LTE stack" << endl;
    send(PK(x2msg), outGate);
}
