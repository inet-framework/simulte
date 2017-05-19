//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "epc/gtp/GtpUserSimplified.h"
#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include <iostream>

Define_Module(GtpUserSimplified);

void GtpUserSimplified::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    // get reference to the binder
    binder_ = getBinder();

    socket_.setOutputGate(gate("udpOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");

    // get the address of the PGW
    pgwAddress_ = L3AddressResolver().resolve("pgw");

    ownerType_ = selectOwnerType(getAncestorPar("nodeType"));
}

EpcNodeType GtpUserSimplified::selectOwnerType(const char * type)
{
    EV << "GtpUserSimplified::selectOwnerType - setting owner type to " << type << endl;
    if(strcmp(type,"ENODEB") == 0)
        return ENB;
    else if(strcmp(type,"PGW") == 0)
        return PGW;

    error("GtpUserSimplified::selectOwnerType - unknown owner type [%s]. Aborting...",type);
}

void GtpUserSimplified::handleMessage(cMessage *msg)
{
    if (strcmp(msg->getArrivalGate()->getFullName(), "trafficFlowFilterGate") == 0)
    {
        EV << "GtpUserSimplified::handleMessage - message from trafficFlowFilter" << endl;
        // obtain the encapsulated IPv4 datagram
        IPv4Datagram * datagram = check_and_cast<IPv4Datagram*>(msg);
        handleFromTrafficFlowFilter(datagram);
    }
    else if(strcmp(msg->getArrivalGate()->getFullName(),"udpIn")==0)
    {
        EV << "GtpUserSimplified::handleMessage - message from udp layer" << endl;

        GtpUserMsg * gtpMsg = check_and_cast<GtpUserMsg *>(msg);
        handleFromUdp(gtpMsg);
    }
}

void GtpUserSimplified::handleFromTrafficFlowFilter(IPv4Datagram * datagram)
{
    // extract control info from the datagram
    TftControlInfo * tftInfo = check_and_cast<TftControlInfo *>(datagram->removeControlInfo());
    TrafficFlowTemplateId flowId = tftInfo->getTft();
    delete (tftInfo);

    EV << "GtpUserSimplified::handleFromTrafficFlowFilter - Received a tftMessage with flowId[" << flowId << "]" << endl;

    // If we are on the eNB and the flowId represents the ID of this eNB, forward the packet locally
    if (flowId == 0)
    {
        // local delivery
        send(datagram,"pppGate");
    }
    else
    {
        // create a new GtpUserSimplifiedMessage
        GtpUserMsg * gtpMsg = new GtpUserMsg();
        gtpMsg->setName("GtpUserMessage");

        // encapsulate the datagram within the GtpUserSimplifiedMessage
        gtpMsg->encapsulate(datagram);

        L3Address tunnelPeerAddress;
        if (flowId == -1) // send to the PGW
        {
            tunnelPeerAddress = pgwAddress_;
        }
        else
        {
            // get the symbolic IP address of the tunnel destination ID
            // then obtain the address via IPvXAddressResolver
            const char* symbolicName = binder_->getModuleNameByMacNodeId(flowId);
            tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
        }
        socket_.sendTo(gtpMsg, tunnelPeerAddress, tunnelPeerPort_);
    }
}

void GtpUserSimplified::handleFromUdp(GtpUserMsg * gtpMsg)
{
    EV << "GtpUserSimplified::handleFromUdp - Decapsulating and sending to local connection." << endl;

    // obtain the original IP datagram and send it to the local network
    IPv4Datagram * datagram = check_and_cast<IPv4Datagram*>(gtpMsg->decapsulate());
    delete(gtpMsg);

    if (ownerType_ == PGW)
    {
        IPv4Address& destAddr = datagram->getDestAddress();
        MacNodeId destId = binder_->getMacNodeId(destAddr);
        if (destId != 0)
        {
             // create a new GtpUserSimplifiedMessage
             GtpUserMsg * gtpMsg = new GtpUserMsg();
             gtpMsg->setName("GtpUserMessage");

             // encapsulate the datagram within the GtpUserSimplifiedMessage
             gtpMsg->encapsulate(datagram);

             MacNodeId destMaster = binder_->getNextHop(destId);
             const char* symbolicName = binder_->getModuleNameByMacNodeId(destMaster);
             L3Address tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
             socket_.sendTo(gtpMsg, tunnelPeerAddress, tunnelPeerPort_);
             return;
        }
    }

    // destination is outside the LTE network
    send(datagram,"pppGate");
}
