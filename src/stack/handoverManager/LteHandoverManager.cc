//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/handoverManager/LteHandoverManager.h"
#include "stack/handoverManager/X2HandoverCommandIE.h"

Define_Module(LteHandoverManager);

void LteHandoverManager::initialize()
{
    // get the node id
    nodeId_ = getAncestorPar("macCellId");

    // get reference to the gates
    x2Manager_[IN] = gate("x2ManagerIn");
    x2Manager_[OUT] = gate("x2ManagerOut");

    // get reference to the IP2lte layer
    ip2lte_ = check_and_cast<IP2lte*>(getParentModule()->getSubmodule("ip2lte"));

    losslessHandover_ = par("losslessHandover").boolValue();

    // register to the X2 Manager
    X2HandoverControlMsg* initMsg = new X2HandoverControlMsg();
    X2ControlInfo* ctrlInfo = new X2ControlInfo();
    ctrlInfo->setInit(true);
    initMsg->setControlInfo(ctrlInfo);
    send(PK(initMsg), x2Manager_[OUT]);
}

void LteHandoverManager::handleMessage(cMessage *msg)
{
    cPacket* pkt = check_and_cast<cPacket*>(msg);
    cGate* incoming = pkt->getArrivalGate();
    if (incoming == x2Manager_[IN])
    {
        // incoming data from X2 Manager
        EV << "LteHandoverManager::handleMessage - Received message from X2 manager" << endl;
        handleX2Message(pkt);
    }
    else
        delete msg;
}

void LteHandoverManager::handleX2Message(cPacket* pkt)
{
    LteX2Message* x2msg = check_and_cast<LteX2Message*>(pkt);
    X2NodeId sourceId = x2msg->getSourceId();

    if (x2msg->getType() == X2_HANDOVER_DATA_MSG)
    {
        X2HandoverDataMsg* hoDataMsg = check_and_cast<X2HandoverDataMsg*>(x2msg);
        IPv4Datagram* datagram = check_and_cast<IPv4Datagram*>(hoDataMsg->decapsulate());
        receiveDataFromSourceEnb(datagram, sourceId);
    }
    else   // X2_HANDOVER_CONTROL_MSG
    {
        X2HandoverControlMsg* hoCommandMsg = check_and_cast<X2HandoverControlMsg*>(x2msg);
        X2HandoverCommandIE* hoCommandIe = check_and_cast<X2HandoverCommandIE*>(hoCommandMsg->popIe());
        receiveHandoverCommand(hoCommandIe->getUeId(), hoCommandMsg->getSourceId(), hoCommandIe->isStartHandover());

        delete hoCommandIe;
    }

    delete x2msg;
}

void LteHandoverManager::sendHandoverCommand(MacNodeId ueId, MacNodeId enb, bool startHo)
{
    Enter_Method("sendHandoverCommand");

    EV<<NOW<<" LteHandoverManager::sendHandoverCommand - Send handover command over X2 to eNB " << enb << " for UE " << ueId << endl;

    // build control info
    X2ControlInfo* ctrlInfo = new X2ControlInfo();
    ctrlInfo->setSourceId(nodeId_);
    DestinationIdList destList;
    destList.push_back(enb);
    ctrlInfo->setDestIdList(destList);

    // build X2 Handover Msg
    X2HandoverCommandIE* hoCommandIe = new X2HandoverCommandIE();
    hoCommandIe->setUeId(ueId);
    if (startHo)
        hoCommandIe->setStartHandover();

    X2HandoverControlMsg* hoMsg = new X2HandoverControlMsg("X2HandoverControlMsg");
    hoMsg->pushIe(hoCommandIe);
    hoMsg->setControlInfo(ctrlInfo);

    // send to X2 Manager
    send(PK(hoMsg),x2Manager_[OUT]);
}

void LteHandoverManager::receiveHandoverCommand(MacNodeId ueId, MacNodeId enb, bool startHo)
{
    EV<<NOW<<" LteHandoverManager::receivedHandoverCommand - Received handover command over X2 from eNB " << enb << " for UE " << ueId << endl;

    // send command to IP2lte/PDCP
    if (startHo)
        ip2lte_->triggerHandoverTarget(ueId, enb);
    else
        ip2lte_->signalHandoverCompleteSource(ueId, enb);
}


void LteHandoverManager::forwardDataToTargetEnb(IPv4Datagram* datagram, MacNodeId targetEnb)
{
    Enter_Method("forwardDataToTargetEnb");
    take(datagram);

    // build control info
    X2ControlInfo* ctrlInfo = new X2ControlInfo();
    ctrlInfo->setSourceId(nodeId_);
    DestinationIdList destList;
    destList.push_back(targetEnb);
    ctrlInfo->setDestIdList(destList);

    // build X2 Handover Msg
    X2HandoverDataMsg* hoMsg = new X2HandoverDataMsg("X2HandoverDataMsg");
    hoMsg->encapsulate(datagram);
    hoMsg->setControlInfo(ctrlInfo);

    EV<<NOW<<" LteHandoverManager::forwardDataToTargetEnb - Send IP datagram to eNB " << targetEnb << endl;

    // send to X2 Manager
    send(PK(hoMsg),x2Manager_[OUT]);
}

void LteHandoverManager::receiveDataFromSourceEnb(IPv4Datagram* datagram, MacNodeId sourceEnb)
{
    EV<<NOW<<" LteHandoverManager::receiveDataFromSourceEnb - Received IP datagram from eNB " << sourceEnb << endl;

    // send data to IP2lte/PDCP for transmission
    ip2lte_->receiveTunneledPacketOnHandover(datagram, sourceEnb);
}

