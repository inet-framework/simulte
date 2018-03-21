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
#include "inet/transportlayer/tcp_common/TCPSegment.h"
#include "inet/transportlayer/udp/UDPPacket.h"
#include "inet/transportlayer/udp/UDP.h"
#include <iostream>

#include "../lteCellInfo/LteCellInfo.h"
#include "inet/common/ModuleAccess.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/configurator/ipv4/IPv4NetworkConfigurator.h"
#include "inet/networklayer/ipv4/IIPv4RoutingTable.h"
#include "stack/mac/layer/LteMacBase.h"

using namespace std;

Define_Module(IP2lte);

void IP2lte::initialize(int stage)
{
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
            registerInterface();
        }
    }
    if (stage == inet::INITSTAGE_NETWORK_LAYER - 1)  // the configurator runs at stage NETWORK_LAYER, so the interface
    {                                                // must be configured at a previous stage
        if (nodeType_ == UE)
        {
            // TODO not so elegant
            cModule *ue = getParentModule()->getParentModule();
            nodeId_ = binder_->registerNode(ue, nodeType_, ue->par("masterId"));
            registerInterface();

            // if the UE has been created dynamically, we need to manually add a default route having "wlan" as output interface
            // otherwise we are not able to reach devices outside the cellular network
            if (NOW > 0)
            {
                IIPv4RoutingTable *irt = getModuleFromPar<IIPv4RoutingTable>(par("routingTableModule"), this);
                IPv4Route * defaultRoute = new IPv4Route();
                defaultRoute->setDestination(IPv4Address(inet::IPv4Address::UNSPECIFIED_ADDRESS));
                defaultRoute->setNetmask(IPv4Address(inet::IPv4Address::UNSPECIFIED_ADDRESS));

                IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
                InterfaceEntry * interfaceEntry = ift->getInterfaceByName("wlan");
                defaultRoute->setInterface(interfaceEntry);

                irt->addRoute(defaultRoute);
            }
        }
    }
    else if (stage == inet::INITSTAGE_NETWORK_LAYER_3+1)
    {
        registerMulticastGroups();
    }
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
    EV << "IP2lte::fromIpUe - message from IP layer: send to stack" << endl;
    // Remove control info from IP datagram
    delete(datagram->removeControlInfo());

    if (ueHold_)
    {
        // hold packets until handover is complete
        ueHoldFromIp_.push_back(datagram);
    }
    else
    {
        toStackUe(datagram);
    }
}

void IP2lte::toStackUe(IPv4Datagram * datagram)
{
    // obtain the encapsulated transport packet
    cPacket * transportPacket = datagram->getEncapsulatedPacket();

    // 5-Tuple infos
    unsigned short srcPort = 0;
    unsigned short dstPort = 0;
    int transportProtocol = datagram->getTransportProtocol();
    // TODO Add support to IPv6
    IPv4Address srcAddr  = datagram->getSrcAddress() ,
                destAddr = datagram->getDestAddress();

    // if needed, create a new structure for the flow
    AddressPair pair(srcAddr, destAddr);
    if (seqNums_.find(pair) == seqNums_.end())
    {
        std::pair<AddressPair, unsigned int> p(pair, 0);
        seqNums_.insert(p);
    }

    int headerSize = datagram->getHeaderLength();

    // inspect packet depending on the transport protocol type
    switch (transportProtocol)
    {
        case IP_PROT_TCP:
            inet::tcp::TCPSegment* tcpseg;
            tcpseg = check_and_cast<inet::tcp::TCPSegment*>(transportPacket);
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
    controlInfo->setSequenceNumber(seqNums_[pair]++);
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

    // TODO Add support to IPv6
    IPv4Address destAddr = datagram->getDestAddress();

    // handle "forwarding" of packets during handover
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (hoForwarding_.find(destId) != hoForwarding_.end())
    {
        // data packet must be forwarded (via X2) to another eNB
        MacNodeId targetEnb = hoForwarding_.at(destId);
        sendTunneledPacketOnHandover(datagram, targetEnb);
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

        hoFromIp_[destId].push_back(datagram);
        return;
    }

    toStackEnb(datagram);
}

void IP2lte::toIpEnb(cMessage * msg)
{
    EV << "IP2lte::toIpEnb - message from stack: send to IP layer" << endl;
    send(msg,ipGateOut_);
}

void IP2lte::toStackEnb(IPv4Datagram* datagram)
{
    // obtain the encapsulated transport packet
    cPacket * transportPacket = datagram->getEncapsulatedPacket();

    // 5-Tuple infos
    unsigned short srcPort = 0;
    unsigned short dstPort = 0;
    int transportProtocol = datagram->getTransportProtocol();
    IPv4Address srcAddr  = datagram->getSrcAddress() ,
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

    switch(transportProtocol)
    {
        case IP_PROT_TCP:
        inet::tcp::TCPSegment* tcpseg;
        tcpseg = check_and_cast<inet::tcp::TCPSegment*>(transportPacket);
        srcPort = tcpseg->getSrcPort();
        dstPort = tcpseg->getDestPort();
        headerSize += tcpseg->getHeaderLength();
        break;
        case IP_PROT_UDP:
        inet::UDPPacket* udppacket;
        udppacket = check_and_cast<inet::UDPPacket*>(transportPacket);
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
    controlInfo->setSequenceNumber(seqNums_[pair]++);
    controlInfo->setHeaderSize(headerSize);

    // TODO Relay management should be placed here
    MacNodeId master = binder_->getNextHop(destId);

    controlInfo->setDestId(master);
    printControlInfo(controlInfo);
    datagram->setControlInfo(controlInfo);

    send(datagram,stackGateOut_);
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
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (!ift)
        return;
    interfaceEntry = new InterfaceEntry(this);
    interfaceEntry->setName("wlan");
    // TODO configure MTE size from NED
    interfaceEntry->setMtu(1500);
    // enable broadcast/multicast
    interfaceEntry->setBroadcast(true);
    // FIXME: this is a hack required to work with the HostAutoConfigurator in INET 3
    // since the HostAutoConfigurator tries to add us to all default multicast groups.
    // once this problem has been fixed, we should set multicast to false here
    interfaceEntry->setMulticast(true);
    ift->addInterface(interfaceEntry);
}

void IP2lte::registerMulticastGroups()
{
    MacNodeId nodeId = check_and_cast<LteMacBase*>(getParentModule()->getSubmodule("mac"))->getMacNodeId();

    // get all the multicast addresses where the node is enrolled
    InterfaceEntry * interfaceEntry;
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (!ift)
        return;
    interfaceEntry = ift->getInterfaceByName("wlan");
    unsigned int numOfAddresses = interfaceEntry->ipv4Data()->getNumOfJoinedMulticastGroups();
    for (unsigned int i=0; i<numOfAddresses; ++i)
    {
        IPv4Address addr = interfaceEntry->ipv4Data()->getJoinedMulticastGroup(i);
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


void IP2lte::sendTunneledPacketOnHandover(IPv4Datagram* datagram, MacNodeId targetEnb)
{
    EV << "IP2lte::sendTunneledPacketOnHandover - destination is handing over to eNB " << targetEnb << ". Forward packet via X2." << endl;
    if (hoManager_ == NULL)
        hoManager_ = check_and_cast<LteHandoverManager*>(getParentModule()->getSubmodule("handoverManager"));
    hoManager_->forwardDataToTargetEnb(datagram, targetEnb);
}

void IP2lte::receiveTunneledPacketOnHandover(IPv4Datagram* datagram, MacNodeId sourceEnb)
{
    EV << "IP2lte::receiveTunneledPacketOnHandover - received packet via X2 from " << sourceEnb << endl;
    IPv4Address destAddr = datagram->getDestAddress();
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    if (hoFromX2_.find(destId) == hoFromX2_.end())
    {
        IpDatagramQueue queue;
        hoFromX2_[destId] = queue;
    }

    hoFromX2_[destId].push_back(datagram);
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
            IPv4Datagram* pkt = it->second.front();
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
            IPv4Datagram* pkt = it->second.front();
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
        IPv4Datagram* pkt = ueHoldFromIp_.front();
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
            IPv4Datagram* pkt = it->second.front();
            it->second.pop_front();
            delete pkt;
        }
    }

    for (it = hoFromIp_.begin(); it != hoFromIp_.end(); ++it)
    {
        while (!it->second.empty())
        {
            IPv4Datagram* pkt = it->second.front();
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
