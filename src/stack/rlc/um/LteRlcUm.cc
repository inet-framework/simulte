//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteRlcUm.h"
#include "D2DModeSwitchNotification_m.h"

Define_Module(LteRlcUm);

UmTxQueue* LteRlcUm::getTxBuffer(MacNodeId nodeId, LogicalCid lcid)
{
    // Find TXBuffer for this CID
    MacCid cid = idToMacCid(nodeId, lcid);
    UmTxBuffers::iterator it = txBuffers_.find(cid);
    if (it == txBuffers_.end())
    {
        // Not found: create
        std::stringstream buf;
        // FIXME HERE

        buf << "UmTxQueue Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("lte.stack.rlc.UmTxQueue");
        UmTxQueue* txbuf = check_and_cast<UmTxQueue *>(
            moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        txBuffers_[cid] = txbuf;    // Add to tx_buffers map

        EV << "LteRlcUm : Added new UmTxBuffer: " << txbuf->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return txbuf;
    }
    else
    {
        // Found
        EV << "LteRlcUm : Using old UmTxBuffer: " << it->second->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

UmRxQueue* LteRlcUm::getRxBuffer(MacNodeId nodeId, LogicalCid lcid)
{
    // Find RXBuffer for this CID
    MacCid cid = idToMacCid(nodeId, lcid);
    UmRxBuffers::iterator it = rxBuffers_.find(cid);
    if (it == rxBuffers_.end())
    {
        // Not found: create
        std::stringstream buf;
        buf << "UmRxQueue Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("lte.stack.rlc.UmRxQueue");
        UmRxQueue* rxbuf = check_and_cast<UmRxQueue *>(
            moduleType->createScheduleInit(buf.str().c_str(), getParentModule()));
        rxBuffers_[cid] = rxbuf;    // Add to rx_buffers map

        EV << "LteRlcUm : Added new UmRxBuffer: " << rxbuf->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return rxbuf;
    }
    else
    {
        // Found
        EV << "LteRlcUm : Using old UmRxBuffer: " << it->second->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

void LteRlcUm::sendDefragmented(cPacket *pkt)
{
    Enter_Method("sendDefragmented()");                            // Direct Method Call
    take(pkt);                                                    // Take ownership

    EV << "LteRlcUm : Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
    send(pkt, up_[OUT]);
}

void LteRlcUm::sendFragmented(cPacket *pkt)
{
    Enter_Method("sendFragmented()");                            // Direct Method Call
    take(pkt);                                                    // Take ownership

    EV << "LteRlcUm : Sending packet " << pkt->getName() << " to port UM_Sap_down$o\n";
    send(pkt, down_[OUT]);
}

void LteRlcUm::sendToLowerLayer(cPacket *pkt)
{
    Enter_Method("sendToLowerLayer()");                            // Direct Method Call
    take(pkt);                                                    // Take ownership
    EV << "LteRlcUm : Sending packet " << pkt->getName() << " to port UM_Sap_down$o\n";
    send(pkt, down_[OUT]);
}

void LteRlcUm::handleUpperMessage(cPacket *pkt)
{
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());
//    UmTxQueue* txbuf = getTxBuffer(lteInfo->getDestId(),lteInfo->getLcid());
    UmTxQueue* txbuf = getTxBuffer(ctrlInfoToUeId(lteInfo), lteInfo->getLcid());

    // Create a new RLC packet
    LteRlcSdu* rlcPkt = new LteRlcSdu("rlcUmPkt");
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setByteLength(RLC_HEADER_UM);
    rlcPkt->encapsulate(pkt);
    rlcPkt->setControlInfo(lteInfo);
    drop(rlcPkt);

    // Fragment Packet
    txbuf->fragment(rlcPkt);
}

void LteRlcUm::handleLowerMessage(cPacket *pkt)
{
    if (strcmp(pkt->getName(), "D2DModeSwitchNotification") == 0)
    {
        EV << NOW << " LteRlcUm::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";
        // add here specific behavior for handling mode switch at the RLC layer

        // forward packet to PDCP
        EV << "LteRlcUm::handleLowerMessage - Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
        send(pkt, up_[OUT]);
    }
    else
    {
        // Extract informations from fragment
        FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());
        MacNodeId nodeId = (lteInfo->getDirection() == DL) ? lteInfo->getDestId() : lteInfo->getSourceId();
        UmRxQueue* rxbuf = getRxBuffer(nodeId, lteInfo->getLcid());
        drop(pkt);

        // Defragment packet
        rxbuf->defragment(pkt);
    }
}

void LteRlcUm::deleteQueues(MacNodeId nodeId)
{
    UmTxBuffers::iterator tit;
    UmRxBuffers::iterator rit;

    for (tit = txBuffers_.begin(); tit != txBuffers_.end(); )
    {
        if (MacCidToNodeId(tit->first) == nodeId)
        {
            delete tit->second;           // Delete Queue
            txBuffers_.erase(tit++);        // Delete Elem
        }
        else
        {
            ++tit;
        }
    }
    for (rit = rxBuffers_.begin(); rit != rxBuffers_.end(); )
    {
        if (MacCidToNodeId(rit->first) == nodeId)
        {
            delete rit->second;           // Delete Queue
            rxBuffers_.erase(rit++);        // Delete Elem
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

    WATCH_MAP(txBuffers_);
    WATCH_MAP(rxBuffers_);
}

void LteRlcUm::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LteRlcUm : Received packet " << pkt->getName() <<
    " from port " << pkt->getArrivalGate()->getName() << endl;

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
