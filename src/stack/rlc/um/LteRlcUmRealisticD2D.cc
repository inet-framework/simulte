//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteRlcUmRealisticD2D.h"
#include "D2DModeSwitchNotification_m.h"

Define_Module(LteRlcUmRealisticD2D);

void LteRlcUmRealisticD2D::handleMessage(cMessage *msg)
{
    LteRlcUmRealistic::handleMessage(msg);
}

void LteRlcUmRealisticD2D::handleLowerMessage(cPacket *pkt)
{
    if (strcmp(pkt->getName(), "D2DModeSwitchNotification") == 0)
    {
        EV << NOW << " LteRlcUmRealisticD2D::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";

        // add here specific behavior for handling mode switch at the RLC layer
        D2DModeSwitchNotification* switchPkt = check_and_cast<D2DModeSwitchNotification*>(pkt);
        FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(switchPkt->getControlInfo());

        if (switchPkt->getTxSide())
        {
            // get the corresponding Rx buffer & call handler
            UmTxEntity* txbuf = getTxBuffer(lteInfo);
            txbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection());

            // forward packet to PDCP
            EV << "LteRlcUmRealisticD2D::handleLowerMessage - Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
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
        LteRlcUmRealistic::handleLowerMessage(pkt);
}

void LteRlcUmRealisticD2D::initialize(int stage)
{
    if (stage == 0)
    {
        // check the MAC module type: if it is not "RealisticD2D", abort simulation
        std::string nodeType = getParentModule()->getParentModule()->par("nodeType").stdstringValue();
        std::string macType = getParentModule()->getParentModule()->par("LteMacType").stdstringValue();
        std::string pdcpType = getParentModule()->getParentModule()->par("LtePdcpRrcType").stdstringValue();

        if (nodeType.compare("ENODEB") == 0)
        {
            nodeType_ = ENODEB;
            if (macType.compare("LteMacEnbRealisticD2D") != 0)
                throw cRuntimeError("LteRlcUmRealisticD2D::initialize - %s module found, must be LteMacEnbRealisticD2D. Aborting", macType.c_str());
            if (pdcpType.compare("LtePdcpRrcEnbD2D") != 0)
                throw cRuntimeError("LteRlcUmRealisticD2D::initialize - %s module found, must be LtePdcpRrcEnbD2D. Aborting", pdcpType.c_str());
        }
        else if (nodeType.compare("UE") == 0)
        {
            nodeType_ = UE;
            if (macType.compare("LteMacUeRealisticD2D") != 0)
                throw cRuntimeError("LteRlcUmRealisticD2D::initialize - %s module found, must be LteMacUeRealisticD2D. Aborting", macType.c_str());
            if (pdcpType.compare("LtePdcpRrcUeD2D") != 0)
                throw cRuntimeError("LteRlcUmRealisticD2D::initialize - %s module found, must be LtePdcpRrcUeD2D. Aborting", pdcpType.c_str());
        }

        up_[IN] = gate("UM_Sap_up$i");
        up_[OUT] = gate("UM_Sap_up$o");
        down_[IN] = gate("UM_Sap_down$i");
        down_[OUT] = gate("UM_Sap_down$o");

        WATCH_MAP(txEntities_);
        WATCH_MAP(rxEntities_);
    }
}
