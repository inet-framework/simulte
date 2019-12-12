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

using namespace omnetpp;

void LteRlcTm::handleUpperMessage(cPacket *pkt)
{
    take(pkt);
    emit(receivedPacketFromUpperLayer, pkt);

    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());

    // check if space is available or queue size is unlimited (queueSize_ is set to 0)
    if(queuedPdus_.getLength() >= queueSize_ && queueSize_ != 0){
        // cannot queue - queue is full
        EV << "LteRlcTm : Dropping packet " << pkt->getName() << " (queue full) \n";

        // statistics: packet was lost
        if (lteInfo->getDirection()==DL)
            emit(rlcPacketLossDl, 1.0);
        else
            emit(rlcPacketLossUl, 1.0);

        drop(pkt);
        delete pkt;
        delete lteInfo;

        return;
    }

    // build the PDU itself
    LteRlcSdu* rlcSduPkt = new LteRlcSdu("rlcTmPkt");
    LteRlcPdu* rlcPduPkt = new LteRlcPdu("rlcTmPkt");
    rlcSduPkt->encapsulate(pkt);
    rlcPduPkt->encapsulate(rlcSduPkt);
    rlcPduPkt->setControlInfo(lteInfo);
    // buffer the PDU
    queuedPdus_.insert(rlcPduPkt);

    // statistics: packet was not lost
    if (lteInfo->getDirection()==DL)
        emit(rlcPacketLossDl, 0.0);
    else
        emit(rlcPacketLossUl, 0.0);


    // create a message so as to notify the MAC layer that the queue contains new data
    LteRlcPdu* newDataPkt = new LteRlcPdu("newDataPkt");
    // make a copy of the RLC SDU
    LteRlcPdu* rlcPktDup = rlcPduPkt->dup();
    // the MAC will only be interested in the size of this packet
    newDataPkt->encapsulate(rlcPktDup);
    newDataPkt->setControlInfo(rlcPduPkt->getControlInfo()->dup());
    EV << "LteRlcTm::handleUpperMessage - Sending message " << newDataPkt->getName() << " to port AM_Sap_down$o\n";
    emit(sentPacketToLowerLayer,newDataPkt);
    send(newDataPkt, down_[OUT]);
}

void LteRlcTm::handleLowerMessage(cPacket *pkt)
{
    emit(receivedPacketFromLowerLayer, pkt);

    if (strcmp(pkt->getName(), "LteMacSduRequest") == 0) {
        if(queuedPdus_.getLength() > 0){
            auto rlcPduPkt = queuedPdus_.pop();
            EV << "LteRlcTm : Received " << pkt->getName() << " - sending packet " << rlcPduPkt->getName() << " to port TM_Sap_down$o\n";
            emit(sentPacketToLowerLayer,pkt);
            drop (rlcPduPkt);

            send(rlcPduPkt, down_[OUT]);
        } else
            EV << "LteRlcTm : Received " << pkt->getName() << " but no PDUs buffered - nothing to send to MAC.\n";

    } else {
        FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->removeControlInfo());
        cPacket* upPkt = check_and_cast<cPacket *>(pkt->decapsulate());
        cPacket* upUpPkt = check_and_cast<cPacket *>(upPkt->decapsulate());
        upUpPkt->setControlInfo(lteInfo);
        delete upPkt;

        EV << "LteRlcTm : Sending packet " << upUpPkt->getName() << " to port TM_Sap_up$o\n";
        emit(sentPacketToUpperLayer, upUpPkt);
        send(upUpPkt, up_[OUT]);
    }

    drop(pkt);
    delete pkt;
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

    queueSize_ = par("queueSize");

    // statistics
    receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
    receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
    sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
    sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");
    rlcPacketLossDl = registerSignal("rlcPacketLossDl");
    rlcPacketLossDl = registerSignal("rlcPacketLossUl");
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
