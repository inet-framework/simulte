//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/compManager/LteCompManagerBase.h"


void LteCompManagerBase::initialize()
{
    // get the node id
    nodeId_ = getAncestorPar("macCellId");

    // get reference to the gates
    x2Manager_[IN] = gate("x2ManagerIn");
    x2Manager_[OUT] = gate("x2ManagerOut");

    // get reference to mac layer
    mac_ = check_and_cast<LteMacEnb*>(getParentModule()->getSubmodule("mac"));

    // get the number of available bands
    numBands_ = mac_->getDeployer()->getNumBands();

    const char* nodeType = par("compNodeType").stringValue();
    if (strcmp(nodeType,"COMP_CLIENT") == 0)
        nodeType_ = COMP_CLIENT;
    else if (strcmp(nodeType,"COMP_CLIENT_COORDINATOR") == 0)
        nodeType_ = COMP_CLIENT_COORDINATOR;
    else if (strcmp(nodeType,"COMP_COORDINATOR") == 0)
        nodeType_ = COMP_COORDINATOR;
    else
        throw cRuntimeError("LteCompManagerBase::initialize - Unrecognized node type %s", nodeType);

    // register to the X2 Manager
    X2CompMsg* initMsg = new X2CompMsg();
    X2ControlInfo* ctrlInfo = new X2ControlInfo();
    ctrlInfo->setInit(true);
    initMsg->setControlInfo(ctrlInfo);
    send(PK(initMsg), x2Manager_[OUT]);

    if (nodeType_ != COMP_CLIENT)
    {
        // get the list of slave nodes
        std::vector<int> clients = cStringTokenizer(par("clientList").stringValue()).asIntVector();
        clientList_.resize(clients.size());
        for (unsigned int i=0; i<clients.size(); i++)
            clientList_[i] = clients[i];

        // get coordination period
        coordinationPeriod_ = par("coordinationPeriod").doubleValue();
        if (coordinationPeriod_ < TTI)
            coordinationPeriod_ = TTI;

        /* Start coordinator tick */
        compCoordinatorTick_ = new cMessage("compCoordinatorTick_");
        compCoordinatorTick_->setSchedulingPriority(3);        // compCoordinatorTick_ after slaves' TTI TICK. TODO check if it must be done before or after..
        scheduleAt(NOW + coordinationPeriod_, compCoordinatorTick_);
    }

    if (nodeType_ != COMP_COORDINATOR)
    {
        // statistics
        compReservedBlocks_ = registerSignal("compReservedBlocks");

        coordinatorId_ = par("coordinatorId");

        /* Start TTI tick */
        compClientTick_ = new cMessage("compClientTick_");
        compClientTick_->setSchedulingPriority(2);        // compClientTick_ after MAC's TTI TICK. TODO check if it must be done before or after..
        scheduleAt(NOW + TTI, compClientTick_);
    }
}

void LteCompManagerBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (strcmp(msg->getName(),"compClientTick_") == 0)
        {
            runClientOperations();
            scheduleAt(NOW+TTI, msg);
        }
        else if (strcmp(msg->getName(),"compCoordinatorTick_") == 0)
        {
            runCoordinatorOperations();
            scheduleAt(NOW+coordinationPeriod_, msg);
        }
        else
            throw cRuntimeError("LteCompManagerBase::handleMessage - Unrecognized self message %s", msg->getName());
    }
    else
    {
        cPacket* pkt = check_and_cast<cPacket*>(msg);
        cGate* incoming = pkt->getArrivalGate();
        if (incoming == x2Manager_[IN])
        {
            // incoming data from X2 Manager
            EV << "LteCompManagerBase::handleMessage - Received message from X2 manager" << endl;
            handleX2Message(pkt);
        }
        else
            delete msg;
    }
}

void LteCompManagerBase::runClientOperations()
{
    EV << "LteCompManagerBase::runClientOperations - node " << nodeId_ << endl;
    provisionalSchedule();
    X2CompRequestIE* requestIe = buildClientRequest();
    sendClientRequest(requestIe);
}

void LteCompManagerBase::runCoordinatorOperations()
{
    EV << "LteCompManagerBase::runCoordinatorOperations - node " << nodeId_ << endl;
    doCoordination();

    // for each client, send the appropriate reply
    std::vector<X2NodeId>::iterator cit = clientList_.begin();
    for (; cit != clientList_.end(); ++cit)
    {
        X2NodeId clientId = *cit;
        X2CompReplyIE* replyIe = buildCoordinatorReply(clientId);
        sendCoordinatorReply(clientId, replyIe);
    }

    if (nodeType_ == COMP_CLIENT_COORDINATOR)
    {
        // local reply
        X2CompReplyIE* replyIe = buildCoordinatorReply(nodeId_);
        sendCoordinatorReply(nodeId_, replyIe);
    }
}

void LteCompManagerBase::handleX2Message(cPacket* pkt)
{
    X2CompMsg* compMsg = check_and_cast<X2CompMsg*>(pkt);
    X2NodeId sourceId = compMsg->getSourceId();

    if (nodeType_ == COMP_COORDINATOR || nodeType_ == COMP_CLIENT_COORDINATOR)
    {
        // this is a request from a client
        handleClientRequest(compMsg);
    }
    else
    {
        // this is a reply from coordinator
        if (sourceId != coordinatorId_)
            throw cRuntimeError("LteCompManagerBase::handleX2Message - Sender is not the coordinator");

        handleCoordinatorReply(compMsg);
    }

    delete compMsg;
}

void LteCompManagerBase::sendClientRequest(X2CompRequestIE* requestIe)
{
    // build control info
    X2ControlInfo* ctrlInfo = new X2ControlInfo();
    ctrlInfo->setSourceId(nodeId_);
    DestinationIdList destList;
    destList.push_back(coordinatorId_);
    ctrlInfo->setDestIdList(destList);

    // build X2 Comp Msg
    X2CompMsg* compMsg = new X2CompMsg("X2CompMsg");
    compMsg->pushIe(requestIe);
    compMsg->setControlInfo(ctrlInfo);

    EV<<NOW<<" LteCompManagerBase::sendCompRequest - Send CoMP request" << endl;

    if (nodeType_ == COMP_CLIENT_COORDINATOR)
    {
        compMsg->setSourceId(nodeId_);
        handleClientRequest(compMsg);
    }
    else
    {
        // send to X2 Manager
        send(PK(compMsg),x2Manager_[OUT]);
    }
}

void LteCompManagerBase::sendCoordinatorReply(X2NodeId clientId, X2CompReplyIE* replyIe)
{
    // build control info
    X2ControlInfo* ctrlInfo = new X2ControlInfo();
    ctrlInfo->setSourceId(nodeId_);
    DestinationIdList destList;
    destList.push_back(clientId);
    ctrlInfo->setDestIdList(destList);

    // build X2 Comp Msg
    X2CompMsg* compMsg = new X2CompMsg("X2CompMsg");
    compMsg->pushIe(replyIe);
    compMsg->setControlInfo(ctrlInfo);

    if (clientId == nodeId_)
    {
         if (nodeType_ != COMP_CLIENT_COORDINATOR)
             throw cRuntimeError("LteCompManagerBase::sendCoordinatorReply - Node %d cannot sends reply to itself, since it is not the coordinator", clientId);

        compMsg->setSourceId(nodeId_);
        handleCoordinatorReply(compMsg);
    }
    else
    {
        // send to X2 Manager
        send(PK(compMsg),x2Manager_[OUT]);
    }
}


void LteCompManagerBase::setUsableBands(UsableBands& usableBands)
{
//    // update usableBands only if there is the new vector contains at least one element
//    if (usableBands.size() > 0)
//        usableBands_ = usableBands;

    usableBands_ = usableBands;
    mac_->getAmc()->setPilotUsableBands(nodeId_, usableBands_);
}

