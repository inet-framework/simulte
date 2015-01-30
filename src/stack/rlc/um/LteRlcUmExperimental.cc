//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteRlcUmExperimental.h"
#include "LteMacSduRequest.h"

Define_Module(LteRlcUmExperimental);

UmTxEntity* LteRlcUmExperimental::getTxBuffer(FlowControlInfo* lteInfo)
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

        EV << "LteRlcUmExperimental : Added new UmTxEntity: " << txEnt->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return txEnt;
    }
    else
    {
        // Found
        EV << "LteRlcUmExperimental : Using old UmTxBuffer: " << it->second->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}


UmRxEntity* LteRlcUmExperimental::getRxBuffer(FlowControlInfo* lteInfo)
{
    MacNodeId nodeId = ctrlInfoToUeId(lteInfo);
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

        EV << "LteRlcUmExperimental : Added new UmRxEntity: " << rxEnt->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return rxEnt;
    }
    else
    {
        // Found
        EV << "LteRlcUmExperimental : Using old UmRxEntity: " << it->second->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

void LteRlcUmExperimental::handleUpperMessage(cPacket *pkt)
{
    EV << "LteRlcUmExperimental::handleUpperMessage - Received packet " << pkt->getName() << " from upper layer, size " << pkt->getByteLength() << "\n";

    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());

    UmTxEntity* txbuf = getTxBuffer(lteInfo);

    // Create a new RLC packet
    LteRlcSdu* rlcPkt = new LteRlcSdu("rlcUmPkt");
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->encapsulate(pkt);

    // create a message so as to notify the MAC layer that the queue contains new data
    LteRlcPdu* newDataPkt = new LteRlcPdu("newDataPkt");
    // make a copy of the RLC SDU
    LteRlcSdu* rlcPktDup = rlcPkt->dup();
    // the MAC will only be interested in the size of this packet
    newDataPkt->encapsulate(rlcPktDup);
    newDataPkt->setControlInfo(lteInfo->dup());

    EV << "LteRlcUmExperimental::handleUpperMessage - Sending message " << newDataPkt->getName() << " to port UM_Sap_down$o\n";
    send(newDataPkt, down_[OUT]);

    rlcPkt->setControlInfo(lteInfo);

    drop(rlcPkt);

    // Bufferize RLC SDU
    EV << "LteRlcUmExperimental::handleUpperMessage - Enque packet " << rlcPkt->getName() << " into the Tx Buffer\n";
    txbuf->enque(rlcPkt);
}

void LteRlcUmExperimental::handleLowerMessage(cPacket *pkt)
{
    EV << "LteRlcUmExperimental::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";

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
        // Extract informations from fragment
        UmRxEntity* rxbuf = getRxBuffer(lteInfo);
        drop(pkt);

        // Bufferize PDU
        EV << "LteRlcUmExperimental::handleLowerMessage - Enque packet " << pkt->getName() << " into the Rx Buffer\n";
        rxbuf->enque(pkt);
    }
}

void LteRlcUmExperimental::deleteQueues(MacNodeId nodeId)
{
    UmTxEntities::iterator tit;
    UmRxEntities::iterator rit;

    for (tit = txEntities_.begin(); tit != txEntities_.end(); tit++)
    {
        if (MacCidToNodeId(tit->first) == nodeId)
        {
            delete tit->second;        // Delete Entity
            txEntities_.erase(tit);        // Delete Elem
        }
    }
    for (rit = rxEntities_.begin(); rit != rxEntities_.end(); rit++)
    {
        if (MacCidToNodeId(rit->first) == nodeId)
        {
            delete rit->second;        // Delete Entity
            rxEntities_.erase(rit);        // Delete Elem
        }
    }
}

/*
 * Main functions
 */

void LteRlcUmExperimental::initialize()
{
    // check the MAC module type: if it is not "experimental", abort simulation
    std::string nodeType = getParentModule()->getParentModule()->par("nodeType").stdstringValue();
    std::string macType = getParentModule()->getParentModule()->par("LteMacType").stdstringValue();

    if (nodeType.compare("ENODEB") == 0)
    {
        if (macType.compare("LteMacEnbExperimental") != 0)
            throw cRuntimeError("LteRlcUmExperimental::initialize - %s module found, must be LteMacEnbExperimental. Aborting", macType.c_str());
    }
    else if (nodeType.compare("UE") == 0)
    {
        if (macType.compare("LteMacUeExperimental") != 0)
            throw cRuntimeError("LteRlcUmExperimental::initialize - %s module found, must be LteMacUeExperimental. Aborting", macType.c_str());
    }

    up_[IN] = gate("UM_Sap_up$i");
    up_[OUT] = gate("UM_Sap_up$o");
    down_[IN] = gate("UM_Sap_down$i");
    down_[OUT] = gate("UM_Sap_down$o");

    WATCH_MAP(txEntities_);
    WATCH_MAP(rxEntities_);
}
