//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/um/LteRlcUm.h"
#include "stack/mac/packet/LteMacSduRequest.h"

Define_Module(LteRlcUm);

UmTxEntity* LteRlcUm::getTxBuffer(FlowControlInfo* lteInfo)
{
    MacNodeId nodeId = ctrlInfoToUeId(lteInfo);
    LogicalCid lcid = lteInfo->getLcid();

    // Find TXBuffer for this CID
    MacCid cid = idToMacCid(nodeId, lcid);
    UmTxEntities::iterator it = txEntities_.find(cid);
    if (it == txEntities_.end())
    {
        // Not found: create
        std::stringstream buf;
        // FIXME HERE

        buf << "UmTxEntity Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("lte.stack.rlc.UmTxEntity");
        UmTxEntity* txEnt = check_and_cast<UmTxEntity *>(moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        txEntities_[cid] = txEnt;    // Add to tx_entities map

        if (lteInfo != NULL)
        {
            // store control info for this flow
            txEnt->setFlowControlInfo(lteInfo->dup());
        }

        EV << "LteRlcUm : Added new UmTxEntity: " << txEnt->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return txEnt;
    }
    else
    {
        // Found
        EV << "LteRlcUm : Using old UmTxBuffer: " << it->second->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}


UmRxEntity* LteRlcUm::getRxBuffer(FlowControlInfo* lteInfo)
{
    MacNodeId nodeId;
    if (lteInfo->getDirection() == DL)
        nodeId = lteInfo->getDestId();
    else
        nodeId = lteInfo->getSourceId();
    LogicalCid lcid = lteInfo->getLcid();

    // Find RXBuffer for this CID
    MacCid cid = idToMacCid(nodeId, lcid);

    UmRxEntities::iterator it = rxEntities_.find(cid);
    if (it == rxEntities_.end())
    {
        // Not found: create
        std::stringstream buf;
        buf << "UmRxEntity Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("lte.stack.rlc.UmRxEntity");
        UmRxEntity* rxEnt = check_and_cast<UmRxEntity *>(
            moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        rxEntities_[cid] = rxEnt;    // Add to rx_entities map

        // store control info for this flow
        rxEnt->setFlowControlInfo(lteInfo->dup());

        EV << "LteRlcUm : Added new UmRxEntity: " << rxEnt->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return rxEnt;
    }
    else
    {
        // Found
        EV << "LteRlcUm : Using old UmRxEntity: " << it->second->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

void LteRlcUm::sendDefragmented(cPacket *pkt)
{
    Enter_Method_Silent("sendDefragmented()");                    // Direct Method Call
    take(pkt);                                                    // Take ownership

    EV << "LteRlcUm : Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
    send(pkt, up_[OUT]);

    emit(sentPacketToUpperLayer, pkt);
}

void LteRlcUm::sendToLowerLayer(cPacket *pkt)
{
    Enter_Method_Silent("sendToLowerLayer()");                    // Direct Method Call
    take(pkt);                                                    // Take ownership
    EV << "LteRlcUm : Sending packet " << pkt->getName() << " to port UM_Sap_down$o\n";
    send(pkt, down_[OUT]);

    emit(sentPacketToLowerLayer, pkt);
}

void LteRlcUm::handleUpperMessage(cPacket *pkt)
{
    EV << "LteRlcUm::handleUpperMessage - Received packet " << pkt->getName() << " from upper layer, size " << pkt->getByteLength() << "\n";

    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());

    UmTxEntity* txbuf = getTxBuffer(lteInfo);

    // Create a new RLC packet
    LteRlcSdu* rlcPkt = new LteRlcSdu("rlcUmPkt");
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setLengthMainPacket(pkt->getByteLength());
    rlcPkt->encapsulate(pkt);
    rlcPkt->setControlInfo(lteInfo);

    drop(rlcPkt);

    if (txbuf->isHoldingDownstreamInPackets())
    {
        // do not store in the TX buffer and do not signal the MAC layer
        EV << "LteRlcUm::handleUpperMessage - Enque packet " << rlcPkt->getName() << " into the Holding Buffer\n";
        txbuf->enqueHoldingPackets(rlcPkt);
    }
    else
    {
        // create a message so as to notify the MAC layer that the queue contains new data
        LteRlcPdu* newDataPkt = new LteRlcPdu("newDataPkt");
        // make a copy of the RLC SDU
        LteRlcSdu* rlcPktDup = rlcPkt->dup();
        // the MAC will only be interested in the size of this packet
        newDataPkt->encapsulate(rlcPktDup);
        newDataPkt->setControlInfo(lteInfo->dup());

        EV << "LteRlcUm::handleUpperMessage - Sending message " << newDataPkt->getName() << " to port UM_Sap_down$o\n";
        send(newDataPkt, down_[OUT]);

        // Bufferize RLC SDU
        EV << "LteRlcUm::handleUpperMessage - Enque packet " << rlcPkt->getName() << " into the Tx Buffer\n";
        txbuf->enque(rlcPkt);

    }

    emit(receivedPacketFromUpperLayer, pkt);
}

void LteRlcUm::handleLowerMessage(cPacket *pkt)
{
    EV << "LteRlcUm::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";

    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

    if (strcmp(pkt->getName(), "LteMacSduRequest") == 0)
    {
        // get the corresponding Tx buffer
        UmTxEntity* txbuf = getTxBuffer(lteInfo);

        LteMacSduRequest* macSduRequest = check_and_cast<LteMacSduRequest*>(pkt);
        unsigned int size = macSduRequest->getSduSize();

        drop(pkt);

        // do segmentation/concatenation and send a pdu to the lower layer
        txbuf->rlcPduMake(size);

        delete macSduRequest;
    }
    else
    {
        emit(receivedPacketFromLowerLayer, pkt);

        // Extract informations from fragment
        UmRxEntity* rxbuf = getRxBuffer(lteInfo);
        drop(pkt);

        // Bufferize PDU
        EV << "LteRlcUm::handleLowerMessage - Enque packet " << pkt->getName() << " into the Rx Buffer\n";
        rxbuf->enque(pkt);
    }
}

void LteRlcUm::deleteQueues(MacNodeId nodeId)
{
    UmTxEntities::iterator tit;
    UmRxEntities::iterator rit;

    LteNodeType nodeType;
    std::string nodeTypePar = getAncestorPar("nodeType").stdstringValue();
    if (strcmp(nodeTypePar.c_str(), "ENODEB") == 0)
        nodeType = ENODEB;
    else
        nodeType = UE;

    // at the UE, delete all connections
    // at the eNB, delete connections related to the given UE
    for (tit = txEntities_.begin(); tit != txEntities_.end(); )
    {
        if (nodeType == UE || (nodeType == ENODEB && MacCidToNodeId(tit->first) == nodeId))
        {
            delete tit->second;        // Delete Entity
            txEntities_.erase(tit++);    // Delete Elem
        }
        else
        {
            ++tit;
        }
    }
    for (rit = rxEntities_.begin(); rit != rxEntities_.end(); )
    {
        if (nodeType == UE || (nodeType == ENODEB && MacCidToNodeId(rit->first) == nodeId))
        {
            delete rit->second;        // Delete Entity
            rxEntities_.erase(rit++);    // Delete Elem
        }
        else
        {
            ++rit;
        }
    }
}

/*
 * Main functions
 */

void LteRlcUm::initialize()
{
    up_[IN] = gate("UM_Sap_up$i");
    up_[OUT] = gate("UM_Sap_up$o");
    down_[IN] = gate("UM_Sap_down$i");
    down_[OUT] = gate("UM_Sap_down$o");

    // statistics
    receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
    receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
    sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
    sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

    WATCH_MAP(txEntities_);
    WATCH_MAP(rxEntities_);
}

void LteRlcUm::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LteRlcUm : Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();
    if (incoming == up_[IN])
    {
        handleUpperMessage(pkt);
    }
    else if (incoming == down_[IN])
    {
        handleLowerMessage(pkt);
    }
    return;
}
