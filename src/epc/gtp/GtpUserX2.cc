//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <iostream>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include "epc/gtp/GtpUserX2.h"

Define_Module(GtpUserX2);

using namespace inet;
using namespace omnetpp;

void GtpUserX2::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    // get reference to the binder
    binder_ = getBinder();

    socket_.setOutputGate(gate("socketOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");
}

void GtpUserX2::handleMessage(cMessage *msg)
{
    Packet* pkt = check_and_cast<Packet*>(msg);

    if (strcmp(msg->getArrivalGate()->getFullName(), "lteStackIn") == 0)
    {
        EV << "GtpUserX2::handleMessage - message from X2 Manager" << endl;

        handleFromStack(pkt);
    }
    else if(strcmp(msg->getArrivalGate()->getFullName(),"socketIn")==0)
    {
        EV << "GtpUserX2::handleMessage - message from udp layer" << endl;

        handleFromUdp(pkt);
    }
}

void GtpUserX2::handleFromStack(Packet * pkt)
{
    auto x2Msg = pkt->peekAtFront<LteX2Message>();
    // extract destination from the message
    X2NodeId destId = x2Msg->getDestinationId();
    X2NodeId srcId = x2Msg->getSourceId();
    EV << "GtpUserX2::handleFromStack - Received a LteX2Message with destId[" << destId << "]" << endl;


    // create a new GtpUserMessage
    auto header = makeShared<GtpUserMsg>();
    header->setTeid(0);
    header->setChunkLength(B(8));
    auto gtpPacket = new Packet(pkt->getName());
    gtpPacket->insertAtFront(header);
    auto data = pkt->peekData();
    gtpPacket->insertAtBack(data);

    delete pkt;

    L3Address peerAddress = binder_->getX2PeerAddress(srcId, destId); //L3AddressResolver().resolve(symbolicName);

    socket_.sendTo(gtpPacket, peerAddress, tunnelPeerPort_);
}

void GtpUserX2::handleFromUdp(Packet * pkt)
{
    EV << "GtpUserX2::handleFromUdp - Decapsulating and sending to local connection." << endl;

    // re-create the original IP datagram and send it to the local network
    auto originalPacket = new Packet (pkt->getName());
    auto gtpUserMsg = pkt->popAtFront<GtpUserMsg>();
    originalPacket->insertAtBack(pkt->peekData());
    originalPacket->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);

    delete pkt;

    // send message to the X2 Manager
    send(originalPacket,"lteStackOut");
}
