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
            txbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection());

            // forward packet to PDCP
            EV << "LteRlcUmD2D::handleLowerMessage - Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
            send(pkt, up_[OUT]);
        }
        else  // rx side
        {
            // get the corresponding Rx buffer & call handler
            UmRxEntity* rxbuf = getRxBuffer(lteInfo);
            rxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getOldMode());

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
