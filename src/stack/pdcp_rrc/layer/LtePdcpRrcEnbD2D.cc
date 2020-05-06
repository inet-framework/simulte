//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/pdcp_rrc/layer/LtePdcpRrcEnbD2D.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "stack/d2dModeSelection/D2DModeSwitchNotification_m.h"

Define_Module(LtePdcpRrcEnbD2D);

/*
 * Upper Layer handlers
 */
void LtePdcpRrcEnbD2D::fromDataPort(cPacket *pkt)
{
    emit(receivedPacketFromUpperLayer, pkt);

    // Control Informations
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());
    setTrafficInformation(pkt, lteInfo);
    headerCompress(pkt, lteInfo->getHeaderSize()); // header compression

    // get source info
    IPv4Address srcAddr = IPv4Address(lteInfo->getSrcAddr());
    // get destination info
    IPv4Address destAddr = IPv4Address(lteInfo->getDstAddr());
    MacNodeId srcId, destId;


    // set direction based on the destination Id. If the destination can be reached
    // using D2D, set D2D direction. Otherwise, set UL direction
    srcId = binder_->getMacNodeId(srcAddr);
    destId = binder_->getMacNodeId(destAddr);
    lteInfo->setDirection(getDirection());

    // check if src and dest of the flow are D2D-capable (currently in IM)
    if (getNodeTypeById(srcId) == UE && getNodeTypeById(destId) == UE && binder_->getD2DCapability(srcId, destId))
    {
        // this way, we record the ID of the endpoint even if the connection is in IM
        // this is useful for mode switching
        lteInfo->setD2dTxPeerId(srcId);
        lteInfo->setD2dRxPeerId(destId);
    }
    else
    {
        lteInfo->setD2dTxPeerId(0);
        lteInfo->setD2dRxPeerId(0);
    }

    // Cid Request
    EV << NOW << " LtePdcpRrcEnbD2D : Received CID request for Traffic [ " << "Source: "
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

        EV << "LtePdcpRrcEnbD2D : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
            lteInfo->getSrcPort(), lteInfo->getDstPort(), lteInfo->getDirection(), mylcid);
    }

    EV << "LtePdcpRrcEnbD2D : Assigned Lcid: " << mylcid << "\n";
    EV << "LtePdcpRrcEnbD2D : Assigned Node ID: " << nodeId_ << "\n";

    // get the PDCP entity for this LCID
    LtePdcpEntity* entity = getEntity(mylcid);

    // get the sequence number for this PDCP SDU.
    // Note that the numbering depends on the entity the packet is associated to.
    unsigned int sno = entity->nextSequenceNumber();

    // set sequence number
    lteInfo->setSequenceNumber(sno);
    // NOTE setLcid and setSourceId have been anticipated for using in "ctrlInfoToMacCid" function
    lteInfo->setLcid(mylcid);
    lteInfo->setSourceId(nodeId_);
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

void LtePdcpRrcEnbD2D::initialize(int stage)
{
    LtePdcpRrcEnb::initialize(stage);
}

void LtePdcpRrcEnbD2D::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);

    // check whether the message is a notification for mode switch
    if (strcmp(pkt->getName(),"D2DModeSwitchNotification") == 0)
    {
        EV << "LtePdcpRrcEnbD2D::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

        D2DModeSwitchNotification* switchPkt = check_and_cast<D2DModeSwitchNotification*>(pkt);

        // call handler
        pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());

        delete pkt;
    }
    else
    {
        LtePdcpRrcEnb::handleMessage(msg);
    }
}

void LtePdcpRrcEnbD2D::pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode)
{
    EV << NOW << " LtePdcpRrcEnbD2D::pdcpHandleD2DModeSwitch - peering with UE " << peerId << " set to " << d2dModeToA(newMode) << endl;

    // add here specific behavior for handling mode switch at the PDCP layer
}
