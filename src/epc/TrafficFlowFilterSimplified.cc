//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "epc/TrafficFlowFilterSimplified.h"
#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/networklayer/common/L3AddressResolver.h"

Define_Module(TrafficFlowFilterSimplified);

void TrafficFlowFilterSimplified::initialize(int stage)
{
    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_NETWORK_LAYER)
        return;

    // get reference to the binder
    binder_ = getBinder();

    fastForwarding_ = par("fastForwarding");

    // reading and setting owner type
    ownerType_ = selectOwnerType(par("ownerType"));
}

EpcNodeType TrafficFlowFilterSimplified::selectOwnerType(const char * type)
{
    EV << "TrafficFlowFilterSimplified::selectOwnerType - setting owner type to " << type << endl;
    if(strcmp(type,"ENODEB") == 0)
        return ENB;
    else if(strcmp(type,"PGW") == 0)
        return PGW;

    error("TrafficFlowFilterSimplified::selectOwnerType - unknown owner type [%s]. Aborting...",type);
}

void TrafficFlowFilterSimplified::handleMessage(cMessage *msg)
{
    EV << "TrafficFlowFilterSimplified::handleMessage - Received Packet:" << endl;
    EV << "name: " << msg->getFullName() << endl;

    // receive and read IP datagram
    IPv4Datagram * datagram = check_and_cast<IPv4Datagram *>(msg);
    IPv4Address &destAddr = datagram->getDestAddress();
    IPv4Address &srcAddr = datagram->getSrcAddress();

    // TODO check for source and dest port number

    EV << "TrafficFlowFilterSimplified::handleMessage - Received datagram : " << datagram->getName() << " - src[" << srcAddr << "] - dest[" << destAddr << "]\n";

    // run packet filter and associate a flowId to the connection (default bearer?)
    // search within tftTable the proper entry for this destination
    TrafficFlowTemplateId tftId = findTrafficFlow(srcAddr, destAddr);   // search for the tftId in the binder
    if(tftId == -2)
    {
        // the destination has been removed from the simulation. Delete msg
        EV << "TrafficFlowFilterSimplified::handleMessage - Destination has been removed from the simulation. Delete packet." << endl;
        delete msg;
    }
    else
    {

        // add control info to the normal ip datagram. This info will be read by the GTP-U application
        TftControlInfo * tftInfo = new TftControlInfo();
        tftInfo->setTft(tftId);
        datagram->setControlInfo(tftInfo);

        EV << "TrafficFlowFilterSimplified::handleMessage - setting tft=" << tftId << endl;

        // send the datagram to the GTP-U module
        send(datagram,"gtpUserGateOut");
    }
}

TrafficFlowTemplateId TrafficFlowFilterSimplified::findTrafficFlow(L3Address srcAddress, L3Address destAddress)
{
    MacNodeId destId = binder_->getMacNodeId(destAddress.toIPv4());
    if (destId == 0)
    {
        if (ownerType_ == ENB)
            return -1;   // the destination is outside the LTE network, so send the packet to the PGW
        else // PGW
            return -2;   // the destination UE has been removed from the simulation
    }

    MacNodeId destMaster = binder_->getNextHop(destId);

    if (ownerType_ == ENB)
    {
        MacNodeId srcMaster = binder_->getNextHop(binder_->getMacNodeId(srcAddress.toIPv4()));
        if (fastForwarding_ && srcMaster == destMaster)
            return 0;                 // local delivery
        return -1;   // send the packet to the PGW
    }

    return destMaster;
}

