//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LtePdcpRrcUeD2D.h"
#include "L3AddressResolver.h"
#include "D2DModeSwitchNotification_m.h"

Define_Module(LtePdcpRrcUeD2D);

/*
 * Upper Layer handlers
 */
void LtePdcpRrcUeD2D::fromDataPort(cPacket *pkt)
{
    emit(receivedPacketFromUpperLayer, pkt);

    // Control Informations
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());
    setTrafficInformation(pkt, lteInfo);
    headerCompress(pkt, lteInfo->getHeaderSize()); // header compression

    // get destination info
    IPv4Address destAddr = IPv4Address(lteInfo->getDstAddr());
    MacNodeId destId;

    // the direction of the incoming connection is a D2D_MULTI one if the application is of the same type,
    // else the direction will be selected according to the current status of the UE, i.e. D2D or UL
    if (destAddr.isMulticast())
    {
        lteInfo->setDirection(D2D_MULTI);

        // assign a multicast group id
        // multicast IP addresses are 224.0.0.0/4.
        // We consider the host part of the IP address (the remaining 28 bits) as identifier of the group,
        // so as it is univocally determined for the whole network
        uint32 address = IPv4Address(lteInfo->getDstAddr()).getInt();
        uint32 mask = ~((uint32)255 << 28);      // 0000 1111 1111 1111
        uint32 groupId = address & mask;
        lteInfo->setMulticastGroupId((int32)groupId);
    }
    else
    {
        // set direction based on the destination Id. If the destination can be reached
        // using D2D, set D2D direction. Otherwise, set UL direction
        destId = binder_->getMacNodeId(destAddr);
        lteInfo->setDirection(getDirection(destId));

        if (binder_->checkD2DCapability(nodeId_, destId))
        {
            // this way, we record the ID of the endpoint even if the connection is in IM
            // this is useful for mode switching
            lteInfo->setD2dTxPeerId(nodeId_);
            lteInfo->setD2dRxPeerId(destId);
        }
        else
        {
            lteInfo->setD2dTxPeerId(0);
            lteInfo->setD2dRxPeerId(0);
        }
    }

    // Cid Request
    EV << NOW << " LtePdcpRrcUeD2D : Received CID request for Traffic [ " << "Source: "
       << IPv4Address(lteInfo->getSrcAddr()) << "@" << lteInfo->getSrcPort()
       << " Destination: " << destAddr << "@" << lteInfo->getDstPort()
       << " , Direction: " << dirToA((Direction)lteInfo->getDirection()) << " ]\n";

    /*
     * Different lcid for different directions of the same flow are assigned.
     * RLC layer will create different RLC entities for different LCIDs
     */

    LogicalCid mylcid;
    if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
        lteInfo->getSrcPort(), lteInfo->getDstPort(), lteInfo->getDirection())) == 0xFFFF)
    {
        // LCID not found

        // assign a new LCID to the connection
        mylcid = lcid_++;

        EV << "LtePdcpRrcUeD2D : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
            lteInfo->getSrcPort(), lteInfo->getDstPort(), lteInfo->getDirection(), mylcid);
    }

    EV << "LtePdcpRrcUeD2D : Assigned Lcid: " << mylcid << "\n";
    EV << "LtePdcpRrcUeD2D : Assigned Node ID: " << nodeId_ << "\n";

    // get the PDCP entity for this LCID
    LtePdcpEntity* entity = getEntity(mylcid);

    // get the sequence number for this PDCP SDU.
    // Note that the numbering depends on the entity the packet is associated to.
    unsigned int sno = entity->nextSequenceNumber();

    // set sequence number
    lteInfo->setSequenceNumber(sno);
    // set some flow-related info
    lteInfo->setLcid(mylcid);
    lteInfo->setSourceId(nodeId_);
    if (lteInfo->getDirection() == D2D)
        lteInfo->setDestId(destId);
    else if (lteInfo->getDirection() == D2D_MULTI)
        lteInfo->setDestId(nodeId_);             // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
    else // UL
        lteInfo->setDestId(getDestId(lteInfo));

    // PDCP Packet creation
    LtePdcpPdu* pdcpPkt = new LtePdcpPdu("LtePdcpPdu");
    pdcpPkt->setByteLength(lteInfo->getRlcType() == UM ? PDCP_HEADER_UM : PDCP_HEADER_AM);
    pdcpPkt->encapsulate(pkt);
    pdcpPkt->setControlInfo(lteInfo);

    EV << "LtePdcp : Preparing to send "
       << lteTrafficClassToA((LteTrafficClass) lteInfo->getTraffic())
       << " traffic\n";
    EV << "LtePdcp : Packet size " << pdcpPkt->getByteLength() << " Bytes\n";
    EV << "LtePdcp : Sending packet " << pdcpPkt->getName() << " on port "
       << (lteInfo->getRlcType() == UM ? "UM_Sap$o\n" : "AM_Sap$o\n");

    // Send message
    send(pdcpPkt, (lteInfo->getRlcType() == UM ? umSap_[OUT] : amSap_[OUT]));
    emit(sentPacketToLowerLayer, pdcpPkt);
}

void LtePdcpRrcUeD2D::initialize(int stage)
{
    EV << "LtePdcpRrcUeD2D::initialize() - stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL)
        LtePdcpRrcBase::initialize();
    if (stage == INITSTAGE_NETWORK_LAYER_3+1)
    {
        // inform the Binder about the D2D capabilities of this node
        // i.e. the (possibly) D2D peering UEs
        const char *d2dPeerAddresses = getAncestorPar("d2dPeerAddresses");
        cStringTokenizer tokenizer(d2dPeerAddresses);
        const char *token;
        while ((token = tokenizer.nextToken()) != NULL)
        {
            IPv4Address d2dPeerAddr = L3AddressResolver().resolve(token).toIPv4();
            MacNodeId d2dPeerId = binder_->getMacNodeId(d2dPeerAddr);
            binder_->addD2DCapability(nodeId_, d2dPeerId);
        }
    }
}

void LtePdcpRrcUeD2D::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);

    // check whether the message is a notification for mode switch
    if (strcmp(pkt->getName(),"D2DModeSwitchNotification") == 0)
    {
        EV << "LtePdcpRrcUeD2D::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

        D2DModeSwitchNotification* switchPkt = check_and_cast<D2DModeSwitchNotification*>(pkt);

        // call handler
        pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());

        delete pkt;
    }
    else
    {
        LtePdcpRrcBase::handleMessage(msg);
    }
}

void LtePdcpRrcUeD2D::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LtePdcpRrcUeD2D::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;

    // add here specific behavior for handling mode switch at the PDCP layer
}

