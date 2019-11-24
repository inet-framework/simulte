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
#include <inet/networklayer/common/InterfaceEntry.h>

#include "epc/gtp/GtpUserSimplified.h"
#include "epc/gtp/GtpUserMsg_m.h"
#include "epc/gtp/TftControlInfo.h"
#include "epc/gtp/conversion.h"
Define_Module(GtpUserSimplified);



using namespace omnetpp;
using namespace inet;



void GtpUserSimplified::initialize(int stage) {
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

void GtpUserSimplified::handleFromTrafficFlowFilter(Packet * datagram) {
    TrafficFlowTemplateId flowId = gtp::modification::removeTftControlInfo(datagram);
    EV
              << "GtpUserSimplified::handleFromTrafficFlowFilter - Received a tftMessage with flowId["
              << flowId << "]" << endl;

    // If we are on the eNB and the flowId represents the ID of this eNB, forward the packet locally
    if (flowId == 0) {
        // local delivery
        send(datagram, "pppGate");
    } else {
        // create a new GtpUserSimplifiedMessage
        // and encapsulate the datagram within the GtpUserSimplifiedMessage
        auto gtpPacket = gtp::conversion::packetToGtpUserMsg(0, datagram);

        L3Address tunnelPeerAddress;
        if (flowId == -1) // send to the PGW
                {
            tunnelPeerAddress = pgwAddress_;
        } else {
            // get the symbolic IP address of the tunnel destination ID
            // then obtain the address via IPvXAddressResolver
            const char* symbolicName = binder_->getModuleNameByMacNodeId(
                    flowId);
            tunnelPeerAddress = L3AddressResolver().resolve(symbolicName);
        }
        socket_.sendTo(gtpPacket, tunnelPeerAddress, tunnelPeerPort_);
    }
}

// TODO: method needs to be refactored - redundant code
void GtpUserSimplified::handleFromUdp(Packet * pkt) {
    EV
              << "GtpUserSimplified::handleFromUdp - Decapsulating datagram from GTP tunnel"
              << endl;

    // re-create the original IP datagram and send it to the local network
    auto tuple = gtp::conversion::copyAndPopGtpHeader(pkt);
    auto originalDatagram = std::get<gtp::conversion::ORIGINAL_DATAGRAM>(tuple);
    const auto& hdr = std::get<gtp::conversion::GTP_PACKET>(tuple)->peekAtFront<Ipv4Header>();
    const Ipv4Address& destAddr = hdr->getDestAddress();
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (ownerType_ == PGW) {
        if (destId != 0) {
            // create a new GtpUserSimplifiedMessage
            // encapsulate the datagram within the GtpUserMsg
            auto gtpPacket = gtp::conversion::packetToGtpUserMsg(0, originalDatagram);

            MacNodeId destMaster = binder_->getNextHop(destId);
            const char* symbolicName = binder_->getModuleNameByMacNodeId(
                    destMaster);
            L3Address tunnelPeerAddress = L3AddressResolver().resolve(
                    symbolicName);
            socket_.sendTo(gtpPacket, tunnelPeerAddress, tunnelPeerPort_);
            EV
                      << "GtpUserSimplified::handleFromUdp - Destination is a MEC server. Sending GTP packet to "
                      << symbolicName << endl;
            return;
        } else {
            // destination is outside the LTE network
            EV
                      << "GtpUserSimplified::handleFromUdp - Deliver datagram to the Internet "
                      << endl;
            send(originalDatagram, "pppGate");
            return;
        }

    } else if (ownerType_ == ENB) {

        if (destId != 0) {
            MacNodeId enbId = getAncestorPar("macNodeId");
            MacNodeId destMaster = binder_->getNextHop(destId);
            if (destMaster == enbId) {
                EV
                          << "GtpUserSimplified::handleFromUdp - Deliver datagram to the LTE NIC "
                          << endl;
                // add Interface-Request for LteNic
                if (ie_ != nullptr)
                    originalDatagram->addTagIfAbsent<InterfaceReq>()->setInterfaceId(
                            ie_->getInterfaceId());
                send(originalDatagram, "pppGate");
                return;
            }
        }

        // send the message to the correct eNB or to the Internet, through the PGW
        // * create a new GtpUserSimplifiedMessage
        // * encapsulate the datagram within the GtpUserMsg
        auto gtpMsg = gtp::conversion::packetToGtpUserMsg(0, originalDatagram);
        socket_.sendTo(gtpMsg, pgwAddress_, tunnelPeerPort_);

    } else {
        throw cRuntimeError("Unknown ownerType_- cannot process packet");
    }
}

