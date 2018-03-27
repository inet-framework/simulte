//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"

Define_Module(LtePdcpRrcUe);
Define_Module(LtePdcpRrcEnb);
Define_Module(LtePdcpRrcRelayUe);
Define_Module(LtePdcpRrcRelayEnb);

LtePdcpRrcBase::LtePdcpRrcBase()
{
    ht_ = new ConnectionsTable();
    lcid_ = 1;
}

LtePdcpRrcBase::~LtePdcpRrcBase()
{
    delete ht_;

    PdcpEntities::iterator it = entities_.begin();
    for (; it != entities_.end(); ++it)
    {
        delete it->second;
    }
    entities_.clear();
}

void LtePdcpRrcBase::headerCompress(cPacket* pkt, int headerSize)
{
    if (headerCompressedSize_ != -1)
    {
        pkt->setByteLength(
            pkt->getByteLength() - headerSize + headerCompressedSize_);
        EV << "LtePdcp : Header compression performed\n";
    }
}

void LtePdcpRrcBase::headerDecompress(cPacket* pkt, int headerSize)
{
    if (headerCompressedSize_ != -1)
    {
        pkt->setByteLength(
            pkt->getByteLength() + headerSize - headerCompressedSize_);
        EV << "LtePdcp : Header decompression performed\n";
    }
}

        /*
         * TODO
         * Osservando le porte tira fuori:
         * lteInfo->setApplication();
         * lteInfo->setDirection();
         * lteInfo->setTraffic();
         * lteInfo->setRlcType();
         */
void LtePdcpRrcBase::setTrafficInformation(cPacket* pkt,
    FlowControlInfo* lteInfo)
{
    if ((strcmp(pkt->getName(), "VoIP")) == 0)
    {
        lteInfo->setApplication(VOIP);
        lteInfo->setTraffic(CONVERSATIONAL);
        lteInfo->setRlcType((int) par("conversationalRlc"));
    }
    else if ((strcmp(pkt->getName(), "gaming")) == 0)
    {
        lteInfo->setApplication(GAMING);
        lteInfo->setTraffic(INTERACTIVE);
        lteInfo->setRlcType((int) par("interactiveRlc"));
    }
    else if ((strcmp(pkt->getName(), "VoDPacket") == 0)
        || (strcmp(pkt->getName(), "VoDFinishPacket") == 0))
    {
        lteInfo->setApplication(VOD);
        lteInfo->setTraffic(STREAMING);
        lteInfo->setRlcType((int) par("streamingRlc"));
    }
    else
    {
        lteInfo->setApplication(CBR);
        lteInfo->setTraffic(BACKGROUND);
        lteInfo->setRlcType((int) par("backgroundRlc"));
    }

    lteInfo->setDirection(getDirection());
}

/*
 * Upper Layer handlers
 */

void LtePdcpRrcBase::fromDataPort(cPacket *pkt)
{
    emit(receivedPacketFromUpperLayer, pkt);

    // Control Informations
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());

    setTrafficInformation(pkt, lteInfo);
    lteInfo->setDestId(getDestId(lteInfo));
    headerCompress(pkt, lteInfo->getHeaderSize()); // header compression

    // Cid Request
    EV << "LteRrc : Received CID request for Traffic [ " << "Source: "
       << IPv4Address(lteInfo->getSrcAddr()) << "@" << lteInfo->getSrcPort()
       << " Destination: " << IPv4Address(lteInfo->getDstAddr()) << "@"
       << lteInfo->getDstPort() << " ]\n";

    // TODO: Since IP addresses can change when we add and remove nodes, maybe node IDs should be used instead of them
    LogicalCid mylcid;
    if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
        lteInfo->getSrcPort(), lteInfo->getDstPort())) == 0xFFFF)
    {
        // LCID not found
        mylcid = lcid_++;

        EV << "LteRrc : Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(),
            lteInfo->getSrcPort(), lteInfo->getDstPort(), mylcid);
    }

    EV << "LteRrc : Assigned Lcid: " << mylcid << "\n";
    EV << "LteRrc : Assigned Node ID: " << nodeId_ << "\n";

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
    pdcpPkt->setByteLength(
        lteInfo->getRlcType() == UM ? PDCP_HEADER_UM : PDCP_HEADER_AM);
    pdcpPkt->encapsulate(pkt);

    EV << "LtePdcp : Preparing to send "
       << lteTrafficClassToA((LteTrafficClass) lteInfo->getTraffic())
       << " traffic\n";
    EV << "LtePdcp : Packet size " << pdcpPkt->getByteLength() << " Bytes\n";

    lteInfo->setSourceId(nodeId_);
    lteInfo->setLcid(mylcid);
    pdcpPkt->setControlInfo(lteInfo);

    EV << "LtePdcp : Sending packet " << pdcpPkt->getName() << " on port "
       << (lteInfo->getRlcType() == UM ? "UM_Sap$o\n" : "AM_Sap$o\n");

    // Send message
    send(pdcpPkt, (lteInfo->getRlcType() == UM ? umSap_[OUT] : amSap_[OUT]));
    emit(sentPacketToLowerLayer, pdcpPkt);
}

void LtePdcpRrcBase::fromEutranRrcSap(cPacket *pkt)
{
    // TODO For now use LCID 1000 for Control Traffic coming from RRC
    FlowControlInfo* lteInfo = new FlowControlInfo();
    lteInfo->setSourceId(nodeId_);
    lteInfo->setLcid(1000);
    lteInfo->setRlcType(TM);
    pkt->setControlInfo(lteInfo);
    EV << "LteRrc : Sending packet " << pkt->getName() << " on port TM_Sap$o\n";
    send(pkt, tmSap_[OUT]);
}

/*
 * Lower layer handlers
 */

void LtePdcpRrcBase::toDataPort(cPacket *pkt)
{
    emit(receivedPacketFromLowerLayer, pkt);
    LtePdcpPdu* pdcpPkt = check_and_cast<LtePdcpPdu*>(pkt);
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
        pdcpPkt->removeControlInfo());

    EV << "LtePdcp : Received packet with CID " << lteInfo->getLcid() << "\n";
    EV << "LtePdcp : Packet size " << pdcpPkt->getByteLength() << " Bytes\n";

    cPacket* upPkt = pdcpPkt->decapsulate(); // Decapsulate packet
    delete pdcpPkt;

    headerDecompress(upPkt, lteInfo->getHeaderSize()); // Decompress packet header
    handleControlInfo(upPkt, lteInfo);

    EV << "LtePdcp : Sending packet " << upPkt->getName()
       << " on port DataPort$o\n";
    // Send message
    send(upPkt, dataPort_[OUT]);
    emit(sentPacketToUpperLayer, upPkt);
}

void LtePdcpRrcBase::toEutranRrcSap(cPacket *pkt)
{
    cPacket* upPkt = pkt->decapsulate();
    delete pkt;

    EV << "LteRrc : Sending packet " << upPkt->getName()
       << " on port EUTRAN_RRC_Sap$o\n";
    send(upPkt, eutranRrcSap_[OUT]);
}

/*
 * Main functions
 */

void LtePdcpRrcBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        dataPort_[IN] = gate("DataPort$i");
        dataPort_[OUT] = gate("DataPort$o");
        eutranRrcSap_[IN] = gate("EUTRAN_RRC_Sap$i");
        eutranRrcSap_[OUT] = gate("EUTRAN_RRC_Sap$o");
        tmSap_[IN] = gate("TM_Sap$i");
        tmSap_[OUT] = gate("TM_Sap$o");
        umSap_[IN] = gate("UM_Sap$i");
        umSap_[OUT] = gate("UM_Sap$o");
        amSap_[IN] = gate("AM_Sap$i");
        amSap_[OUT] = gate("AM_Sap$o");

        binder_ = getBinder();
        headerCompressedSize_ = par("headerCompressedSize"); // Compressed size
        nodeId_ = getAncestorPar("macNodeId");

        // statistics
        receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
        receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
        sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
        sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

        // TODO WATCH_MAP(gatemap_);
        WATCH(headerCompressedSize_);
        WATCH(nodeId_);
        WATCH(lcid_);
    }
}

void LtePdcpRrcBase::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LtePdcp : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();
    if (incoming == dataPort_[IN])
    {
        fromDataPort(pkt);
    }
    else if (incoming == eutranRrcSap_[IN])
    {
        fromEutranRrcSap(pkt);
    }
    else if (incoming == tmSap_[IN])
    {
        toEutranRrcSap(pkt);
    }
    else
    {
        toDataPort(pkt);
    }
    return;
}

LtePdcpEntity* LtePdcpRrcBase::getEntity(LogicalCid lcid)
{
    // Find entity for this LCID
    PdcpEntities::iterator it = entities_.find(lcid);
    if (it == entities_.end())
    {
        // Not found: create
        LtePdcpEntity* ent = new LtePdcpEntity();
        entities_[lcid] = ent;    // Add to entities map

        EV << "LtePdcpRrcBase::getEntity - Added new PdcpEntity for Lcid: " << lcid << "\n";

        return ent;
    }
    else
    {
        // Found
        EV << "LtePdcpRrcBase::getEntity - Using old PdcpEntity for Lcid: " << lcid << "\n";

        return it->second;
    }

}

void LtePdcpRrcBase::finish()
{
    // TODO make-finish
}

void LtePdcpRrcEnb::initialize(int stage)
{
    LtePdcpRrcBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        nodeId_ = getAncestorPar("macNodeId");
}

void LtePdcpRrcUe::initialize(int stage)
{
    LtePdcpRrcBase::initialize(stage);
    if (stage == inet::INITSTAGE_NETWORK_LAYER_3)
    {
        nodeId_ = getAncestorPar("macNodeId");
    }
}
