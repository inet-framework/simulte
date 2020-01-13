//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/transportlayer/udp/UdpHeader_m.h>

#include "corenetwork/lteip/LteIp.h"
#include "corenetwork/lteip/Constants.h"

#include "../lteCellInfo/LteCellInfo.h"
#include "corenetwork/binder/LteBinder.h"
namespace inet {
    constexpr int IP_HEADER_BYTES = 20;
}
#include "inet/common/ModuleAccess.h"

Define_Module(LteIp);
using namespace std;
using namespace omnetpp;
using namespace inet;

/**
 * @author: wolfgang kallies, wischhof
 * TODO:this entire class had to be rewritten/adapted to the new inet - still needs to be debugged/tested
 */
void LteIp::initialize(int stage) {
    OperationalBase::initialize(stage);

    stackGateOut_ = gate("stackLte$o");
    peerGateOut_ = gate("ifOut");

    defaultTimeToLive_ = par("timeToLive");
    //mapping_.parseProtocolMapping(par("protocolMapping"));
    setNodeType(par("nodeType").stdstringValue());

    numDropped_ = numForwarded_ = seqNum_ = 0;
    if (nodeType_ == UE) {
        cModule *ue = getParentModule();
        getBinder()->registerNode(ue, nodeType_, ue->par("masterId"));
    } else if (nodeType_ == ENODEB) {
        cModule *enodeb = getParentModule();
        getBinder()->registerNode(enodeb, nodeType_);
    }

    WATCH(numForwarded_);
    WATCH(numDropped_);
    WATCH(seqNum_);
}

void LteIp::updateDisplayString() {
    char buf[80] = "";
    if (numForwarded_ > 0)
        sprintf(buf + strlen(buf), "fwd:%d ", numForwarded_);
    if (numDropped_ > 0)
        sprintf(buf + strlen(buf), "drop:%d ", numDropped_);
    getDisplayString().setTagArg("t", 0, buf);
}

void LteIp::endService(cPacket *msg) {
    if (nodeType_ == INTERNET) {
        if (msg->getArrivalGate()->isName("transportIn")) {
            // message from transport: send to peer
            numForwarded_++;
            EV << "LteIp: message from transport: send to peer" << endl;
            fromTransport(msg, peerGateOut_);
        } else if (msg->getArrivalGate()->isName("ifIn")) {
            // message from peer: send to transport
            numForwarded_++;
            EV << "LteIp: message from peer: send to transport" << endl;
            toTransport(msg);
        } else {
            // error: drop message
            numDropped_++;
            delete msg;
            EV << "LteIp (INTERNET): Wrong gate "
                      << msg->getArrivalGate()->getName() << endl;
        }
    } else if (nodeType_ == ENODEB) {
        if (msg->getArrivalGate()->isName("ifIn")) {
            // message from peer: send to stack
            numForwarded_++;
            EV << "LteIp: message from peer: send to stack" << endl;

            inet::Packet *ipDatagram = check_and_cast<inet::Packet *>(msg);
            cPacket *transportPacket = ipDatagram->getEncapsulatedPacket();
            auto header = ipDatagram->peekAtFront<Ipv4Header>();
            auto protocolId = header->getProtocolId();

            // TODO: KLUDGE: copied over from function fromTransport
            unsigned short srcPort = 0;
            unsigned short dstPort = 0;
            B headerSize = B(0); //IP_HEADER_BYTES;

            if (protocolId == IP_PROT_TCP) {
                inet::tcp::TcpHeader* tcpseg;
                tcpseg = check_and_cast<inet::tcp::TcpHeader*>(transportPacket);
                srcPort = tcpseg->getSrcPort();
                dstPort = tcpseg->getDestPort();
                headerSize += tcpseg->getHeaderLength();
            } else if (protocolId == IP_PROT_UDP) {
                auto udppacket = check_and_cast<inet::Packet*>(transportPacket);
                auto header = udppacket->peekAtFront<UdpHeader>();
                srcPort = (unsigned short) header->getSourcePort();
                dstPort = (unsigned short) header->getDestinationPort();
                headerSize += B(UDP_HEADER_BYTES);
            } else {
                throw std::runtime_error(
                        "LteIP::endService() case not implemented");
            }

            FlowControlInfo *controlInfo = new FlowControlInfo();
            controlInfo->setSrcAddr(header->getSrcAddress().getInt());
            controlInfo->setDstAddr(header->getDestAddress().getInt());
            controlInfo->setSrcPort(srcPort);
            controlInfo->setDstPort(dstPort);
            controlInfo->setSequenceNumber(seqNum_++);
            controlInfo->setHeaderSize(headerSize.get());
            MacNodeId destId = getBinder()->getMacNodeId(
                    Ipv4Address(controlInfo->getDstAddr()));
            // master of this ue (myself or a relay)
            // TODO: KLUDGE:
            MacNodeId master = getBinder()->getNextHop(destId);
//            if (master != nodeId_) { // ue is relayed, dest must be the relay
            destId = master;
//            } // else ue is directly attached
            controlInfo->setDestId(destId);
            printControlInfo(controlInfo);
            msg->setControlInfo(controlInfo);

            send(msg, stackGateOut_);
        } else if (msg->getArrivalGate()->isName("stackLte$i")) {
            // message from stack: send to peer
            numForwarded_++;
            EV << "LteIp: message from stack: send to peer" << endl;
            send(msg, peerGateOut_);
        } else {
            // error: drop message
            numDropped_++;
            delete msg;
            EV << "LteIp (ENODEB): Wrong gate "
                      << msg->getArrivalGate()->getName() << endl;
        }
    } else if (nodeType_ == UE) {
        if (msg->getArrivalGate()->isName("transportIn")) {
            // message from transport: send to stack
            numForwarded_++;
            std::cout << "LteIp: message from transport: send to stack" << endl;
            fromTransport(msg, stackGateOut_);
        } else if (msg->getArrivalGate()->isName("stackLte$i")) {
            // message from stack: send to transport
            numForwarded_++;
            std::cout << "LteIp: message from stack: send to transport" << endl;
            toTransport(msg);
        } else {
            // error: drop message
            numDropped_++;
            delete msg;
            EV << "LteIp (UE): Wrong gate " << msg->getArrivalGate()->getName()
                      << endl;
        }
    }

    if (getEnvir()->isGUI())
        updateDisplayString();
}

void LteIp::fromTransport(cPacket * transportPacket, cGate *outputgate) {

    auto packetTmp = check_and_cast<inet::Packet*>(transportPacket);
    auto ipControlInfo = make_shared<Ipv4Header>(
            *(packetTmp->popAtFront<Ipv4Header>()));

    //** Create IP datagram and fill its fields **


//    TODO: needs proper testing
    inet::Packet *datagram = new inet::Packet(transportPacket->getName(), 0);
    datagram->setByteLength(IP_HEADER_BYTES);
    datagram->encapsulate(transportPacket);

    // set destination address
    Ipv4Address dest = ipControlInfo->getDestAddress();
    auto header = makeShared<Ipv4Header>(*datagram->peekAtFront<Ipv4Header>());
    header->setDestAddress(dest);

//    datagram->setDestAddress(dest);

    // set source address
    Ipv4Address src = ipControlInfo->getSrcAddress();

    // when source address was given, use it; otherwise use local Addr
    if (src.isUnspecified()) {
        // find interface entry and use its address
        IInterfaceTable *interfaceTable = getModuleFromPar<IInterfaceTable>(
                par("interfaceTableModule"), this);
        // TODO: how do we find the LTE interface?
        src = interfaceTable->findInterfaceByName("wlan")->getIpv4Address();
        EV << "Local address used: " << src << endl;
    } else {
        EV << "Source address in control info from transport layer " << src
                  << endl;
    }
    header->setSrcAddress(src);

    // set other fields
    header->setTypeOfService(ipControlInfo->getTypeOfService());
    // TODO: header immutable? may be dropped
    datagram->popAtFront<Ipv4Header>();
    datagram->trimFront();
    datagram->insertAtFront(header);
    //ipControlInfo->setTransportProtocol(ipControlInfo->getProtocol());

    // fragmentation fields are correctly filled, but not used
    ipControlInfo->setIdentification(curFragmentId_++);
    ipControlInfo->setMoreFragments(false);
    ipControlInfo->setDontFragment(ipControlInfo->getDontFragment());
    ipControlInfo->setFragmentOffset(0);

    // ttl
    short ci_ttl = ipControlInfo->getTimeToLive();
    ipControlInfo->setTimeToLive((ci_ttl > 0) ? ci_ttl : defaultTimeToLive_);

    //** Add control info for stack **
    unsigned short srcPort = 0;
    unsigned short dstPort = 0;
    B headerSize = B(IP_HEADER_BYTES);

    auto protocolId = ipControlInfo->getProtocolId();
    if (protocolId == IP_PROT_TCP) {
        auto tcppacket = check_and_cast<inet::Packet*>(transportPacket);
        auto header = tcppacket->peekAtFront<inet::tcp::TcpHeader>();
        srcPort = header->getSrcPort();
        dstPort = header->getDestPort();
        headerSize += header->getHeaderLength();
    } else if (protocolId == IP_PROT_UDP) {
        auto udppacket = check_and_cast<inet::Packet*>(transportPacket);
        auto header = udppacket->peekAtFront<UdpHeader>();
        srcPort = (unsigned short) header->getSourcePort();
        dstPort = (unsigned short) header->getDestinationPort();
        headerSize += B(UDP_HEADER_BYTES);
    } else {
        std::string errormessage = "";
        errormessage += "LteIp::fromTransport() error: ";
        errormessage += "got unhandled Protocol\n";
        errormessage += "protocol id: "
                + std::to_string(ipControlInfo->getProtocolId()) + "\n";
        throw std::runtime_error(errormessage);
    }

    // TODO: a second Ipv4 header inserted at the front?
    datagram->insertAtFront(Ptr<Ipv4Header>(ipControlInfo.get()));

    FlowControlInfo *controlInfo = new FlowControlInfo();
    controlInfo->setSrcAddr(src.getInt());
    controlInfo->setDstAddr(dest.getInt());
    controlInfo->setSrcPort(srcPort);
    controlInfo->setDstPort(dstPort);
    controlInfo->setSequenceNumber(seqNum_++);
    controlInfo->setHeaderSize(headerSize.get());
    printControlInfo(controlInfo);

    datagram->setControlInfo(controlInfo);

    //** Send datagram to lte stack or LteIp peer **
    send(datagram, outputgate);
}

void LteIp::toTransport(cPacket * msg) {
    // msg is an IP Datagram (from Lte stack or IP peer)
    inet::Packet *datagram = check_and_cast<inet::Packet *>(msg);

    send(datagram, "transportOut");
}

void LteIp::setNodeType(std::string s) {
    nodeType_ = aToNodeType(s);
    EV << "Node type: " << s << " -> " << nodeType_ << endl;
}

void LteIp::printControlInfo(FlowControlInfo* ci) {
    EV << "Src IP : " << Ipv4Address(ci->getSrcAddr()) << endl;
    EV << "Dst IP : " << Ipv4Address(ci->getDstAddr()) << endl;
    EV << "Src Port : " << ci->getSrcPort() << endl;
    EV << "Dst Port : " << ci->getDstPort() << endl;
    EV << "Seq Num  : " << ci->getSequenceNumber() << endl;
    EV << "Header Size : " << ci->getHeaderSize() << endl;
}

void LteIp::handleMessageWhenUp(cMessage *msg) {
    throw cRuntimeError(" handleMessageWhenUp method not implemented");

    /*  note: this is the implementation in the Ipv4-Layer
     * TODO: check if it can be adapted/applied hiere
     *
     if (msg->arrivedOn("transportIn")) {    //TODO packet->getArrivalGate()->getBaseId() == transportInGateBaseId
     if (auto request = dynamic_cast<Request *>(msg))
     handleRequest(request);
     else
     handlePacketFromHL(check_and_cast<Packet*>(msg));
     }
     else if (msg->arrivedOn("queueIn")) {    // from network
     EV_INFO << "Received " << msg << " from network.\n";
     handleIncomingDatagram(check_and_cast<Packet*>(msg));
     }
     else
     throw cRuntimeError("message arrived on unknown gate '%s'", msg->getArrivalGate()->getName());
     */
}

/**
 * TODO: implement ILifeCycleMethods: stop should flush all pending packets (see Ipv4(?))
 */

void LteIp::handleStartOperation(LifecycleOperation *operation) {
    throw cRuntimeError(" handleStartOperation not implemented");
}

void LteIp::handleStopOperation(LifecycleOperation *operation) {
    throw cRuntimeError(" handleStopOperation not implemented");
}

void LteIp::handleCrashOperation(LifecycleOperation *operation) {
    throw cRuntimeError(" handleCrashOperation not implemented");
}

