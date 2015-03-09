//
//                           SimuLTE
// Copyright (C) 2015 Antonio Virdis, Giovanni Nardini, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteCompManager.h"

Define_Module(LteCompManager);

void LteCompManager::initialize()
{
    // get the node id
    nodeId_ = getAncestorPar("macCellId");

    // get reference to the gates
    x2Manager_[IN] = gate("x2ManagerIn");
    x2Manager_[OUT] = gate("x2ManagerOut");

    // get reference to mac layer
    mac_ = check_and_cast<LteMacEnb*>(getParentModule()->getSubmodule("mac"));

    // statistics
    compReservedBlocks_ = registerSignal("compReservedBlocks");

    // get the node type
    if (!strcmp(par("nodeType").stringValue(), "COMP_MASTER"))
    {
        type_ = COMP_MASTER;

        // get the list of slave nodes
        std::vector<int> slaves = cStringTokenizer(par("slavesList").stringValue()).asIntVector();
        slavesList_.resize(slaves.size());
        for (unsigned int i=0; i<slaves.size(); i++)
        {
            slavesList_[i] = slaves[i];

            // initialize corresponding entry in receivedRequests map
            receivedRequests_.insert(std::pair<X2NodeId,bool>(slavesList_[i], false));
        }
    }
    else if (!strcmp(par("nodeType").stringValue(), "COMP_SLAVE"))
    {
        type_ = COMP_SLAVE;
        masterId_ = par("masterId");
    }
    else
        throw cRuntimeError("LteCompManager::initialize - Unknown CoMP node type %s for eNB %d", par("nodeType").stringValue(), nodeId_);

    // register to the X2 Manager
    X2CompMsg* initMsg = new X2CompMsg();
    X2ControlInfo* ctrlInfo = new X2ControlInfo();
    ctrlInfo->setInit(true);
    initMsg->setControlInfo(ctrlInfo);
    send(PK(initMsg), x2Manager_[OUT]);

    /* Start TTI tick */
    ttiTick_ = new cMessage("ttiTick_");
    ttiTick_->setSchedulingPriority(2);        // TTI TICK after MAC's TTI TICK
    scheduleAt(NOW + TTI, ttiTick_);
}

void LteCompManager::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        handleSelfMessage();
        scheduleAt(NOW+TTI, msg);
        return;
    }

    cPacket* pkt = check_and_cast<cPacket*>(msg);
    cGate* incoming = pkt->getArrivalGate();
    if (incoming == x2Manager_[IN])
    {
        // incoming data from X2 Manager
        EV << "LteCompManager::handleMessage - Received message from X2 manager" << endl;

        if (type_ == COMP_MASTER)
        {
            handleRequestFromSlave(pkt);
        }
        else // COMP_SLAVE
        {
            handleReplyFromMaster(pkt);
        }
    }
    else
        delete msg;

}

void LteCompManager::handleSelfMessage()
{
    EV << "LteCompManager::handleSelfMessage - Main loop, node " << nodeId_ << endl;

    provisionalSchedule();
}

std::vector<unsigned int> LteCompManager::roundVector(std::vector<double>& vec, int sum)
{
    // the rounding algorithm needs that the vector is sorted in ascending order

    // we sort the vector, then put the elements in the original order
    // to do this, we use an auxiliary vector to remember the original positions

    unsigned int len = vec.size();
    std::vector<unsigned int> integerVec;
    integerVec.resize(len, 0);

    // auxiliary vector
    std::vector<unsigned int> auxVec;
    auxVec.resize(len, 0);
    for (unsigned int i=0; i<auxVec.size(); i++)
        auxVec[i] = i;

    // sort vector in ascending order
    bool swap = true;
    while(swap)
    {
        swap = false;
        for (unsigned int i=0; i< len-1; i++)
        {
            if (vec[i] > vec[i+1])
            {
                double tmp = vec[i+1];
                vec[i+1] = vec[i];
                vec[i] = tmp;

                unsigned int auxTmp = auxVec[i+1];
                auxVec[i+1] = auxVec[i];
                auxVec[i] = auxTmp;

                swap = true;
            }
        }
    }

    // round vector (the sum of the elements is preserved)
    int integerTot = 0;
    double doubleTot = 0;
    for (unsigned int i = 0; i < len; i++)
    {
        doubleTot += vec[i];
        vec[i] = (int)(doubleTot - integerTot);
        integerTot += vec[i];
    }

    // TODO -  check numerical errors
//    if (integerTot < sum)
//        vec[len-1]++;

    // set the integer vector with the elements in their original positions
    for (unsigned int i = 0; i < integerVec.size(); i++)
    {
        unsigned int index = auxVec[i];
        integerVec[index] = vec[i];
    }

    return integerVec;
}

UsableBands LteCompManager::parseAllowedBlocksMap(std::vector<CompRbStatus>& allowedBlocksMap)
{
    unsigned int reservedBlocks = 0;
    UsableBands usableBands;
    for (unsigned int b=0; b<allowedBlocksMap.size(); b++)
    {
        if (allowedBlocksMap[b] == AVAILABLE_RB)
        {
            // eNB is allowed to use this block
            usableBands.push_back(b);
            reservedBlocks++;
        }
    }

    emit(compReservedBlocks_, reservedBlocks);

    return usableBands;
}

void LteCompManager::setUsableBands(UsableBands& usableBands)
{
    // update usableBands only if there is the new vector contains at least one element
//    if (usableBands.size() > 0)
        usableBands_ = usableBands;

    mac_->getAmc()->setPilotUsableBands(nodeId_, usableBands_);
}

void LteCompManager::handleRequestFromSlave(cPacket* pkt)
{
    X2CompMsg* compMsg = check_and_cast<X2CompMsg*>(pkt);
    X2NodeId sourceId = compMsg->getSourceId();
    while (compMsg->hasIe())
    {
        X2InformationElement* ie = compMsg->popIe();
        if (ie->getType() != COMP_REQUEST_IE)
            throw cRuntimeError("LteCompManager::handleRequestFromSlave - Expected COMP_REQUEST_IE");

        X2CompRequestIE* requestIe = check_and_cast<X2CompRequestIE*>(ie);
        unsigned int reqBlocks = requestIe->getNumBlocks();

        // update map entry for this node
        if (reqBlocksMap_.find(sourceId) == reqBlocksMap_.end() )
            reqBlocksMap_.insert(std::pair<X2NodeId, unsigned int>(sourceId, reqBlocks));
        else
            reqBlocksMap_[sourceId] = reqBlocks;

        receivedRequests_[sourceId] = true;

        delete requestIe;
    }

    // check whether all slaves have sent their requests...
    bool doPart = true;
    std::map<X2NodeId, bool>::iterator it = receivedRequests_.begin();
    for (; it != receivedRequests_.end(); ++it)
    {
        if (!it->second)
        {
            doPart = false;
            break;
        }
    }

    // ..if yes, do bandwidth partitioning
    if (doPart)
    {
        doBandwidthPartitioning();

        // clear map
        for (it = receivedRequests_.begin(); it != receivedRequests_.end(); ++it)
            it->second = false;
    }

    delete compMsg;
}

void LteCompManager::handleReplyFromMaster(cPacket* pkt)
{
    X2CompMsg* compMsg = check_and_cast<X2CompMsg*>(pkt);
    X2NodeId sourceId = compMsg->getSourceId();
    if (sourceId != masterId_)
        throw cRuntimeError("LteCompManager::handleReplyFromMaster - Sender is not the master node");

    while (compMsg->hasIe())
    {
        X2InformationElement* ie = compMsg->popIe();
        if (ie->getType() != COMP_REPLY_IE)
            throw cRuntimeError("LteCompManager::handleReplyFromMaster - Expected COMP_REPLY_IE");

        X2CompReplyIE* replyIe = check_and_cast<X2CompReplyIE*>(ie);
        std::vector<CompRbStatus> allowedBlocksMap = replyIe->getAllowedBlocksMap();
        UsableBands usableBands = parseAllowedBlocksMap(allowedBlocksMap);
        setUsableBands(usableBands);

        delete replyIe;
    }

    delete compMsg;
}

void LteCompManager::provisionalSchedule()
{
    EV<<NOW<<" LteCompManager::provisionalSchedule - Start "<<endl;

    unsigned int totalReqBlocks = 0;
    Direction dir = DL;
    LteMacBufferMap* vbuf = mac_->getMacBuffers();
    ActiveSet activeSet = mac_->getActiveSet(dir);
    ActiveSet::iterator ait = activeSet.begin();
    for (; ait != activeSet.end(); ++ait)
    {
        MacCid cid = *ait;
        MacNodeId ueId = MacCidToNodeId(cid);

        unsigned int queueLength = vbuf->at(cid)->getQueueOccupancy();
//        std::cout<<NOW<<" LteCompManager::provisionalSchedule - nodo: "<<ueId<< " coda: "<<queueLength<<" bytes"<<endl;

        // Compute the number of bytes available in one block for this UE
        unsigned int bytesPerBlock = 0;
        unsigned int blocks = 1;
        Codeword cw = 0;
        Band band = 0;
        bytesPerBlock = mac_->getAmc()->computeBytesOnNRbs(ueId,band,cw,blocks,dir); // The index of the band is useless
        EV<<NOW<<" LteCompManager::provisionalSchedule - Per il nodo: "<<ueId<< " sono disponibili: "<<bytesPerBlock<<" bytes in un blocco"<<endl;
//        std::cout<<NOW<<" LteCompManager::provisionalSchedule - Per il nodo: "<<ueId<< " sono disponibili: "<<bytesPerBlock<<" bytes in un blocco"<<endl;

        // Compute the number of blocks required to satisfy the UE's buffer
        unsigned int reqBlocks;
        if (bytesPerBlock == 0)
            reqBlocks = 0;
        else
            reqBlocks = (queueLength + bytesPerBlock-1)/bytesPerBlock;
        EV<<NOW<<" LteCompManager::provisionalSchedule - Per il nodo: "<<ueId<< " sono necessari: "<<reqBlocks<<" blocchi"<<endl;
//        std::cout<<NOW<<" LteCompManager::provisionalSchedule - Per il nodo: "<<ueId<< " sono necessari: "<<reqBlocks<<" blocchi"<<endl;

        totalReqBlocks += reqBlocks;
    }

    if (type_ == COMP_MASTER)
    {
        // local handling

        // update map entry for this node
        if (reqBlocksMap_.find(nodeId_) == reqBlocksMap_.end() )
            reqBlocksMap_.insert(std::pair<X2NodeId, unsigned int>(nodeId_, totalReqBlocks));
        else
            reqBlocksMap_[nodeId_] = totalReqBlocks;

        receivedRequests_[nodeId_] = true;
    }
    else
    {

        // build IE
        X2CompRequestIE* requestIe = new X2CompRequestIE();
        requestIe->setNumBlocks(totalReqBlocks);

        // build control info
        X2ControlInfo* ctrlInfo = new X2ControlInfo();
        ctrlInfo->setSourceId(nodeId_);
        DestinationIdList destList;
        destList.push_back(masterId_);
        ctrlInfo->setDestIdList(destList);

        // build X2 Comp Msg
        X2CompMsg* compMsg = new X2CompMsg("X2CompMsg");
        compMsg->pushIe(requestIe);
        compMsg->setControlInfo(ctrlInfo);

        EV<<NOW<<" LteCompManager::provisionalSchedule - Send request for " << totalReqBlocks << " blocks" << endl;

        // send to X2 Manager
        send(PK(compMsg),x2Manager_[OUT]);
    }

    EV<<NOW<<" LteCompManager::provisionalSchedule - End "<<endl;
}

void LteCompManager::doBandwidthPartitioning()
{
    EV<<NOW<<" LteCompManager::doBandwidthPartitioning - Start " << endl;

   int numBands = mac_->getDeployer()->getNumBands();

   unsigned int requestsSum = 0;
   std::map<X2NodeId, unsigned int>::iterator it = reqBlocksMap_.begin();
   for (; it != reqBlocksMap_.end(); ++it)
       requestsSum += it->second;

   // assign a number of blocks that is proportional to the requests received from each eNB
   unsigned int totalReservedBlocks = 0;
   std::vector<double> reservation;
   reservation.clear();
   for (it = reqBlocksMap_.begin(); it != reqBlocksMap_.end(); ++it)
   {
       // requests from the current node
       unsigned int req = it->second;

       // compute the number of blocks to reserve
       double percentage;
       if (requestsSum == 0)
           percentage = 1.0 / (slavesList_.size() + 1);  // slaves + master
       else
           percentage = (double)req / requestsSum;

       double toReserve = numBands * percentage;
       totalReservedBlocks += toReserve;

       reservation.push_back(toReserve);
   }

   // round vector to integer
   std::vector<unsigned int> reservationRounded = roundVector(reservation, totalReservedBlocks);

   // build map for each node
   std::vector<CompRbStatus> allowedBlocks;
   unsigned int index = 0;
   unsigned int band = 0;
   for (it = reqBlocksMap_.begin(); it != reqBlocksMap_.end(); ++it)
   {
       X2NodeId id = it->first;
       unsigned int numBlocks = reservationRounded[index];

       // set "numBlocks" contiguous blocks for this node
       allowedBlocks.clear();
       allowedBlocks.resize(numBands, NOT_AVAILABLE_RB);
       unsigned int lb = band;
       unsigned int ub = band+numBlocks;
       for (unsigned int b=lb; b < ub; b++)
       {
           allowedBlocks[b] = AVAILABLE_RB;
           band++;
       }

       if (id == nodeId_)
       {
           // local handling: no need to send via X2
           UsableBands usableBands = parseAllowedBlocksMap(allowedBlocks);
           setUsableBands(usableBands);
       }
       else
       {
           // build reply IE and sends the message to the slave node
           sendReplyToSlave(id, allowedBlocks);
       }

       index++;
   }

   EV<<NOW<<" LteCompManager::doBandwidthPartitioning - End " << endl;

}

void LteCompManager::sendReplyToSlave(X2NodeId slaveId, std::vector<CompRbStatus> allowedBlocksMap)
{
    // build IE
    X2CompReplyIE* replyIe = new X2CompReplyIE();
    replyIe->setAllowedBlocksMap(allowedBlocksMap);

    // build control info
    X2ControlInfo* ctrlInfo = new X2ControlInfo();
    ctrlInfo->setSourceId(nodeId_);
    DestinationIdList destList;
    destList.push_back(slaveId);
    ctrlInfo->setDestIdList(destList);

    // build X2 Comp Msg
    X2CompMsg* compMsg = new X2CompMsg("X2CompMsg");
    compMsg->pushIe(replyIe);
    compMsg->setControlInfo(ctrlInfo);

    // send to X2 Manager
    send(PK(compMsg),x2Manager_[OUT]);
}
