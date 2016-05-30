//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LtePdcpRrcUeD2D.h"
#include "IPvXAddressResolver.h"

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
    MacNodeId destId = binder_->getMacNodeId(destAddr);

    // set direction based on the destination Id. If the destination can be reached
    // using D2D, set D2D direction. Otherwise, set UL direction
    lteInfo->setDirection(getDirection(destId));

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

void LtePdcpRrcUeD2D::initialize()
{
    EV << "LtePdcpRrcUeD2D::initialize()" << endl;
    LtePdcpRrcBase::initialize();
}

void LtePdcpRrcUeD2D::handleMessage(cMessage* msg)
{
    LtePdcpRrcBase::handleMessage(msg);
}
