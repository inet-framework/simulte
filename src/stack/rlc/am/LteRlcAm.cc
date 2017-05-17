//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/am/LteRlcAm.h"
#include "common/LteCommon.h"
#include "stack/rlc/am/buffer/AmTxQueue.h"
#include "stack/rlc/am/buffer/AmRxQueue.h"

Define_Module(LteRlcAm);

AmTxQueue*
LteRlcAm::getTxBuffer(MacNodeId nodeId, LogicalCid lcid)
{
    // Find TXBuffer for this CID
    MacCid cid = idToMacCid(nodeId, lcid);
    AmTxBuffers::iterator it = txBuffers_.find(cid);

    if (it == txBuffers_.end())
    {
        // Not found: create
        std::stringstream buf;
        // FIXME HERE

        buf << "AmTxQueue Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("lte.stack.rlc.AmTxQueue");
        AmTxQueue* txbuf = check_and_cast<AmTxQueue *>(
            moduleType->createScheduleInit(buf.str().c_str(),
                getParentModule()));
        txBuffers_[cid] = txbuf; // Add to tx_buffers map

        EV << NOW << " LteRlcAm : Added new AmTxBuffer: " << txbuf->getId()
           << " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return txbuf;
    }
    else
    {
        // Found
        EV << NOW << " LteRlcAm : Using old AmTxBuffer: " << it->second->getId()
           << " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

AmRxQueue*
LteRlcAm::getRxBuffer(MacNodeId nodeId, LogicalCid lcid)
{
    // Find RXBuffer for this CID
    MacCid cid = idToMacCid(nodeId, lcid);

    AmRxBuffers::iterator it = rxBuffers_.find(cid);
    if (it == rxBuffers_.end())
    {
        // Not found: create
        std::stringstream buf;
        buf << "AmRxQueue Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("lte.stack.rlc.AmRxQueue");
        AmRxQueue* rxbuf = check_and_cast<AmRxQueue *>(
            moduleType->createScheduleInit(buf.str().c_str(),
                getParentModule()));
        rxBuffers_[cid] = rxbuf; // Add to rx_buffers map

        EV << NOW << " LteRlcAm : Added new AmRxBuffer: " << rxbuf->getId()
           << " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return rxbuf;
    }
    else
    {
        // Found
        EV << NOW << " LteRlcAm : Using old AmRxBuffer: " << it->second->getId()
           << " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

void LteRlcAm::sendDefragmented(cPacket *pkt)
{
    Enter_Method("sendDefragmented()"); // Direct Method Call
    take(pkt); // Take ownership

    EV << NOW << " LteRlcAm : Sending packet " << pkt->getName()
       << " to port AM_Sap_up$o\n";
    send(pkt, up_[OUT]);
}

void LteRlcAm::sendFragmented(cPacket *pkt)
{
    Enter_Method("sendFragmented()"); // Direct Method Call
    take(pkt); // Take ownership

    EV << NOW << " LteRlcAm : Sending packet " << pkt->getName() << " of size "
       << pkt->getByteLength() << "  to port AM_Sap_down$o\n";

    send(pkt, down_[OUT]);
}

void LteRlcAm::handleUpperMessage(cPacket *pkt)
{
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
        pkt->removeControlInfo());
//    AmTxQueue* txbuf = getTxBuffer(lteInfo->getDestId(), lteInfo->getLcid());
    AmTxQueue* txbuf = getTxBuffer(ctrlInfoToUeId(lteInfo), lteInfo->getLcid());

    // Create a new RLC packet
    LteRlcAmSdu* rlcPkt = new LteRlcAmSdu("rlcAmPkt");
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setByteLength(RLC_HEADER_AM);
    rlcPkt->encapsulate(pkt);
    rlcPkt->setControlInfo(lteInfo);
    drop(rlcPkt);
    EV << NOW << " LteRlcAm : handleUpperMessage sending to AM TX Queue" << endl;
    // Fragment Packet
    txbuf->enque(rlcPkt);
}

void LteRlcAm::routeControlMessage(cPacket *pkt)
{
    Enter_Method("routeControlMessage");

    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
        pkt->removeControlInfo());
//    AmTxQueue* txbuf = getTxBuffer(lteInfo->getSourceId(), lteInfo->getLcid());
    AmTxQueue* txbuf = getTxBuffer(ctrlInfoToUeId(lteInfo), lteInfo->getLcid());
    txbuf->handleControlPacket(pkt);
}

void LteRlcAm::handleLowerMessage(cPacket *pkt)
{
    LteRlcAmPdu* pdu = check_and_cast<LteRlcAmPdu*>(pkt);

    if ((pdu->getAmType() == ACK) || (pdu->getAmType() == MRW_ACK))
    {
        EV << NOW << " LteRlcAm::handleLowerMessage Received ACK message" << endl;

        // forwarding ACK to associated transmitting entity
        routeControlMessage(pdu);
        return;
    }

        // Extract informations from fragment
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(
        pkt->getControlInfo());
//    AmRxQueue* rxbuf = getRxBuffer(lteInfo->getDestId(), lteInfo->getLcid());
    AmRxQueue* rxbuf = getRxBuffer(ctrlInfoToUeId(lteInfo), lteInfo->getLcid());
    drop(pkt);

    EV << NOW << " LteRlcAm::handleLowerMessage sending packet to AM RX Queue " << endl;
    // Defragment packet
    rxbuf->enque(check_and_cast<LteRlcAmPdu*>(pkt));
}

void LteRlcAm::deleteQueues(MacNodeId nodeId)
{
    AmTxBuffers::iterator tit;
    AmRxBuffers::iterator rit;

    for (tit = txBuffers_.begin(); tit != txBuffers_.end(); )
    {
        if (MacCidToNodeId(tit->first) == nodeId)
        {
            delete tit->second; // Delete Queue
            txBuffers_.erase(tit++); // Delete Elem
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
            delete rit->second; // Delete Queue
            rxBuffers_.erase(rit++); // Delete Elem
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

void LteRlcAm::initialize()
{
    up_[IN] = gate("AM_Sap_up$i");
    up_[OUT] = gate("AM_Sap_up$o");
    down_[IN] = gate("AM_Sap_down$i");
    down_[OUT] = gate("AM_Sap_down$o");
}

void LteRlcAm::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << NOW << " LteRlcAm : Received packet " << pkt->getName() << " from port "
       << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();
    if (incoming == up_[IN])
    {
        EV << NOW << " LteRlcAm : calling handleUpperMessage" << endl;
        handleUpperMessage(pkt);
    }
    else if (incoming == down_[IN])
    {
        EV << NOW << " LteRlcAm : calling handleLowerMessage" << endl;
        handleLowerMessage(pkt);
    }
    return;
}
