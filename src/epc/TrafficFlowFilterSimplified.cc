//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "epc/TrafficFlowFilterSimplified.h"
#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>

Define_Module(TrafficFlowFilterSimplified);

using namespace inet;
using namespace omnetpp;

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

    // register service processing IP-packets on the LTE Uu Link
    registerService(LteProtocol::ipv4uu, gate("internetFilterGateIn"), gate("internetFilterGateIn"));
}

EpcNodeType TrafficFlowFilterSimplified::selectOwnerType(const char * type)
{
    EV << "TrafficFlowFilterSimplified::selectOwnerType - setting owner type to " << type << endl;
    if(strcmp(type,"ENODEB") == 0)
        return ENB;
    if(strcmp(type,"PGW") != 0)
        error("TrafficFlowFilterSimplified::selectOwnerType - unknown owner type [%s]. Aborting...",type);
    return PGW;
}

void TrafficFlowFilterSimplified::handleMessage(cMessage *msg)
{
    EV << "TrafficFlowFilterSimplified::handleMessage - Received Packet:" << endl;
    EV << "name: " << msg->getFullName() << endl;

    Packet* pkt = check_and_cast<Packet *>(msg);

    // receive and read IP datagram
    // TODO: needs to be adapted for IPv6
    const auto& ipv4Header = pkt->peekAtFront<Ipv4Header>();
    const Ipv4Address &destAddr = ipv4Header->getDestAddress();
    const Ipv4Address &srcAddr = ipv4Header->getSrcAddress();
    pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

    // TODO check for source and dest port number

    EV << "TrafficFlowFilterSimplified::handleMessage - Received datagram : " << pkt->getName() << " - src[" << srcAddr << "] - dest[" << destAddr << "]\n";

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
        auto tftInfo = pkt->addTag<TftControlInfo>();
        tftInfo->setTft(tftId);

        EV << "TrafficFlowFilterSimplified::handleMessage - setting tft=" << tftId << endl;

        // send the datagram to the GTP-U module
        send(pkt,"gtpUserGateOut");
    }
}

TrafficFlowTemplateId TrafficFlowFilterSimplified::findTrafficFlow(L3Address srcAddress, L3Address destAddress)
{
    MacNodeId destId = binder_->getMacNodeId(destAddress.toIpv4());
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
        MacNodeId srcMaster = binder_->getNextHop(binder_->getMacNodeId(srcAddress.toIpv4()));
        if (fastForwarding_ && srcMaster == destMaster)
            return 0;                 // local delivery
        return -1;   // send the packet to the PGW
    }

    return destMaster;
}

