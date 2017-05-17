//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/tm/LteRlcTm.h"

Define_Module(LteRlcTm);

void LteRlcTm::handleUpperMessage(cPacket *pkt)
{
    LteRlcSdu* rlcSduPkt = new LteRlcSdu("rlcTmPkt");
    LteRlcPdu* rlcPduPkt = new LteRlcPdu("rlcTmPkt");
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());
    rlcSduPkt->encapsulate(pkt);
    rlcPduPkt->encapsulate(rlcSduPkt);
    rlcPduPkt->setControlInfo(lteInfo);

    EV << "LteRlcTm : Sending packet " << rlcPduPkt->getName() << " to port TM_Sap_down$o\n";
    send(rlcPduPkt, down_[OUT]);
}

void LteRlcTm::handleLowerMessage(cPacket *pkt)
{
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());
    cPacket* upPkt = check_and_cast<cPacket *>(pkt->decapsulate());
    cPacket* upUpPkt = check_and_cast<cPacket *>(upPkt->decapsulate());
    upUpPkt->setControlInfo(lteInfo);
    delete pkt;
    delete upPkt;

    EV << "LteRlcTm : Sending packet " << upUpPkt->getName() << " to port TM_Sap_up$o\n";
    send(upUpPkt, up_[OUT]);
}

/*
 * Main functions
 */

void LteRlcTm::initialize()
{
    up_[IN] = gate("TM_Sap_up$i");
    up_[OUT] = gate("TM_Sap_up$o");
    down_[IN] = gate("TM_Sap_down$i");
    down_[OUT] = gate("TM_Sap_down$o");
}

void LteRlcTm::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LteRlcTm : Received packet " << pkt->getName() <<
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
