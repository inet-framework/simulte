//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/um/LteRlcUmD2D.h"
#include "stack/d2dModeSelection/D2DModeSwitchNotification_m.h"

Define_Module(LteRlcUmD2D);

UmTxEntity* LteRlcUmD2D::getTxBuffer(FlowControlInfo* lteInfo)
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

        EV << "LteRlcUmD2D : Added new UmTxEntity: " << txEnt->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        // store per-peer map
        MacNodeId d2dPeer = lteInfo->getD2dRxPeerId();
        if (d2dPeer != 0)
            perPeerTxEntities_[d2dPeer].insert(txEnt);

        if (isEmptyingTxBuffer(d2dPeer))
            txEnt->startHoldingDownstreamInPackets();

        return txEnt;
    }
    else
    {
        // Found
        EV << "LteRlcUmD2D : Using old UmTxBuffer: " << it->second->getId() <<
        " for node: " << nodeId << " for Lcid: " << lcid << "\n";

        return it->second;
    }
}

void LteRlcUmD2D::handleLowerMessage(cPacket *pkt)
{
    if (strcmp(pkt->getName(), "D2DModeSwitchNotification") == 0)
    {
        EV << NOW << " LteRlcUmD2D::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";

        // add here specific behavior for handling mode switch at the RLC layer
        D2DModeSwitchNotification* switchPkt = check_and_cast<D2DModeSwitchNotification*>(pkt);
        FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(switchPkt->getControlInfo());

        if (switchPkt->getTxSide())
        {
            // get the corresponding Rx buffer & call handler
            UmTxEntity* txbuf = getTxBuffer(lteInfo);
            txbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getClearRlcBuffer());

            // forward packet to PDCP
            EV << "LteRlcUmD2D::handleLowerMessage - Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
            send(pkt, up_[OUT]);
        }
        else  // rx side
        {
            // get the corresponding Rx buffer & call handler
            UmRxEntity* rxbuf = getRxBuffer(lteInfo);
            rxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getOldMode(), switchPkt->getClearRlcBuffer());

            delete switchPkt;
        }
    }
    else
        LteRlcUm::handleLowerMessage(pkt);
}

void LteRlcUmD2D::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
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
}

void LteRlcUmD2D::resumeDownstreamInPackets(MacNodeId peerId)
{
    if (peerId == 0 || (perPeerTxEntities_.find(peerId) == perPeerTxEntities_.end()))
        return;

    std::set<UmTxEntity*>::iterator it = perPeerTxEntities_.at(peerId).begin();
    std::set<UmTxEntity*>::iterator et = perPeerTxEntities_.at(peerId).end();
    for (; it != et; ++it)
    {
        if ((*it)->isHoldingDownstreamInPackets())
            (*it)->resumeDownstreamInPackets();
    }
}

bool LteRlcUmD2D::isEmptyingTxBuffer(MacNodeId peerId)
{
    EV << NOW << " LteRlcUmD2D::isEmptyingTxBuffer - peerId " << peerId << endl;

    if (peerId == 0 || (perPeerTxEntities_.find(peerId) == perPeerTxEntities_.end()))
        return false;

    std::set<UmTxEntity*>::iterator it = perPeerTxEntities_.at(peerId).begin();
    std::set<UmTxEntity*>::iterator et = perPeerTxEntities_.at(peerId).end();
    for (; it != et; ++it)
    {
        if ((*it)->isEmptyingBuffer())
        {
            EV << NOW << " LteRlcUmD2D::isEmptyingTxBuffer - found " << endl;
            return true;
        }
    }
    return false;
}
