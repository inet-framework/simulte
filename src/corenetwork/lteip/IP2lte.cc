//
//                           SimuLTE
// Copyright (C) 2013 Antonio Virdis
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "IP2lte.h"
#include "LteBinder.h"
#include "LteDeployer.h"
#include "TCPSegment.h"
#include "UDPPacket.h"
#include "UDP.h"
#include <iostream>

#include "InterfaceTableAccess.h"
#include "IPv4InterfaceData.h"
#include "InterfaceEntry.h"

using namespace std;

Define_Module(IP2lte);

void IP2lte::initialize()
{
    stackGateOut_ = gate("stackLte$o");
    ipGateOut_ = gate("upperLayerOut");

    setNodeType(par("nodeType").stdstringValue());

    seqNum_ = 0;

    if (nodeType_ == UE)
    {
        // TODO not so elegant
        cModule *ue = getParentModule()->getParentModule();
        getBinder()->registerNode(ue, nodeType_, ue->par("masterId"));
    }
    else if (nodeType_ == ENODEB)
    {
        // TODO not so elegant
        cModule *enodeb = getParentModule()->getParentModule();
        MacNodeId cellId = getBinder()->registerNode(enodeb, nodeType_);
        LteDeployer * deployer = check_and_cast<LteDeployer*>(enodeb->getSubmodule("deployer"));
        getBinder()->registerDeployer(deployer, cellId);
    }

    registerInterface();
}

void IP2lte::handleMessage(cMessage *msg)
{
    if( nodeType_ == ENODEB )
    {
        // message from IP Layer: send to stack
        if (msg->getArrivalGate()->isName("upperLayerIn"))
        {
            IPv4Datagram *ipDatagram = check_and_cast<IPv4Datagram *>(msg);
            fromIpEnb(ipDatagram);
        }
        // message from stack: send to peer
        else if(msg->getArrivalGate()->isName("stackLte$i"))
            toIpEnb(msg);
        else
        {
            // error: drop message
            delete msg;
            EV << "IP2lte::handleMessage - (ENODEB): Wrong gate " << msg->getArrivalGate()->getName() << endl;
        }
    }

    else if( nodeType_ == UE )
    {
        // message from transport: send to stack
        if (msg->getArrivalGate()->isName("upperLayerIn"))
        {
            IPv4Datagram *ipDatagram = check_and_cast<IPv4Datagram *>(msg);
            EV << "LteIp: message from transport: send to stack" << endl;
            fromIpUe(ipDatagram);
        }
        else if(msg->getArrivalGate()->isName("stackLte$i"))
        {
            // message from stack: send to transport
            EV << "LteIp: message from stack: send to transport" << endl;
            IPv4Datagram *datagram = check_and_cast<IPv4Datagram *>(msg);
            toIpUe(datagram);
        }
        else
        {
            // error: drop message
            delete msg;
            EV << "LteIp (UE): Wrong gate " << msg->getArrivalGate()->getName() << endl;
        }
    }
}


void IP2lte::setNodeType(std::string s)
{
    nodeType_ = aToNodeType(s);
    EV << "Node type: " << s << " -> " << nodeType_ << endl;
}


void IP2lte::fromIpUe(IPv4Datagram * datagram)
{
    // Remove control info from IP datagram
    delete(datagram->removeControlInfo());

    // obtain the encapsulated transport packet
    cPacket * transportPacket = datagram->getEncapsulatedPacket();

    // 5-Tuple infos
    unsigned short srcPort = 0;
    unsigned short dstPort = 0;
    int transportProtocol = datagram->getTransportProtocol();
    // TODO Add support to IPv6
    IPv4Address srcAddr  = datagram->getSrcAddress() ,
                destAddr = datagram->getDestAddress();

    int headerSize = datagram->getHeaderLength();

    // inspect packet depending on the transport protocol type
    switch (transportProtocol)
    {
        case IP_PROT_TCP:
            TCPSegment* tcpseg;
            tcpseg = check_and_cast<TCPSegment*>(transportPacket);
            srcPort = tcpseg->getSrcPort();
            dstPort = tcpseg->getDestPort();
            headerSize += tcpseg->getHeaderLength();
            break;
        case IP_PROT_UDP:
            UDPPacket* udppacket;
            udppacket = check_and_cast<UDPPacket*>(transportPacket);
            srcPort = udppacket->getSourcePort();
            dstPort = udppacket->getDestinationPort();
            headerSize += UDP_HEADER_BYTES;
            break;
    }

    FlowControlInfo *controlInfo = new FlowControlInfo();
    controlInfo->setSrcAddr(srcAddr.getInt());
    controlInfo->setDstAddr(destAddr.getInt());
    controlInfo->setSrcPort(srcPort);
    controlInfo->setDstPort(dstPort);
    controlInfo->setSequenceNumber(seqNum_++);
    controlInfo->setHeaderSize(headerSize);
    printControlInfo(controlInfo);

    datagram->setControlInfo(controlInfo);

    //** Send datagram to lte stack or LteIp peer **
    send(datagram,stackGateOut_);
}

void IP2lte::toIpUe(IPv4Datagram *datagram)
{
    EV << "IP2lte::toIpUe - message from stack: send to IP layer" << endl;
    send(datagram,ipGateOut_);
}

void IP2lte::fromIpEnb(IPv4Datagram * datagram)
{
    EV << "IP2lte::fromIpEnb - message from IP layer: send to stack" << endl;
    // Remove control info from IP datagram
    delete(datagram->removeControlInfo());

    // obtain the encapsulated transport packet
    cPacket * transportPacket = datagram->getEncapsulatedPacket();

    // 5-Tuple infos
    unsigned short srcPort = 0;
    unsigned short dstPort = 0;
    int transportProtocol = datagram->getTransportProtocol();
    // TODO Add support to IPv6
    IPv4Address srcAddr  = datagram->getSrcAddress() ,
                destAddr = datagram->getDestAddress();

    int headerSize = 0;

    switch(transportProtocol)
    {
        case IP_PROT_TCP:
        TCPSegment* tcpseg;
        tcpseg = check_and_cast<TCPSegment*>(transportPacket);
        srcPort = tcpseg->getSrcPort();
        dstPort = tcpseg->getDestPort();
        headerSize += tcpseg->getHeaderLength();
        break;
        case IP_PROT_UDP:
        UDPPacket* udppacket;
        udppacket = check_and_cast<UDPPacket*>(transportPacket);
        srcPort = udppacket->getSourcePort();
        dstPort = udppacket->getDestinationPort();
        headerSize += UDP_HEADER_BYTES;
        break;
    }

    FlowControlInfo *controlInfo = new FlowControlInfo();
    controlInfo->setSrcAddr(srcAddr.getInt());
    controlInfo->setDstAddr(destAddr.getInt());
    controlInfo->setSrcPort(srcPort);
    controlInfo->setDstPort(dstPort);
    controlInfo->setSequenceNumber(seqNum_++);
    controlInfo->setHeaderSize(headerSize);

    // TODO Relay management should be placed here
    MacNodeId destId = getBinder()->getMacNodeId(IPv4Address(controlInfo->getDstAddr()));
    MacNodeId master = getBinder()->getNextHop(destId);

    controlInfo->setDestId(master);
    printControlInfo(controlInfo);
    datagram->setControlInfo(controlInfo);

    send(datagram,stackGateOut_);
}

void IP2lte::toIpEnb(cMessage * msg)
{
    EV << "IP2lte::toIpEnb - message from stack: send to IP layer" << endl;
    send(msg,ipGateOut_);
}

void IP2lte::printControlInfo(FlowControlInfo* ci)
{
    EV << "Src IP : " << IPv4Address(ci->getSrcAddr()) << endl;
    EV << "Dst IP : " << IPv4Address(ci->getDstAddr()) << endl;
    EV << "Src Port : " << ci->getSrcPort() << endl;
    EV << "Dst Port : " << ci->getDstPort() << endl;
    EV << "Seq Num  : " << ci->getSequenceNumber() << endl;
    EV << "Header Size : " << ci->getHeaderSize() << endl;
}


void IP2lte::registerInterface()
{
    InterfaceEntry * interfaceEntry;
    IInterfaceTable *ift = InterfaceTableAccess().getIfExists();
    if (!ift)
        return;
    interfaceEntry = new InterfaceEntry(this);
    interfaceEntry->setName("wlan");
    // TODO configure MTE size from NED
    interfaceEntry->setMtu(1500);
    ift->addInterface(interfaceEntry);
}

