//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//
#include <iostream>

#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/common/packet/printer/PacketPrinter.h>

#include "epc/gtp/GtpUserSimplified.h"
#include "epc/gtp/GtpUserMsg_m.h"
#include "epc/gtp/TftControlInfo.h"
Define_Module(GtpUserSimplified);

using namespace omnetpp;
using namespace inet;

void GtpUserSimplified::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    // get reference to the binder
    binder_ = getBinder();

    // transport layer access
    socket_.setOutputGate(gate("socketOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");

    // get the address of the PGW
    pgwAddress_ = L3AddressResolver().resolve("pgw");

    ownerType_ = selectOwnerType(getAncestorPar("nodeType"));

    ie_ = detectInterface();
}

void GtpUserSimplified::handleFromTrafficFlowFilter(Packet * datagram)
{
    TftControlInfo * tftInfo = datagram->removeTag<TftControlInfo>();
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
        // and encapsulate the datagram within the GtpUserSimplifiedMessage
        auto header = makeShared<GtpUserMsg>();
        header->setTeid(0);
        header->setChunkLength(B(8));
        auto gtpPacket = new Packet(datagram->getName());
        gtpPacket->insertAtFront(header);
        auto data = datagram->peekData();
        gtpPacket->insertAtBack(data);

        delete datagram;

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
        socket_.sendTo(gtpPacket, tunnelPeerAddress, tunnelPeerPort_);
    }
}

// TODO: method needs to be refactored - redundant code
void GtpUserSimplified::handleFromUdp(Packet * pkt)
{
    EV << "GtpUserSimplified::handleFromUdp - Decapsulating datagram from GTP tunnel" << endl;

    // re-create the original IP datagram and send it to the local network
    auto originalPacket = new Packet (pkt->getName());
    auto gtpUserMsg = pkt->popAtFront<GtpUserMsg>();
    originalPacket->insertAtBack(pkt->peekData());
    originalPacket->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

    delete pkt;
        const auto& hdr = originalPacket->peekAtFront<Ipv4Header>();
        const Ipv4Address& destAddr = hdr->getDestAddress();
        MacNodeId destId = binder_->getMacNodeId(destAddr);

    if (ownerType_ == PGW)
    {
        if (destId != 0)
        {
             // create a new GtpUserSimplifiedMessage
             // encapsulate the datagram within the GtpUserMsg
             auto header = makeShared<GtpUserMsg>();
             header->setTeid(0);
             header->setChunkLength(B(8));
             auto gtpPacket = new Packet(originalPacket->getName());
             gtpPacket->insertAtFront(header);
             auto data = originalPacket->peekData();
             gtpPacket->insertAtBack(data);
             delete originalPacket;

             MacNodeId destMaster = binder_->getNextHop(destId);
             const char* symbolicName = binder_->getModuleNameByMacNodeId(destMaster);
             L3Address tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
             socket_.sendTo(gtpPacket, tunnelPeerAddress, tunnelPeerPort_);
             EV << "GtpUserSimplified::handleFromUdp - Destination is a MEC server. Sending GTP packet to " << symbolicName << endl;
            return;
        } else
        {
            // destination is outside the LTE network
            EV << "GtpUserSimplified::handleFromUdp - Deliver datagram to the Internet " << endl;
            send(originalPacket,"pppGate");
            return;
        }

    } else if (ownerType_ == ENB)
    {

        if (destId != 0)
        {
            MacNodeId enbId = getAncestorPar("macNodeId");
            MacNodeId destMaster = binder_->getNextHop(destId);
            if (destMaster == enbId)
            {
                EV << "GtpUserSimplified::handleFromUdp - Deliver datagram to the LTE NIC " << endl;
                // add Interface-Request for LteNic
                if (ie_ != nullptr)
                    originalPacket->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie_->getInterfaceId());
                send(originalPacket,"pppGate");
                return;
            }
        }

        // send the message to the correct eNB or to the Internet, through the PGW
        // * create a new GtpUserSimplifiedMessage
        // * encapsulate the datagram within the GtpUserMsg
        auto gtpMsg = convertToGtpUserMsg(0, originalPacket);
        socket_.sendTo(gtpMsg, pgwAddress_, tunnelPeerPort_);

    } else {
        throw cRuntimeError("Unknown ownerType_- cannot process packet");
    }
}
