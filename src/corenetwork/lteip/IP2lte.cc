//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "corenetwork/lteip/IP2lte.h"
#include "corenetwork/binder/LteBinder.h"
#include "inet/transportlayer/udp/Udp.h"
#include <iostream>

#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/transportlayer/tcp_common/TcpHeader.h"

#include "../lteCellInfo/LteCellInfo.h"
#include "inet/common/ModuleAccess.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/configurator/ipv4/Ipv4NetworkConfigurator.h"
#include "inet/networklayer/ipv4/IIpv4RoutingTable.h"
#include "stack/mac/layer/LteMacBase.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/transportlayer/common/L4Tools.h"
#include "inet/common/MessageDispatcher.h"
#include "inet/applications/common/SocketTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"

using namespace std;
using namespace inet;
using namespace omnetpp;

Define_Module(IP2lte);

void IP2lte::initialize(int stage)
{
    inet::MacProtocolBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
        stackGateOut_ = gate("stackLte$o");
        ipGateOut_ = gate("upperLayerOut");

        setNodeType(par("nodeType").stdstringValue());

        seqNum_ = 0;

        hoManager_ = NULL;

        ueHold_ = false;

        binder_ = getBinder();

        if (nodeType_ == ENODEB)
        {
            // TODO not so elegant
            cModule *enodeb = getParentModule()->getParentModule();
            MacNodeId cellId = getBinder()->registerNode(enodeb, nodeType_);
            nodeId_ = cellId;
            //registerInterface();
        }

    }
    else if (stage == inet::INITSTAGE_LINK_LAYER)  // the configurator runs at stage NETWORK_LAYER, so the interface
    {                                                // must be configured at a previous stage
        if (nodeType_ == UE)
        {
            // TODO not so elegant
            cModule *ue = getParentModule()->getParentModule();
            nodeId_ = binder_->registerNode(ue, nodeType_, ue->par("masterId"));
            // registerInterface();

            // if the UE has been created dynamically, we need to manually add a default route having "wlan" as output interface
            // otherwise we are not able to reach devices outside the cellular network
            if (NOW > 0)
            {
                IIpv4RoutingTable *irt = getModuleFromPar<IIpv4RoutingTable>(par("routingTableModule"), this);
                Ipv4Route * defaultRoute = new Ipv4Route();
                defaultRoute->setDestination(Ipv4Address(inet::Ipv4Address::UNSPECIFIED_ADDRESS));
                defaultRoute->setNetmask(Ipv4Address(inet::Ipv4Address::UNSPECIFIED_ADDRESS));

                IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
                InterfaceEntry * interfaceEntry = ift->findInterfaceByName("wlan");
                defaultRoute->setInterface(interfaceEntry);

                irt->addRoute(defaultRoute);
            }
        }
        auto endGate = ipGateOut_->getPathEndGate();
        auto mod = endGate->getOwnerModule();
        if (dynamic_cast<inet::MessageDispatcher *>(mod)) {
            auto parent = this->getParentModule();
            inet::registerService(LteProtocol::lte, parent->gate("upperLayerIn"), nullptr);
            inet::registerProtocol(LteProtocol::lte, nullptr, parent->gate("upperLayerOut"));
        }
    }
    else if (stage == inet::INITSTAGE_TRANSPORT_LAYER)
    {
        registerMulticastGroups();
    }
}

void IP2lte::handleMessageWhenUp(cMessage *msg)
{
    if( nodeType_ == ENODEB )
    {
        // message from IP Layer: send to stack
        if (msg->getArrivalGate()->isName("upperLayerIn"))
        {
            auto pkt = check_and_cast<Packet *>(msg);
            auto ipDatagram = pkt->peekAtFront<Ipv4Header>();
            fromIpEnb(pkt);
        }
        // message from stack: send to peer
        else if(msg->getArrivalGate()->isName("stackLte$i")) {
            auto pkt = check_and_cast<Packet *>(msg);
            decapsulate(pkt);
            toIpEnb(msg);
        }
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
            auto pkt = check_and_cast<Packet *>(msg);
            auto sockInd = pkt->removeTagIfPresent<SocketInd>();
            if (sockInd)
                delete sockInd;
            auto header = pkt->peekAtFront<Ipv4Header>();
            EV << "LteIp: message from transport: send to stack" << endl;
            pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::lte);
            fromIpUe(pkt);
        }
        else if(msg->getArrivalGate()->isName("stackLte$i"))
        {
            // message from stack: send to transport
            EV << "LteIp: message from stack: send to transport" << endl;
            auto pkt = check_and_cast<Packet *>(msg);
            decapsulate(pkt);
            toIpUe(pkt);
        }
        else
        {
            // error: drop message
            delete msg;
            EV << "LteIp (UE): Wrong gate " << msg->getArrivalGate()->getName() << endl;
        }
    }
}

void IP2lte::decapsulate(Packet *pkt)
{
    auto sockInd = pkt->removeTagIfPresent<SocketInd>();
    if (sockInd)
        delete sockInd;
    removeAllSimuLteTags(pkt);
    auto ipDatagram = pkt->peekAtFront<Ipv4Header>();
    auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
    networkProtocolInd->setProtocol(&Protocol::ipv4);
    networkProtocolInd->setNetworkProtocolHeader(ipDatagram);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
    pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    pkt->addTagIfAbsent<InterfaceInd>()->setInterfaceId(interfaceEntry->getInterfaceId());
}

void IP2lte::setNodeType(std::string s)
{
    nodeType_ = aToNodeType(s);
    EV << "Node type: " << s << " -> " << nodeType_ << endl;
}

void IP2lte::fromIpUe(Packet * pkt)
{

    EV << "IP2lte::fromIpUe - message from IP layer: send to stack" << endl;
    // Remove control info from IP datagram
    auto sockInd = pkt->removeTagIfPresent<SocketInd>();
    if (sockInd)
        delete sockInd;
    removeAllSimuLteTags(pkt);

    auto tag = pkt->removeTagIfPresent<FlowControlInfo>();
    if (tag)
        delete tag;

    if (ueHold_)
    {
        // hold packets until handover is complete
        ueHoldFromIp_.push_back(pkt);
    }
    else
    {
        toStackUe(pkt);
    }
}

void IP2lte::toStackUe(Packet * pkt)
{
    // obtain the encapsulated transport packet
   // cPacket * transportPacket = datagram->getEncapsulatedPacket();

    // 5-Tuple infos
    unsigned short srcPort = 0;
    unsigned short dstPort = 0;


    auto protocolId = getProtocolId(pkt);
    auto datagram = pkt->peekAtFront<Ipv4Header>();
    auto trasnportHeader = getTransportProtocolHeader(pkt);

    int transportProtocol = datagram->getProtocolId();
    // TODO Add support to IPv6 (=> see L3Tools.cc of INET)
    auto srcAddr  = datagram->getSrcAddress();
    auto destAddr = datagram->getDestAddress();

    // if needed, create a new structure for the flow
    AddressPair pair(srcAddr, destAddr);
    if (seqNums_.find(pair) == seqNums_.end())
    {
        std::pair<AddressPair, unsigned int> p(pair, 0);
        seqNums_.insert(p);
    }

    int headerSize = datagram->getHeaderLength().get();

    // inspect packet depending on the transport protocol type
    if (IP_PROT_TCP == transportProtocol) {
        auto tcpseg = dynamicPtrCast<const tcp::TcpHeader>(trasnportHeader);
        srcPort = tcpseg->getSrcPort();
        dstPort = tcpseg->getDestPort();
        headerSize += tcpseg->getHeaderLength().get();
    }
    else if (IP_PROT_UDP == transportProtocol) {
        auto udppacket = dynamicPtrCast<const UdpHeader>(trasnportHeader);
        srcPort = udppacket->getSrcPort();
        dstPort = udppacket->getDestPort();
        headerSize += inet::UDP_HEADER_LENGTH.get();
    } else {
        EV_ERROR << "Unknown transport protocol id." << endl;
    }

    pkt->addTagIfAbsent<FlowControlInfo>()->setSrcAddr(srcAddr.getInt());
    pkt->addTagIfAbsent<FlowControlInfo>()->setDstAddr(destAddr.getInt());
    pkt->addTagIfAbsent<FlowControlInfo>()->setSrcPort(srcPort);
    pkt->addTagIfAbsent<FlowControlInfo>()->setDstPort(dstPort);
    pkt->addTagIfAbsent<FlowControlInfo>()->setSequenceNumber(seqNums_[pair]++);
    pkt->addTagIfAbsent<FlowControlInfo>()->setHeaderSize(headerSize);
    printControlInfo(pkt);

    //datagram->setControlInfo(controlInfo);

    //** Send datagram to lte stack or LteIp peer **
    send(pkt,stackGateOut_);
}

/*
void IP2lte::prepareForIpv4(Packet *datagram, const Protocol *protocol){
    // add DispatchProtocolRequest so that the packet is handled by the specified protocol
    datagram->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(protocol);
    datagram->addTagIfAbsent<PacketProtocolTag>()->setProtocol(protocol);
    // add Interface-Indication to indicate which interface this packet was received from
    datagram->addTagIfAbsent<InterfaceInd>()->setInterfaceId(interfaceEntry->getInterfaceId());
}
*/

void IP2lte::toIpUe(Packet *datagram)
{
    EV << "IP2lte::toIpUe - message from stack: send to IP layer" << endl;
    send(datagram,ipGateOut_);
}

void IP2lte::fromIpEnb(Packet * pkt)
{
    EV << "IP2lte::fromIpEnb - message from IP layer: send to stack" << endl;
    // Remove control info from IP datagram
    auto sockInd = pkt->removeTagIfPresent<SocketInd>();
    if (sockInd)
        delete sockInd;
    //delete(datagram->removeControlInfo());
    removeAllSimuLteTags(pkt);

    auto datagram = pkt->peekAtFront<Ipv4Header>();

    // TODO Add support to IPv6
    Ipv4Address destAddr = datagram->getDestAddress();

    // handle "forwarding" of packets during handover
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (hoForwarding_.find(destId) != hoForwarding_.end())
    {
        // data packet must be forwarded (via X2) to another eNB
        MacNodeId targetEnb = hoForwarding_.at(destId);
        sendTunneledPacketOnHandover(pkt, targetEnb);
        return;
    }

    // handle incoming packets destined to UEs that is completing handover
    if (hoHolding_.find(destId) != hoHolding_.end())
    {
        // hold packets until handover is complete
        if (hoFromIp_.find(destId) == hoFromIp_.end())
        {
            IpDatagramQueue queue;
            hoFromIp_[destId] = queue;
        }

        hoFromIp_[destId].push_back(pkt);
        return;
    }

    toStackEnb(pkt);
}

void IP2lte::toIpEnb(cMessage * msg)
{
    EV << "IP2lte::toIpEnb - message from stack: send to IP layer" << endl;
    // prepareForIpv4(msg, &LteProtocol::ipv4uu);
    send(msg,ipGateOut_);
}

void IP2lte::toStackEnb(Packet* pkt)
{
    // obtain the encapsulated transport packet
    auto datagram = pkt->peekAtFront<Ipv4Header>();
    auto transportHeader = getTransportProtocolHeader(pkt);
    // 5-Tuple infos
    unsigned short srcPort = 0;
    unsigned short dstPort = 0;
    int transportProtocol = datagram->getProtocolId();
    Ipv4Address srcAddr  = datagram->getSrcAddress() ,
                destAddr = datagram->getDestAddress();
    MacNodeId destId = binder_->getMacNodeId(destAddr);

    // if needed, create a new structure for the flow
    AddressPair pair(srcAddr, destAddr);
    if (seqNums_.find(pair) == seqNums_.end())
    {
        std::pair<AddressPair, unsigned int> p(pair, 0);
        seqNums_.insert(p);
    }

    int headerSize = 0;

    if (IP_PROT_TCP == transportProtocol) {
        auto tcpseg = dynamicPtrCast<const tcp::TcpHeader>(transportHeader);
        srcPort = tcpseg->getSrcPort();
        dstPort = tcpseg->getDestPort();
        headerSize += tcpseg->getHeaderLength().get();
    }
    else if (IP_PROT_UDP == transportProtocol) {
        auto udppacket = dynamicPtrCast<const UdpHeader>(transportHeader);
        srcPort = udppacket->getSrcPort();
        dstPort = udppacket->getDestPort();
        headerSize += inet::UDP_HEADER_LENGTH.get();
    } else {
            EV_ERROR << "Unknown transport protocol id." << endl;
    }

    // FlowControlInfo *controlInfo = new FlowControlInfo();
    pkt->addTagIfAbsent<FlowControlInfo>()->setSrcAddr(srcAddr.getInt());
    pkt->addTagIfAbsent<FlowControlInfo>()->setDstAddr(destAddr.getInt());
    pkt->addTagIfAbsent<FlowControlInfo>()->setSrcPort(srcPort);
    pkt->addTagIfAbsent<FlowControlInfo>()->setDstPort(dstPort);
    pkt->addTagIfAbsent<FlowControlInfo>()->setSequenceNumber(seqNums_[pair]++);
    pkt->addTagIfAbsent<FlowControlInfo>()->setHeaderSize(headerSize);

    // TODO Relay management should be placed here
    MacNodeId master = binder_->getNextHop(destId);

    pkt->addTagIfAbsent<FlowControlInfo>()->setDestId(master);
    printControlInfo(pkt);
    //datagram->setControlInfo(controlInfo);
    send(pkt,stackGateOut_);
}


void IP2lte::printControlInfo(Packet* pkt)
{
    EV << "Src IP : " << Ipv4Address(pkt->getTag<FlowControlInfo>()->getSrcAddr()) << endl;
    EV << "Dst IP : " << Ipv4Address(pkt->getTag<FlowControlInfo>()->getDstAddr()) << endl;
    EV << "Src Port : " << pkt->getTag<FlowControlInfo>()->getSrcPort() << endl;
    EV << "Dst Port : " << pkt->getTag<FlowControlInfo>()->getDstPort() << endl;
    EV << "Seq Num  : " << pkt->getTag<FlowControlInfo>()->getSequenceNumber() << endl;
    EV << "Header Size : " << pkt->getTag<FlowControlInfo>()->getHeaderSize() << endl;
}


void IP2lte::configureInterfaceEntry()
{
    // data rate
    interfaceEntry->setInterfaceName("wlan");
    // TODO configure MTE size from NED
    interfaceEntry->setMtu(1500);
    // enable broadcast/multicast
    interfaceEntry->setBroadcast(true);
    interfaceEntry->setMulticast(true);
    interfaceEntry->setLoopback(false);

    // generate a link-layer address to be used as interface token for IPv6
    InterfaceToken token(0, getSimulation()->getUniqueNumber(), 64);
    interfaceEntry->setInterfaceToken(token);

    // capabilities
    interfaceEntry->setMulticast(true);
    interfaceEntry->setPointToPoint(true);
}

void IP2lte::registerMulticastGroups()
{
    MacNodeId nodeId = check_and_cast<LteMacBase*>(getParentModule()->getSubmodule("mac"))->getMacNodeId();

    // get all the multicast addresses where the node is enrolled
    InterfaceEntry * interfaceEntry;
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (!ift)
        return;
    interfaceEntry = ift->findInterfaceByName("wlan");
    unsigned int numOfAddresses = interfaceEntry->getProtocolData<Ipv4InterfaceData>()->getNumOfJoinedMulticastGroups();
    for (unsigned int i=0; i<numOfAddresses; ++i)
    {
        Ipv4Address addr = interfaceEntry->getProtocolData<Ipv4InterfaceData>()->getJoinedMulticastGroup(i);
        // get the group id and add it to the binder
        uint32 address = addr.getInt();
        uint32 mask = ~((uint32)255 << 28);      // 0000 1111 1111 1111
        uint32 groupId = address & mask;
        binder_->registerMulticastGroup(nodeId, groupId);
    }
}

void IP2lte::triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " IP2lte::triggerHandoverSource - start tunneling of packets destined to " << ueId << " towards eNB " << targetEnb << endl;

    hoForwarding_[ueId] = targetEnb;

    if (hoManager_ == NULL)
        hoManager_ = check_and_cast<LteHandoverManager*>(getParentModule()->getSubmodule("handoverManager"));
    hoManager_->sendHandoverCommand(ueId, targetEnb, true);
}

void IP2lte::triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb)
{
    EV << NOW << " IP2lte::triggerHandoverTarget - start holding packets destined to " << ueId << endl;

    // reception of handover command from X2
    hoHolding_.insert(ueId);
}


void IP2lte::sendTunneledPacketOnHandover(Packet* pkt, MacNodeId targetEnb)
{
    EV << "IP2lte::sendTunneledPacketOnHandover - destination is handing over to eNB " << targetEnb << ". Forward packet via X2." << endl;
    if (hoManager_ == NULL)
        hoManager_ = check_and_cast<LteHandoverManager*>(getParentModule()->getSubmodule("handoverManager"));
    hoManager_->forwardDataToTargetEnb(pkt, targetEnb);
}

void IP2lte::receiveTunneledPacketOnHandover(Packet* pkt, MacNodeId sourceEnb)
{
    EV << "IP2lte::receiveTunneledPacketOnHandover - received packet via X2 from " << sourceEnb << endl;

    auto datagram = pkt->peekAtFront<Ipv4Header>();
    Ipv4Address destAddr = datagram->getDestAddress();
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (hoFromX2_.find(destId) == hoFromX2_.end())
    {
        IpDatagramQueue queue;
        hoFromX2_[destId] = queue;
    }

    hoFromX2_[destId].push_back(pkt);
}

void IP2lte::signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb)
{
    EV << NOW << " IP2lte::signalHandoverCompleteSource - handover of UE " << ueId << " to eNB " << targetEnb << " completed!" << endl;
    hoForwarding_.erase(ueId);
}

void IP2lte::signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb)
{
    Enter_Method("signalHandoverCompleteTarget");

    // signal the event to the source eNB
    if (hoManager_ == NULL)
        hoManager_ = check_and_cast<LteHandoverManager*>(getParentModule()->getSubmodule("handoverManager"));
    hoManager_->sendHandoverCommand(ueId, sourceEnb, false);

    // TODO start a timer (there may be in-flight packets)
    // send down buffered packets in the following order:
    // 1) packets received from X2
    // 2) packets received from IP
//    cMessage* handoverFinish = new cMessage("handoverFinish");
//    scheduleAt(NOW + 0.005, handoverFinish);   // TODO check delay

    std::map<MacNodeId, IpDatagramQueue>::iterator it;
    for (it = hoFromX2_.begin(); it != hoFromX2_.end(); ++it)
    {
        while (!it->second.empty())
        {
            auto pkt = it->second.front();
            it->second.pop_front();

            // send pkt down
            take(pkt);
            toStackEnb(pkt);
        }
    }

    for (it = hoFromIp_.begin(); it != hoFromIp_.end(); ++it)
    {
        while (!it->second.empty())
        {
            auto pkt = it->second.front();
            it->second.pop_front();

            // send pkt down
            take(pkt);
            toStackEnb(pkt);
        }
    }

    hoHolding_.erase(ueId);

}

void IP2lte::triggerHandoverUe()
{
    EV << NOW << " IP2lte::triggerHandoverUe - start holding packets" << endl;

    // reception of handover command from X2
    ueHold_ = true;
}


void IP2lte::signalHandoverCompleteUe()
{
    Enter_Method("signalHandoverCompleteUe");

    // send held packets
    while (!ueHoldFromIp_.empty())
    {
        auto pkt = ueHoldFromIp_.front();
        ueHoldFromIp_.pop_front();

        // send pkt down
        take(pkt);
        toStackUe(pkt);
    }
    ueHold_ = false;

}

IP2lte::~IP2lte()
{
    std::map<MacNodeId, IpDatagramQueue>::iterator it;
    for (it = hoFromX2_.begin(); it != hoFromX2_.end(); ++it)
    {
        while (!it->second.empty())
        {
            auto pkt = it->second.front();
            it->second.pop_front();
            delete pkt;
        }
    }

    for (it = hoFromIp_.begin(); it != hoFromIp_.end(); ++it)
    {
        while (!it->second.empty())
        {
            auto pkt = it->second.front();
            it->second.pop_front();
            delete pkt;
        }
    }
}

void IP2lte::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH)
    {
        // do this only at deletion of the module during the simulation
        binder_->unregisterNode(nodeId_);
    }
}
