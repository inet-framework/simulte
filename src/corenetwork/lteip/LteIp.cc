//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"

#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/transportlayer/tcp_common/TcpHeader.h"
#include "corenetwork/lteip/LteIp.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/transportlayer/common/L4Tools.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/DscpTag_m.h"
#include "inet/networklayer/common/EcnTag_m.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4SocketCommand_m.h"
#include "inet/applications/common/SocketTag_m.h"
#include "inet/networklayer/common/TosTag_m.h"

#include "inet/common/ProtocolTag_m.h"
#include "../lteCellInfo/LteCellInfo.h"
#include "corenetwork/binder/LteBinder.h"
#include "inet/common/ModuleAccess.h"

using namespace omnetpp;
using namespace inet;

Define_Module(LteIp);

void LteIp::initialize(int stage)
{

    if (stage == INITSTAGE_LOCAL) {
        stackGateOut_ = gate("stackLte$o");
        peerGateOut_ = gate("ifOut");
        defaultTimeToLive_ = par("timeToLive");
        //mapping_.parseProtocolMapping(par("protocolMapping"));
        setNodeType(par("nodeType").stdstringValue());
        numDropped_ = numForwarded_ = seqNum_ = 0;
        if (nodeType_ == UE)
        {
            cModule *ue = getParentModule();
            getBinder()->registerNode(ue, nodeType_, ue->par("masterId"));
        }
        else if (nodeType_ == ENODEB)
        {
            cModule *enodeb = getParentModule();
            getBinder()->registerNode(enodeb, nodeType_);
        }
        registerService(Protocol::ipv4, gate("transportIn"), nullptr);
        registerProtocol(Protocol::ipv4, nullptr, gate("transportOut"));
        WATCH(numForwarded_);
        WATCH(numDropped_);
        WATCH(seqNum_);
    }
}

void LteIp::updateDisplayString()
{
    char buf[80] = "";
    if (numForwarded_ > 0)
        sprintf(buf + strlen(buf), "fwd:%d ", numForwarded_);
    if (numDropped_ > 0)
        sprintf(buf + strlen(buf), "drop:%d ", numDropped_);
    getDisplayString().setTagArg("t", 0, buf);
}


void LteIp::handleRequest(inet::Request *request)
{
    auto ctrl = request->getControlInfo();
    if (ctrl == nullptr)
        throw cRuntimeError("Request '%s' arrived without controlinfo", request->getName());
    else if (Ipv4SocketBindCommand *command = dynamic_cast<Ipv4SocketBindCommand *>(ctrl)) {
        int socketId = request->getTag<SocketReq>()->getSocketId();
        SocketDescriptor *descriptor = new SocketDescriptor(socketId, command->getProtocol()->getId(), command->getLocalAddress());
        socketIdToSocketDescriptor[socketId] = descriptor;
        delete request;
    }
    else if (Ipv4SocketConnectCommand *command = dynamic_cast<Ipv4SocketConnectCommand *>(ctrl)) {
        int socketId = request->getTag<SocketReq>()->getSocketId();
        if (socketIdToSocketDescriptor.find(socketId) == socketIdToSocketDescriptor.end())
            throw cRuntimeError("Ipv4Socket: should use bind() before connect()");
        socketIdToSocketDescriptor[socketId]->remoteAddress = command->getRemoteAddress();
        delete request;
    }
    else if (dynamic_cast<Ipv4SocketCloseCommand *>(ctrl) != nullptr) {
        int socketId = request->getTag<SocketReq>()->getSocketId();
        auto it = socketIdToSocketDescriptor.find(socketId);
        if (it != socketIdToSocketDescriptor.end()) {
            delete it->second;
            socketIdToSocketDescriptor.erase(it);
            auto indication = new inet::Indication("closed", IPv4_I_SOCKET_CLOSED);
            auto ctrl = new inet::Ipv4SocketClosedIndication();
            indication->setControlInfo(ctrl);
            indication->addTag<SocketInd>()->setSocketId(socketId);
            send(indication, "transportOut");
        }
        delete request;
    }
    else if (dynamic_cast<Ipv4SocketDestroyCommand *>(ctrl) != nullptr) {
        int socketId = request->getTag<SocketReq>()->getSocketId();
        auto it = socketIdToSocketDescriptor.find(socketId);
        if (it != socketIdToSocketDescriptor.end()) {
            delete it->second;
            socketIdToSocketDescriptor.erase(it);
        }
        delete request;
    }
    else
        throw cRuntimeError("Unknown command: '%s' with %s", request->getName(), ctrl->getClassName());
}


void LteIp::handleRegisterService(const Protocol& protocol, cGate *out, ServicePrimitive servicePrimitive)
{
    Enter_Method("handleRegisterService");
}

void LteIp::handleRegisterProtocol(const Protocol& protocol, cGate *in, ServicePrimitive servicePrimitive)
{
    Enter_Method("handleRegisterProtocol");
    if (in->isName("transportIn"))
            upperProtocols.insert(&protocol);
}


void LteIp::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("transportIn")) {    //TODO packet->getArrivalGate()->getBaseId() == transportInGateBaseId
        if (auto request = dynamic_cast<Request *>(msg))
            handleRequest(request);
    }

    if (nodeType_ == INTERNET)
    {
        if (msg->getArrivalGate()->isName("transportIn"))
        {
            // message from transport: send to peer
            if (auto request = dynamic_cast<Request *>(msg))
                       handleRequest(request);
            else {
                numForwarded_++;
                EV << "LteIp: message from transport: send to peer" << endl;
                fromTransport(check_and_cast<Packet *>(msg), peerGateOut_);
            }
        }
        else if (msg->getArrivalGate()->isName("ifIn"))
        {
            // message from peer: send to transport
            numForwarded_++;
            EV << "LteIp: message from peer: send to transport" << endl;
            auto pkt = check_and_cast<Packet *>(msg);
            auto ipDatagram = pkt->peekAtFront<Ipv4Header>();
            pkt->addTagIfAbsent<NetworkProtocolInd>()->setProtocol(&Protocol::ipv4);
            pkt->addTagIfAbsent<NetworkProtocolInd>()->setNetworkProtocolHeader(ipDatagram);
            toTransport(pkt);
        }
        else
        {
            // error: drop message
            numDropped_++;
            delete msg;
            EV << "LteIp (INTERNET): Wrong gate " << msg->getArrivalGate()->getName() << endl;
        }
    }
    else if( nodeType_ == ENODEB )
    {
        if (msg->getArrivalGate()->isName("ifIn"))
        {
            // message from peer: send to stack
            numForwarded_++;
            EV << "LteIp: message from peer: send to stack" << endl;

            auto pkt = check_and_cast<Packet *>(msg);
            auto ipDatagram = pkt->peekAtFront<Ipv4Header>();
            pkt->addTagIfAbsent<NetworkProtocolInd>()->setProtocol(&Protocol::ipv4);
            pkt->addTagIfAbsent<NetworkProtocolInd>()->setNetworkProtocolHeader(ipDatagram);

            auto transportHeader = getTransportProtocolHeader(pkt);
            int transportProtocol = ipDatagram->getProtocolId();

            unsigned short srcPort = 0;
            unsigned short dstPort = 0;
            inet::B headerSize = inet::IPv4_MIN_HEADER_LENGTH;

            if (IP_PROT_TCP == transportProtocol) {
                auto tcpseg = dynamicPtrCast<const tcp::TcpHeader>(transportHeader);
                srcPort = tcpseg->getSrcPort();
                dstPort = tcpseg->getDestPort();
                headerSize += tcpseg->getHeaderLength();
            }
            else if (IP_PROT_UDP == transportProtocol) {
                auto udppacket = dynamicPtrCast<const UdpHeader>(transportHeader);
                srcPort = udppacket->getSrcPort();
                dstPort = udppacket->getDestPort();
                headerSize += inet::UDP_HEADER_LENGTH;
            }
            pkt->addTagIfAbsent<FlowControlInfo>()->setSrcAddr(ipDatagram->getSrcAddress().getInt());
            pkt->addTagIfAbsent<FlowControlInfo>()->setDstAddr(ipDatagram->getDestAddress().getInt());
            pkt->addTagIfAbsent<FlowControlInfo>()->setSrcPort(srcPort);
            pkt->addTagIfAbsent<FlowControlInfo>()->setDstPort(dstPort);
            pkt->addTagIfAbsent<FlowControlInfo>()->setSequenceNumber(seqNum_++);
            pkt->addTagIfAbsent<FlowControlInfo>()->setHeaderSize(headerSize.get());
            MacNodeId destId = getBinder()->getMacNodeId(ipDatagram->getDestAddress());
            // master of this ue (myself or a relay)

            MacNodeId master = getBinder()->getNextHop(destId);
//            if (master != nodeId_) { // ue is relayed, dest must be the relay
            destId = master;
//            } // else ue is directly attached
            pkt->addTagIfAbsent<FlowControlInfo>()->setDestId(destId);
            printControlInfo(pkt);
            send(pkt,stackGateOut_);
        }
        else if(msg->getArrivalGate()->isName("stackLte$i"))
        {
            // message from stack: send to peer
            numForwarded_++;
            EV << "LteIp: message from stack: send to peer" << endl;
            send(msg,peerGateOut_);
        }
        else
        {
            // error: drop message
            numDropped_++;
            delete msg;
            EV << "LteIp (ENODEB): Wrong gate " << msg->getArrivalGate()->getName() << endl;
        }
    }
    else if( nodeType_ == UE )
    {
        if (msg->getArrivalGate()->isName("transportIn"))
        {
            // message from transport: send to stack
            numForwarded_++;
            EV << "LteIp: message from transport: send to stack" << endl;
            fromTransport(check_and_cast<Packet *> (msg), stackGateOut_);
        }
        else if(msg->getArrivalGate()->isName("stackLte$i"))
        {
            // message from stack: send to transport
            numForwarded_++;
            EV << "LteIp: message from stack: send to transport" << endl;
            toTransport(check_and_cast<Packet *> (msg));
        }
        else
        {
            // error: drop message
            numDropped_++;
            delete msg;
            EV << "LteIp (UE): Wrong gate " << msg->getArrivalGate()->getName() << endl;
        }
    }

    if (getEnvir()->isGUI())
    updateDisplayString();
}

void LteIp::fromTransport(Packet * transportPacket, cGate *outputgate)
{
    auto hopLimitReq = transportPacket->removeTagIfPresent<inet::HopLimitReq>();
    auto l3AddressReq = transportPacket->removeTag<inet::L3AddressReq>();
    Ipv4Address src = l3AddressReq->getSrcAddress().toIpv4();
    Ipv4Address dest = l3AddressReq->getDestAddress().toIpv4();
    short ttl = (hopLimitReq != nullptr) ? hopLimitReq->getHopLimit() : -1;

    removeAllSimuLteTags(transportPacket);

    if (hopLimitReq)
        delete hopLimitReq;
    if (l3AddressReq)
        delete l3AddressReq;


    //** Create IP datagram and fill its fields **
    auto ipHeader = makeShared<Ipv4Header>();
    ipHeader->setDestAddress(dest);

    // when source address was given, use it; otherwise use local Addr
    if (src.isUnspecified())
    {
        // find interface entry and use its address
        IInterfaceTable *interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        src = interfaceTable->findInterfaceByName(par("interfaceName").stringValue())->getIpv4Address();
        EV << "Local address used: " << src << endl;
    }
    else
    {
        EV << "Source address in control info from transport layer " << src << endl;
    }
    ipHeader->setSrcAddress(src);
    ipHeader->setProtocolId((IpProtocolId)ProtocolGroup::ipprotocol.getProtocolNumber(transportPacket->getTag<PacketProtocolTag>()->getProtocol()));

    bool dontFragment = false;
    if (auto dontFragmentReq = transportPacket->removeTagIfPresent<FragmentationReq>()) {
        dontFragment = dontFragmentReq->getDontFragment();
        delete dontFragmentReq;
    }


    // set other fields
    if (TosReq *tosReq = transportPacket->removeTagIfPresent<TosReq>()) {
        ipHeader->setTypeOfService(tosReq->getTos());
        delete tosReq;
        if (transportPacket->findTag<DscpReq>())
            throw cRuntimeError("TosReq and DscpReq found together");
        if (transportPacket->findTag<EcnReq>())
            throw cRuntimeError("TosReq and EcnReq found together");
    }
    if (DscpReq *dscpReq = transportPacket->removeTagIfPresent<DscpReq>()) {
        ipHeader->setDscp(dscpReq->getDifferentiatedServicesCodePoint());
        delete dscpReq;
    }
    if (EcnReq *ecnReq = transportPacket->removeTagIfPresent<EcnReq>()) {
        ipHeader->setEcn(ecnReq->getExplicitCongestionNotification());
        delete ecnReq;
    }

    ipHeader->setIdentification(curFragmentId_++);
    ipHeader->setMoreFragments(false);
    ipHeader->setDontFragment(dontFragment);
    ipHeader->setFragmentOffset(0);
    ipHeader->setTimeToLive(ttl);

    //** Add control info for stack **
    unsigned short srcPort = 0;
    unsigned short dstPort = 0;
    inet::B headerSize = IPv4_MIN_HEADER_LENGTH;

    auto trasnportHeader = getTransportProtocolHeader(transportPacket);
    if (IP_PROT_TCP == ipHeader->getProtocolId()) {
        auto tcpseg = dynamicPtrCast<const tcp::TcpHeader>(trasnportHeader);
        srcPort = tcpseg->getSrcPort();
        dstPort = tcpseg->getDestPort();
        headerSize += tcpseg->getHeaderLength();
    }
    else if (IP_PROT_UDP == ipHeader->getProtocolId()) {
        auto udppacket = dynamicPtrCast<const UdpHeader>(trasnportHeader);
        srcPort = udppacket->getSrcPort();
        dstPort = udppacket->getDestPort();
        headerSize += inet::UDP_HEADER_LENGTH;
    }

    insertNetworkProtocolHeader(transportPacket, Protocol::ipv4, ipHeader);

    transportPacket->addTagIfAbsent<FlowControlInfo>()->setSrcAddr(src.getInt());
    transportPacket->addTagIfAbsent<FlowControlInfo>()->setDstAddr(dest.getInt());
    transportPacket->addTagIfAbsent<FlowControlInfo>()->setSrcPort(srcPort);
    transportPacket->addTagIfAbsent<FlowControlInfo>()->setDstPort(dstPort);
    transportPacket->addTagIfAbsent<FlowControlInfo>()->setSequenceNumber(seqNum_++);
    transportPacket->addTagIfAbsent<FlowControlInfo>()->setHeaderSize(headerSize.get());
    printControlInfo(transportPacket);

    //** Send datagram to lte stack or LteIp peer **
    send(transportPacket, outputgate);
}

void LteIp::decapsulate(Packet *packet)
{
    // decapsulate transport packet
    const auto& ipv4Header = packet->popAtFront<Ipv4Header>();
    packet->removeTagIfPresent<NetworkProtocolInd>();

    // create and fill in control info
    packet->addTagIfAbsent<DscpInd>()->setDifferentiatedServicesCodePoint(ipv4Header->getDscp());
    packet->addTagIfAbsent<EcnInd>()->setExplicitCongestionNotification(ipv4Header->getEcn());
    packet->addTagIfAbsent<TosInd>()->setTos(ipv4Header->getTypeOfService());

    // original Ipv4 datagram might be needed in upper layers to send back ICMP error message
    auto transportProtocol = ProtocolGroup::ipprotocol.getProtocol(ipv4Header->getProtocolId());
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(transportProtocol);
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(transportProtocol);
    auto l3AddressInd = packet->addTagIfAbsent<L3AddressInd>();
    l3AddressInd->setSrcAddress(ipv4Header->getSrcAddress());
    l3AddressInd->setDestAddress(ipv4Header->getDestAddress());
    packet->addTagIfAbsent<HopLimitInd>()->setHopLimit(ipv4Header->getTimeToLive());
}

void LteIp::toTransport(Packet * pkt)
{
    // msg is an IP Datagram (from Lte stack or IP peer)

    auto ipv4Header = pkt->peekAtFront<Ipv4Header>();
    const Protocol *protocol = ipv4Header->getProtocol();

    auto remoteAddress(ipv4Header->getSrcAddress());
    auto localAddress(ipv4Header->getDestAddress());
    decapsulate(pkt);

    bool hasSocket = false;
    for (const auto &elem: socketIdToSocketDescriptor) {
        if (elem.second->protocolId == protocol->getId()
                && (elem.second->localAddress.isUnspecified() || elem.second->localAddress == localAddress)
                && (elem.second->remoteAddress.isUnspecified() || elem.second->remoteAddress == remoteAddress)) {
            auto *packetCopy = pkt->dup();
            packetCopy->setKind(IPv4_I_DATA);
            packetCopy->addTagIfAbsent<SocketInd>()->setSocketId(elem.second->socketId);
            EV_INFO << "Passing up to socket " << elem.second->socketId << "\n";
            emit(packetSentToUpperSignal, packetCopy);
            send(packetCopy, "transportOut");
            hasSocket = true;
        }
    }
    if (upperProtocols.find(protocol) != upperProtocols.end()) {
        EV_INFO << "Passing up to protocol " << *protocol << "\n";
        emit(packetSentToUpperSignal, pkt);
        send(pkt, "transportOut");
    }
    else if (hasSocket) {
        delete pkt;
        numForwarded_--;
        numDropped_++;
    }
    else {
        numForwarded_--;
        numDropped_++;
        delete pkt;

        EV_ERROR << "Transport protocol '" << protocol->getName() << "' not connected, discarding packet\n";
        /*pkt->setFrontOffset(ipv4HeaderPosition);
        const InterfaceEntry* fromIE = getSourceInterface(pkt);
        sendIcmpError(pkt, fromIE ? fromIE->getInterfaceId() : -1, ICMP_DESTINATION_UNREACHABLE, ICMP_DU_PROTOCOL_UNREACHABLE);*/
    }
}

void LteIp::setNodeType(std::string s)
{
    nodeType_ = aToNodeType(s);
    EV << "Node type: " << s << " -> " << nodeType_ << endl;
}

void LteIp::printControlInfo(Packet* ci)
{
    EV << "Src IP : " << Ipv4Address(ci->getTag<FlowControlInfo>()->getSrcAddr()) << endl;
    EV << "Dst IP : " << Ipv4Address(ci->getTag<FlowControlInfo>()->getDstAddr()) << endl;
    EV << "Src Port : " << ci->getTag<FlowControlInfo>()->getSrcPort() << endl;
    EV << "Dst Port : " << ci->getTag<FlowControlInfo>()->getDstPort() << endl;
    EV << "Seq Num  : " << ci->getTag<FlowControlInfo>()->getSequenceNumber() << endl;
    EV << "Header Size : " << ci->getTag<FlowControlInfo>()->getHeaderSize() << endl;
}
