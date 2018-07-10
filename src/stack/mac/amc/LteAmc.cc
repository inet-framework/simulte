//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/layer/LteMacEnb.h"

// NOTE: AMC Pilots header file inclusions must go here
#include "stack/mac/amc/AmcPilotAuto.h"
#include "stack/mac/amc/AmcPilotD2D.h"

LteAmc::~LteAmc()
{
    delete pilot_;
}

/*********************
 * PRIVATE FUNCTIONS
 *********************/

AmcPilot* LteAmc::getAmcPilot(const cPar& p)
{
    EV << "Creating Amc pilot " << p.stringValue() << endl;
    const char* s = p.stringValue();
    if(strcmp(s,"AUTO")==0)
    return new AmcPilotAuto(this);
    if(strcmp(s,"D2D")==0)
    return new AmcPilotD2D(this);
    throw cRuntimeError("Amc Pilot not recognized");
}

MacNodeId LteAmc::getNextHop(MacNodeId dst)
{
    MacNodeId nh = binder_->getNextHop(dst);

    if (nh == nodeId_)
    {
        // I'm the master for this slave (it is directly connected)
        return dst;
    }

    EV << "LteAmc::getNextHop Node Id dst : " << dst << endl;

    // The UE is connected to a relay
    // XXX assert(nodeType_==ENODEB);
    return nh;
}

void LteAmc::printParameters()
{
    EV << "###################" << endl;
    EV << "# LteAmc parameters" << endl;
    EV << "###################" << endl;

    EV << "NumUeDl: " << dlConnectedUe_.size() << endl;
    EV << "NumUeUl: " << ulConnectedUe_.size() << endl;
    EV << "Number of cell bands: " << numBands_ << endl;

    EV << "MacNodeId: " << nodeId_ << endl;
    EV << "MacCellId: " << cellId_ << endl;
    EV << "AmcMode: " << mac_->par("amcMode").stdstringValue() << endl;
    EV << "RbAllocationType: " << allocationType_ << endl;
    EV << "FBHB capacity DL: " << fbhbCapacityDl_ << endl;
    EV << "FBHB capacity UL: " << fbhbCapacityUl_ << endl;
    EV << "PmiWeight: " << pmiComputationWeight_ << endl;
    EV << "CqiWeight: " << cqiComputationWeight_ << endl;
    EV << "kCqi: " << kCqi_ << endl;
    EV << "DL MCS scale: " << mcsScaleDl_ << endl;
    EV << "UL MCS scale: " << mcsScaleUl_ << endl;
    EV << "Confidence LB: " << lb_ << endl;
    EV << "Confidence UB: " << ub_ << endl;
}

void LteAmc::printFbhb(Direction dir)
{
    EV << "###################################" << endl;
    EV << "# AMC FeedBack Historical Base (" << dirToA(dir) << ")" << endl;
    EV << "###################################" << endl;

    History_ *history;
    std::vector<MacNodeId> *revIndex;

    if(dir==DL)
    {
        history = &dlFeedbackHistory_;
        revIndex = &dlRevNodeIndex_;
    }
    else if(dir==UL)
    {
        history = &ulFeedbackHistory_;
        revIndex = &ulRevNodeIndex_;
    }
    else
    {
        throw cRuntimeError("LteAmc::printFbhb(): Unrecognized direction");
    }

    // preparing iterators
    History_::const_iterator it = history->begin();
    History_::const_iterator et = history->end();
    std::vector< std::vector< LteSummaryBuffer > >::const_iterator uit,uet;
    std::vector<LteSummaryBuffer>::const_iterator txit,txet;

    for(; it!=et; it++)  // for each antenna
    {
        EV << simTime() << " # Remote: " << dasToA(it->first) << "\n";
        uit = (*history)[it->first].begin();
        uet = (*history)[it->first].end();
        int i = 0;
        for(; uit!=uet; uit++) // for each UE
        {
            EV << "Ue index: " << i << ", MacNodeId: " << (*revIndex)[i] << endl;
            txit = (*history)[it->first][i].begin();
            txet = (*history)[it->first][i].end();
            int t = 0;
            TxMode txMode;
            for(; txit!=txet; txit++)  // for each tx mode
            {
                txMode = TxMode(t);
                t++;

                // Print only non empty feedback summary! (all cqi are != NOSIGNALCQI)
                Cqi testCqi = ((*txit).get()).getCqi(Codeword(0),Band(0));
                if(testCqi==NOSIGNALCQI)
                continue;

                EV << "@TxMode " << txMode << endl;
                ((*txit).get()).print(0,(*revIndex)[i],dir, txMode,"LteAmc::printAmcFbhb");
            }
            i++;
        }
    }
}

void LteAmc::printTxParams(Direction dir)
{
    EV << "######################" << endl;
    EV << "# UserTxParams vector (" << dirToA(dir) << ")" << endl;
    EV << "######################" << endl;

    std::vector<UserTxParams>::const_iterator it,et;
    std::vector<UserTxParams> *userInfo;
    std::vector<MacNodeId> *revIndex;

    if(dir==DL)
    {
        userInfo = &dlTxParams_;
        revIndex = &dlRevNodeIndex_;
    }
    else if(dir==UL)
    {
        userInfo = &ulTxParams_;
        revIndex = &ulRevNodeIndex_;
    }
    else if (dir == D2D)
    {
        userInfo = &d2dTxParams_;
        revIndex = &d2dRevNodeIndex_;
    }
    else
    {
        throw cRuntimeError("LteAmc::printTxParams(): Unrecognized direction");
    }

    it = userInfo->begin();
    et = userInfo->end();

    // Cqi testCqi=0;
    int index = 0;
    for(; it!=et; it++)
    {
        EV << "Ue index: " << index << ", MacNodeId: " << (*revIndex)[index] << endl;

        // Print only non empty user transmission parameters
        // testCqi = (*it).readCqiVector().at(0);
        //if(testCqi!=0)
        (*it).print("info");

        index++;
    }
}

void LteAmc::printMuMimoMatrix(const char* s)
{
    muMimoDlMatrix_.print(s);
    muMimoUlMatrix_.print(s);
    muMimoD2DMatrix_.print(s);
}

/********************
 * PUBLIC FUNCTIONS
 ********************/

LteAmc::LteAmc(LteMacEnb *mac, LteBinder *binder, LteCellInfo *cellInfo, int numAntennas)
{
    mac_ = mac;
    binder_ = binder;
    cellInfo_ = cellInfo;
    numAntennas_ = numAntennas;
    initialize();
}

void LteAmc::initialize()
{
    /** Get MacNodeId and MacCellId **/
    nodeId_ = mac_->getMacNodeId();
    cellId_ = mac_->getMacCellId();

    /** Get deployed UEs maps from Binder **/
    dlConnectedUe_ = binder_->getDeployedUes(nodeId_, DL);
    ulConnectedUe_ = binder_->getDeployedUes(nodeId_, UL);
    d2dConnectedUe_ = binder_->getDeployedUes(nodeId_, UL);

    /** Get parameters from cellInfo **/
    numBands_ = cellInfo_->getNumBands();
    mcsScaleDl_ = cellInfo_->getMcsScaleDl();
    mcsScaleUl_ = cellInfo_->getMcsScaleUl();
    mcsScaleD2D_ = cellInfo_->getMcsScaleUl();

    /** Get AMC parameters from MAC module NED **/
    fbhbCapacityDl_ = mac_->par("fbhbCapacityDl");
    fbhbCapacityUl_ = mac_->par("fbhbCapacityUl");
    fbhbCapacityD2D_ = mac_->par("fbhbCapacityD2D");
    pmiComputationWeight_ = mac_->par("pmiWeight");
    cqiComputationWeight_ = mac_->par("cqiWeight");
    kCqi_ = mac_->par("kCqi");
    pilot_ = getAmcPilot(mac_->par("amcMode"));
    allocationType_ = getRbAllocationType(mac_->par("rbAllocationType").stringValue());
    lb_ = mac_->par("summaryLowerBound");
    ub_ = mac_->par("summaryUpperBound");

    printParameters();

    /** Structures initialization **/

    // Scale Mcs Tables
    dlMcsTable_.rescale(mcsScaleDl_);
    ulMcsTable_.rescale(mcsScaleUl_);
    d2dMcsTable_.rescale(mcsScaleD2D_);

    // Initialize DAS structures
    for (int i = 0; i < numAntennas_; i++)
    {
        EV << "Adding Antenna: " << dasToA(Remote(i)) << endl;
        remoteSet_.insert(Remote(i));
    }

        // Initializing feedback and scheduling structures

        /**
         * Preparing iterators.
         * Note: at initialization ALL dlConnectedUe_ and ulConnectedUs_ elements are TRUE.
         */
    ConnectedUesMap::const_iterator it, et;
    RemoteSet::const_iterator ait, aet;

    /* DOWNLINK */

    it = dlConnectedUe_.begin();
    et = dlConnectedUe_.end();

    EV << "DL CONNECTED: " << dlConnectedUe_.size() << endl;

    for (; it != et; it++)  // For all UEs (DL)
    {
        MacNodeId nodeId = it->first;
        dlNodeIndex_[nodeId] = dlRevNodeIndex_.size();
        dlRevNodeIndex_.push_back(nodeId);

        EV << "Creating UE, id: " << nodeId << ", index: " << dlNodeIndex_[nodeId] << endl;

        ait = remoteSet_.begin();
        aet = remoteSet_.end();

        for (; ait != aet; ait++)
        {
            // initialize historical feedback base for this UE (index) for all tx modes and for all RUs
            dlFeedbackHistory_[*ait].push_back(
                std::vector<LteSummaryBuffer>(DL_NUM_TXMODE,
                    LteSummaryBuffer(fbhbCapacityDl_, MAXCW, numBands_, lb_, ub_)));
        }
    }

    // Initialize user transmission parameters structures
    dlTxParams_.resize(dlConnectedUe_.size(), UserTxParams());

    /* UPLINK */
    EV << "UL CONNECTED: " << dlConnectedUe_.size() << endl;

    it = ulConnectedUe_.begin();
    et = ulConnectedUe_.end();

    for (; it != et; it++)  // For all UEs (UL)
    {
        MacNodeId nodeId = it->first;
        ulNodeIndex_[nodeId] = ulRevNodeIndex_.size();
        ulRevNodeIndex_.push_back(nodeId);

        ait = remoteSet_.begin();
        aet = remoteSet_.end();

        for (; ait != aet; ait++)
        {
            // initialize historical feedback base for this UE (index) for all tx modes and for all RUs
            ulFeedbackHistory_[*ait].push_back(
                std::vector<LteSummaryBuffer>(UL_NUM_TXMODE,
                    LteSummaryBuffer(fbhbCapacityUl_, MAXCW, numBands_, lb_, ub_)));
        }
    }

    // Initialize user transmission parameters structures
    ulTxParams_.resize(ulConnectedUe_.size(), UserTxParams());

    /* D2D */
    EV << "D2D CONNECTED: " << d2dConnectedUe_.size() << endl;

    it = d2dConnectedUe_.begin();
    et = d2dConnectedUe_.end();

    for (; it != et; it++)  // For all UEs (UL)
    {
        MacNodeId nodeId = it->first;
        d2dNodeIndex_[nodeId] = d2dRevNodeIndex_.size();
        d2dRevNodeIndex_.push_back(nodeId);

//        ait = remoteSet_.begin();
//        aet = remoteSet_.end();
//
//        for (; ait != aet; ait++)
//        {
//            // initialize historical feedback base for this UE (index) for all tx modes and for all RUs
//            d2dFeedbackHistory_[*ait].push_back(
//                std::vector<LteSummaryBuffer>(UL_NUM_TXMODE,
//                    LteSummaryBuffer(fbhbCapacityD2D_, MAXCW, numBands_, lb_, ub_)));
//        }
    }

    // Initialize user transmission parameters structures
    d2dTxParams_.resize(d2dConnectedUe_.size(), UserTxParams());

    //printFbhb(DL);
    //printFbhb(UL);
    //printTxParams(DL);
    //printTxParams(UL);
}

void LteAmc::rescaleMcs(double rePerRb, Direction dir)
{
    if (dir == DL)
    {
        dlMcsTable_.rescale(rePerRb);
    }
    else if (dir == UL)
    {
        ulMcsTable_.rescale(rePerRb);
    }
    else if (dir == D2D) {
        d2dMcsTable_.rescale(rePerRb);
    }
}

/*******************************************
 *    Functions for feedback management    *
 *******************************************/

void LteAmc::pushFeedback(MacNodeId id, Direction dir, LteFeedback fb)
{
    EV << "Feedback from MacNodeId " << id << " (direction " << dirToA(dir) << ")" << endl;

    History_ *history;
    std::map<MacNodeId, unsigned int> *nodeIndex;

    if(dir==DL)
    {
        history = &dlFeedbackHistory_;
        nodeIndex = &dlNodeIndex_;
    }
    else if(dir==UL)
    {
        history = &ulFeedbackHistory_;
        nodeIndex = &ulNodeIndex_;
    }
    else
    {
        throw cRuntimeError("LteAmc::pushFeedback(): Unrecognized direction");
    }

    // Put the feedback in the FBHB
    Remote antenna = fb.getAntennaId();
    TxMode txMode = fb.getTxMode();
    if (nodeIndex->find(id) == nodeIndex->end())
    {
        return;
    }
    int index = (*nodeIndex).at(id);

    EV << "ID: " << id << endl;
    EV << "index: " << index << endl;
    (*history)[antenna].at(index).at(txMode).put(fb);

    // DEBUG
//    printFbhb(dir);
    EV << "Antenna: " << dasToA(antenna) << ", TxMode: " << txMode << ", Index: " << index << endl;
    EV << "RECEIVED" << endl;
    fb.print(0,id,dir,"LteAmc::pushFeedback");
//    EV << "SUMMARY" << endl;
//    (*history)[antenna].at(index).at(txMode).get().print(0,id,dir,txMode,"LteAmc::pushFeedback");
}

void LteAmc::pushFeedbackD2D(MacNodeId id, LteFeedback fb, MacNodeId peerId)
{
    EV << "Feedback from MacNodeId " << id << " (direction D2D), peerId = " << peerId << endl;

    std::map<MacNodeId, History_> *history = &d2dFeedbackHistory_;
    std::map<MacNodeId, unsigned int> *nodeIndex = &d2dNodeIndex_;

    // Put the feedback in the FBHB
    Remote antenna = fb.getAntennaId();
    TxMode txMode = fb.getTxMode();
    int index = (*nodeIndex).at(id);

    EV << "ID: " << id << endl;
    EV << "index: " << index << endl;

    if (history->find(peerId) == history->end())
    {
        // initialize new history for this peering UE
        History_ newHist;

        ConnectedUesMap::const_iterator it, et;
        it = d2dConnectedUe_.begin();
        et = d2dConnectedUe_.end();
        for (; it != et; it++)  // For all UEs (D2D)
        {
            newHist[antenna].push_back(std::vector<LteSummaryBuffer>(UL_NUM_TXMODE, LteSummaryBuffer(fbhbCapacityD2D_, MAXCW, numBands_, lb_, ub_)));
        }
        (*history)[peerId] = newHist;
    }
    (*history)[peerId][antenna].at(index).at(txMode).put(fb);

    // DEBUG
    EV << "PeerId: " << peerId << ", Antenna: " << dasToA(antenna) << ", TxMode: " << txMode << ", Index: " << index << endl;
    EV << "RECEIVED" << endl;
    fb.print(0,id,D2D,"LteAmc::pushFeedbackD2D");
}


LteSummaryFeedback LteAmc::getFeedback(MacNodeId id, Remote antenna, TxMode txMode, const Direction dir)
{
    MacNodeId nh = getNextHop(id);
    if (id != nh)
        EV << NOW << " LteAmc::getFeedback detected " << nh << " as nexthop for " << id << "\n";
    id = nh;

    if (dir == DL)
        return dlFeedbackHistory_.at(antenna).at(dlNodeIndex_.at(id)).at(txMode).get();
    else if (dir == UL)
        return ulFeedbackHistory_.at(antenna).at(ulNodeIndex_.at(id)).at(txMode).get();
    else
    {
        throw cRuntimeError("LteAmc::getFeedback(): Unrecognized direction");
    }
}

LteSummaryFeedback LteAmc::getFeedbackD2D(MacNodeId id, Remote antenna, TxMode txMode, MacNodeId peerId)
{
    MacNodeId nh = getNextHop(id);

    if (id != nh)
        EV << NOW << " LteAmc::getFeedbackD2D detected " << nh << " as nexthop for " << id << "\n";
    id = nh;

    if (peerId == 0)
    {
        // we returns the first feedback stored  in the structure
        std::map<MacNodeId, History_>::iterator it = d2dFeedbackHistory_.begin();
        for (; it != d2dFeedbackHistory_.end(); ++it)
        {
            if (it->first == 0) // skip fake UE 0
                continue;

            if (binder_->getD2DCapability(id, it->first))
            {
                peerId = it->first;
                break;
            }
        }

        // default feedback: when there is no feedback from peers yet (NOSIGNALCQI)
        if (peerId == 0)
            return d2dFeedbackHistory_.at(0).at(MACRO).at(0).at(txMode).get();
    }
    return d2dFeedbackHistory_.at(peerId).at(antenna).at(d2dNodeIndex_.at(id)).at(txMode).get();
}

/*******************************************
 *    Functions for MU-MIMO support       *
 *******************************************/

MacNodeId LteAmc::computeMuMimoPairing(const MacNodeId nodeId, Direction dir)
{
    if (dir == DL)
    {
        return muMimoDlMatrix_.getMuMimoPair(nodeId);
    }
    else if (dir==UL)
    {
        return muMimoUlMatrix_.getMuMimoPair(nodeId);
    }
    else if (dir == D2D)
    {
        return muMimoD2DMatrix_.getMuMimoPair(nodeId);
    }
}

/********************************
 *       Access Functions       *
 ********************************/

bool LteAmc::existTxParams(MacNodeId id, const Direction dir)
{
    MacNodeId nh = getNextHop(id);
    if (id != nh)
        EV << NOW << " LteAmc::existTxparams detected " << nh << " as nexthop for " << id << "\n";
    id = nh;

    if (dir == DL)
        return dlTxParams_.at(dlNodeIndex_.at(id)).isSet();
    else if (dir == UL)
        return ulTxParams_.at(ulNodeIndex_.at(id)).isSet();
    else if (dir == D2D)
        return d2dTxParams_.at(d2dNodeIndex_.at(id)).isSet();
    else
    {
        throw cRuntimeError("LteAmc::existTxparams(): Unrecognized direction");
    }
}

const UserTxParams& LteAmc::setTxParams(MacNodeId id, const Direction dir, UserTxParams& info)
{
    MacNodeId nh = getNextHop(id);
    if (id != nh)
        EV << NOW << " LteAmc::setTxParams detected " << nh << " as nexthop for " << id << "\n";
    id = nh;

    info.isSet() = true;

    /**
     * NOTE: if the antenna set has not been explicitly written in UserTxParams
     * by the AMC pilot, this antennas set contains only MACRO
     * (this is done setting MACRO in UserTxParams constructor)
     */

    // DEBUG
    EV << NOW << " LteAmc::setTxParams DAS antenna set for user " << id << " is \t";
    for (std::set<Remote>::const_iterator it = info.readAntennaSet().begin(); it != info.readAntennaSet().end(); ++it)
    {
        EV << "[" << dasToA(*it) << "]\t";
    }
    EV << endl;

    if (dir == DL)
        return (dlTxParams_.at(dlNodeIndex_.at(id)) = info);
    else if (dir == UL)
        return (ulTxParams_.at(ulNodeIndex_.at(id)) = info);
    else if (dir == D2D)
        return (d2dTxParams_.at(d2dNodeIndex_.at(id)) = info);
    else
    {
        throw cRuntimeError("LteAmc::setTxParams(): Unrecognized direction");
    }
}

const UserTxParams& LteAmc::computeTxParams(MacNodeId id, const Direction dir)
{
    // DEBUG
    EV << NOW << " LteAmc::computeTxParams --------------::[ START ]::--------------\n";
    EV << NOW << " LteAmc::computeTxParams CellId: " << cellId_ << "\n";
    EV << NOW << " LteAmc::computeTxParams NodeId: " << id << "\n";
    EV << NOW << " LteAmc::computeTxParams Direction: " << dirToA(dir) << "\n";
    EV << NOW << " LteAmc::computeTxParams - - - - - - - - - - - - - - - - - - - - -\n";
    EV << NOW << " LteAmc::computeTxParams RB allocation type: " << allocationTypeToA(allocationType_) << "\n";
    EV << NOW << " LteAmc::computeTxParams - - - - - - - - - - - - - - - - - - - - -\n";

    MacNodeId nh = getNextHop(id);
    if(id != nh)
    EV << NOW << " LteAmc::computeTxParams detected " << nh << " as nexthop for " << id << "\n";
    id = nh;

    const UserTxParams &info = pilot_->computeTxParams(id,dir);
    EV << NOW << " LteAmc::computeTxParams --------------::[  END  ]::--------------\n";

    return info;
}

void LteAmc::cleanAmcStructures(Direction dir, ActiveSet aUser)
{
    EV << NOW << " LteAmc::cleanAmcStructures. Direction " << dirToA(dir) << endl;

    //Convert from active cid to active users
    //Update active user for TMS algorithms
    pilot_->updateActiveUsers(aUser,dir);
    if (dir == DL)
    {
        // clearing assignments
        std::vector<UserTxParams>::iterator it = dlTxParams_.begin();
        std::vector<UserTxParams>::iterator et = dlTxParams_.end();
        for(; it != et; ++it)
        it->restoreDefaultValues();
    }
    else if (dir == UL)
    {
        // clearing assignments
        std::vector<UserTxParams>::iterator it = ulTxParams_.begin();
        std::vector<UserTxParams>::iterator et = ulTxParams_.end();
        for(; it != et; ++it)
            it->restoreDefaultValues();

        // clearing D2D assignments
        it = d2dTxParams_.begin();
        et = d2dTxParams_.end();
        for(; it != et; ++it)
            it->restoreDefaultValues();
    }
    else
    {
        throw cRuntimeError("LteAmc::cleanAmcStructures(): Unrecognized direction");
    }
}

    /*******************************************
     *      Scheduler interface functions      *
     *******************************************/

unsigned int LteAmc::computeReqRbs(MacNodeId id, Band b, Codeword cw, unsigned int bytes, const Direction dir)
{
    EV << NOW << " LteAmc::getRbs Node " << id << ", Band " << b << ", Codeword " << cw << ", direction " << dirToA(dir) << endl;

    if(bytes == 0)
    {
        // DEBUG
        EV << NOW << " LteAmc::getRbs Occupation: 0 bytes\n";
        EV << NOW << " LteAmc::getRbs Number of RBs: 0\n";

        return 0;
    }

    // Loading TBS vectors
    const unsigned int* tbsVect;// it is a row of the itbs matrix
    UserTxParams info = computeTxParams(id, dir);
    unsigned char layers = info.getLayers().at(cw);

    LteMod mod = info.getCwModulation(cw);
    unsigned int iTbs = getItbsPerCqi(info.readCqiVector().at(cw), dir);
    unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));
    tbsVect = itbs2tbs(mod, info.readTxMode(), layers, iTbs-i);

    // Computing RB occupation
    unsigned int j;
    for(j = 0; j < 110; ++j)
    if(tbsVect[j] >= bytes*8)
    break;

    // DEBUG
    EV << NOW << " LteAmc::getRbs Occupation: " << bytes << " bytes , CQI : " << info.readCqiVector().at(cw) << " \n";
    EV << NOW << " LteAmc::getRbs Number of RBs: " << j+1 << "\n";

    return j+1;
}

unsigned int LteAmc::computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir)
{
    if (blocks > 110)    // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::computeBitsOnNRbs(): Too many blocks");

    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " LteAmc::blocks2bits Node: " << id << "\n";
    EV << NOW << " LteAmc::blocks2bits Band: " << b << "\n";
    EV << NOW << " LteAmc::blocks2bits Direction: " << dirToA(dir) << "\n";

    // Acquiring current user scheduling information
    const UserTxParams & info = computeTxParams(id, dir);

    std::vector<unsigned char> layers = info.getLayers();

    unsigned int bits = 0;
    unsigned int codewords = layers.size();
    for (Codeword cw = 0; cw < codewords; ++cw)
    {
        // if CQI == 0 the UE is out of range, thus bits=0
        if (info.readCqiVector().at(cw) == 0)
        {
            EV << NOW << " LteAmc::blocks2bits - CQI equal to zero on cw " << cw << ", return no blocks available" << endl;
            continue;
        }

        LteMod mod = info.getCwModulation(cw);
        unsigned int iTbs = getItbsPerCqi(info.readCqiVector().at(cw), dir);
        unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));

        // DEBUG
        EV << NOW << " LteAmc::blocks2bits ---::[ Codeword = " << cw << "\n";
        EV << NOW << " LteAmc::blocks2bits Modulation: " << modToA(mod) << "\n";
        EV << NOW << " LteAmc::blocks2bits iTbs: " << iTbs << "\n";
        EV << NOW << " LteAmc::blocks2bits i: " << i << "\n";
        EV << NOW << " LteAmc::blocks2bits CQI: " << info.readCqiVector().at(cw) << "\n";

        mac_->emitItbs(iTbs);

        const unsigned int* tbsVect = itbs2tbs(mod, info.readTxMode(), layers.at(cw), iTbs-i);
        bits += tbsVect[blocks-1];
    }

            // DEBUG
    EV << NOW << " LteAmc::blocks2bits Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::blocks2bits Available space: " << bits << "\n";

    return bits;
}

unsigned int LteAmc::computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir)
{
    if (blocks > 110)    // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::blocks2bits(): Too many blocks");

    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " LteAmc::blocks2bits Node: " << id << "\n";
    EV << NOW << " LteAmc::blocks2bits Band: " << b << "\n";
    EV << NOW << " LteAmc::blocks2bits Codeword: " << cw << "\n";
    EV << NOW << " LteAmc::blocks2bits Direction: " << dirToA(dir) << "\n";

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir);

    // if CQI == 0 the UE is out of range, thus return 0
    if (info.readCqiVector().at(cw) == 0)
    {
        EV << NOW << " LteAmc::blocks2bits - CQI equal to zero, return no blocks available" << endl;
        return 0;
    }
    unsigned char layers = info.getLayers().at(cw);

    unsigned int iTbs = getItbsPerCqi(info.readCqiVector().at(cw), dir);
    LteMod mod = info.getCwModulation(cw);
    unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));

    // DEBUG
    EV << NOW << " LteAmc::blocks2bits Modulation: " << modToA(mod) << "\n";
    EV << NOW << " LteAmc::blocks2bits iTbs: " << iTbs << "\n";
    EV << NOW << " LteAmc::blocks2bits i: " << i << "\n";

    const unsigned int* tbsVect = itbs2tbs(mod, info.readTxMode(), layers, iTbs - i);

    // DEBUG
    EV << NOW << " LteAmc::blocks2bits Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::blocks2bits Available space: " << tbsVect[blocks-1] << "\n";

    return tbsVect[blocks - 1];
}

unsigned int LteAmc::computeBytesOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir)
{
    EV << NOW << " LteAmc::blocks2bytes Node " << id << ", Band " << b << ", direction " << dirToA(dir) << ", blocks " << blocks << "\n";

    unsigned int bits = computeBitsOnNRbs(id, b, blocks, dir);
    unsigned int bytes = bits/8;

    // DEBUG
    EV << NOW << " LteAmc::blocks2bytes Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::blocks2bytes Available space: " << bits << "\n";
    EV << NOW << " LteAmc::blocks2bytes Available space: " << bytes << "\n";

    return bytes;
}

unsigned int LteAmc::computeBytesOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir)
{
    EV << NOW << " LteAmc::blocks2bytes Node " << id << ", Band " << b << ", Codeword " << cw << ",  direction " << dirToA(dir) << ", blocks " << blocks << "\n";

    unsigned int bits = computeBitsOnNRbs(id, b, cw, blocks, dir);
    unsigned int bytes = bits/8;

    // DEBUG
    EV << NOW << " LteAmc::blocks2bytes Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::blocks2bytes Available space: " << bits << "\n";
    EV << NOW << " LteAmc::blocks2bytes Available space: " << bytes << "\n";

    return bytes;
}

unsigned int LteAmc::computeBytesOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir)
{
    EV << NOW << " LteAmc::computeBytesOnNRbs_MB Node " << id << ", Band " << b << ",  direction " << dirToA(dir) << ", blocks " << blocks << "\n";

    unsigned int bits = computeBitsOnNRbs_MB(id, b, blocks, dir);
    unsigned int bytes = bits/8;

    // DEBUG
    EV << NOW << " LteAmc::computeBytesOnNRbs_MB Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::computeBytesOnNRbs_MB Available space: " << bits << "\n";
    EV << NOW << " LteAmc::computeBytesOnNRbs_MB Available space: " << bytes << "\n";

    return bytes;

}

unsigned int LteAmc::computeBitsOnNRbs_MB(MacNodeId id, Band b,  unsigned int blocks, const Direction dir)
{
    if (blocks > 110)    // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::computeBitsOnNRbs_MB(): Too many blocks");

    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Node: " << id << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Band: " << b << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Direction: " << dirToA(dir) << "\n";

    Cqi cqi = readMultiBandCqi(id,dir)[b];

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir);

    std::vector<unsigned char> layers = info.getLayers();

    // if CQI == 0 the UE is out of range, thus return 0
    if (cqi == 0)
    {
        EV << NOW << " LteAmc::computeBitsOnNRbs_MB - CQI equal to zero, return no blocks available" << endl;
        return 0;
    }

    unsigned int iTbs = getItbsPerCqi(cqi, dir);
    LteMod mod = cqiTable[cqi].mod_;
    unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));

    // DEBUG
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Modulation: " << modToA(mod) << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB iTbs: " << iTbs << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB i: " << i << "\n";

    const unsigned int* tbsVect = itbs2tbs(mod, TRANSMIT_DIVERSITY, layers[0], iTbs - i);

    // DEBUG
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Available space: " << tbsVect[blocks-1] << "\n";

    return tbsVect[blocks - 1];

}

bool LteAmc::setPilotUsableBands(MacNodeId id , std::vector<unsigned short>  usableBands)
{
    pilot_->setUsableBands(id,usableBands);
    return true;
}

bool LteAmc::getPilotUsableBands(MacNodeId id, std::vector<unsigned short>*& usableBands)
{
    return pilot_->getUsableBands(id, usableBands);
}

unsigned int LteAmc::getItbsPerCqi(Cqi cqi, const Direction dir)
{
    // CQI threshold table selection
    McsTable* mcsTable;
    if (dir == DL)
        mcsTable = &dlMcsTable_;
    else if ((dir == UL) || (dir == D2D) || (dir == D2D_MULTI))
        mcsTable = &ulMcsTable_;
    else
    {
        throw cRuntimeError("LteAmc::getItbsPerCqi(): Unrecognized direction");
    }
    CQIelem entry = cqiTable[cqi];
    LteMod mod = entry.mod_;
    double rate = entry.rate_;

    // Select the ranges for searching in the McsTable.
    unsigned int min = 0; // _QPSK
    unsigned int max = 9; // _QPSK
    if (mod == _16QAM)
    {
        min = 10;
        max = 16;
    }
    if (mod == _64QAM)
    {
        min = 17;
        max = 28;
    }

    // Initialize the working variables at the minimum value.
    MCSelem elem = mcsTable->at(min);
    unsigned int iTbs = elem.iTbs_;

    // Search in the McsTable from min to max until the rate exceeds
    // the threshold in an entry of the table.
    for (unsigned int i = min; i <= max; i++)
    {
        elem = mcsTable->at(i);
        if (elem.threshold_ <= rate)
            iTbs = elem.iTbs_;
        else
            break;
    }

    // Return the iTbs found.
    return iTbs;
}

const UserTxParams& LteAmc::getTxParams(MacNodeId id, const Direction dir)
{
    MacNodeId nh = getNextHop(id);
    if (id != nh)
        EV << NOW << " LteAmc::getTxParams detected " << nh << " as nexthop for " << id << "\n";
    id = nh;

    if (dir == DL)
        return dlTxParams_.at(dlNodeIndex_.at(id));
    else if (dir == UL)
        return ulTxParams_.at(ulNodeIndex_.at(id));
    else if (dir == D2D)
        return d2dTxParams_.at(d2dNodeIndex_.at(id));
    else
        throw cRuntimeError("LteAmc::getTxParams(): Unrecognized direction");
}

double LteAmc::readCoderate(MacNodeId id, Codeword cw, unsigned int bytes, const Direction dir)
{
    if (bytes == 0)
        return 0.0; // To avoid floating point exception...

    // Number of available resource element in the given link direction
    unsigned int availRe;
    if (dir == DL)
    {
        availRe = 2
            * (cellInfo_->getRbyDl() * cellInfo_->getRbxDl() - cellInfo_->getSignalDl() * cellInfo_->getRbyDl()
                - cellInfo_->getRbPilotDl());
    }
    else if (dir == UL)
    {
        availRe = 2
            * (cellInfo_->getRbyUl() * cellInfo_->getRbxUl() - cellInfo_->getSignalUl() * cellInfo_->getRbyUl()
                - cellInfo_->getRbPilotUl());
    }
    else
    {
        throw cRuntimeError("LteAmc::getCoderate(): Unrecognized direction");
    }

    // Loading the user transmission parameters
    const UserTxParams& info = computeTxParams(id, dir);
    std::vector<unsigned char> layers = info.getLayers();

    // Loading TBS vectors
    std::vector<const unsigned int*> tbsVect;

    unsigned int codewords = layers.size();
    for (Codeword c = 0; c < codewords; ++c)
    {
        LteMod mod = info.getCwModulation(c);
        unsigned int iTbs = getItbsPerCqi(info.readCqiVector().at(c), dir);
        unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));
        tbsVect.push_back(itbs2tbs(mod, info.readTxMode(), layers.at(c), iTbs - i));
    }

    // Computing RB occupation
    unsigned int blocks;
    for (blocks = 0; blocks < 110; ++blocks)
    {
        unsigned int sum = 0;
        for (Codeword c = 0; c < codewords; ++c)
            sum += (tbsVect.at(c))[blocks];

        if (sum >= bytes * 8)
            break;
    }

    ++blocks;

    LteMod mod = info.getCwModulation(cw);
    double qm = (mod == _QPSK) ? 2.0 : (mod == _16QAM ? 4.0 : (mod == _64QAM ? 6.0 : 0.0));

    double num = ((bytes * 8.0) + 24.0) * 1024.0;
    double den = qm * availRe * blocks * info.getLayers().at(cw);

    return (num / den);
}

unsigned int
LteAmc::blockGain(Cqi cqi, unsigned int layers, unsigned int blocks, Direction dir)
{
    if (blocks > 110)  // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::blocksGain(): Too many blocks (%d)", blocks);

    if (cqi > 15)  // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::blocksGain(): CQI greater than 15 (%d)", cqi);

    if (blocks == 0)
        return 0;
    const unsigned int* tbsVect = readTbsVect(cqi, layers, dir);

    if (tbsVect == NULL)
        return 0;
    return (tbsVect[blocks - 1] / 8);
}

unsigned int
LteAmc::bytesGain(Cqi cqi, unsigned int layers, unsigned int bytes, Direction dir)
{
    if (bytes == 0)
        return 0;
    const unsigned int* tbsVect = readTbsVect(cqi, layers, dir);

    if (tbsVect == 0)
        return 0;

    unsigned int i = 0;
    for (; i < 110; ++i)
    {
        if (tbsVect[i] >= (bytes * 8))
            break;
    }
    return i + 1;
}

const unsigned int*
LteAmc::readTbsVect(Cqi cqi, unsigned int layers, Direction dir)
{
    unsigned int itbs = getItbsPerCqi(cqi, dir);
    LteMod mod = cqiTable[cqi].mod_;

    const unsigned int* tbsVect = NULL;
    switch (mod)
    {
        case _QPSK:
            {
            switch (layers)
            {
                case 1:
                    tbsVect = itbs2tbs_qpsk_1[itbs];
                    break;
                case 2:
                    tbsVect = itbs2tbs_qpsk_2[itbs];
                    break;
                case 4:
                    tbsVect = itbs2tbs_qpsk_4[itbs];
                    break;
                case 8:
                    tbsVect = itbs2tbs_qpsk_8[itbs];
                    break;
            }
            break;
        }
        case _16QAM:
            {
            switch (layers)
            {
                case 1:
                    tbsVect = itbs2tbs_16qam_1[itbs - 9];
                    break;
                case 2:
                    tbsVect = itbs2tbs_16qam_2[itbs - 9];
                    break;
                case 4:
                    tbsVect = itbs2tbs_16qam_4[itbs - 9];
                    break;
//                    case 8:
//                        tbsVect = itbs2tbs_16qam_8[itbs]; break;
            }
            break;
        }
        case _64QAM:
            {
            switch (layers)
            {
                case 1:
                    tbsVect = itbs2tbs_64qam_1[itbs - 15];
                    break;
                case 2:
                    tbsVect = itbs2tbs_64qam_2[itbs - 15];
                    break;
                case 4:
                    tbsVect = itbs2tbs_64qam_4[itbs - 15];
                    break;
//                    case 8:
//                        tbsVect = itbs2tbs_64qam_8[itbs]; break;
            }
            break;
        }
    }
    return tbsVect;
}

/*************************************************
 *    Functions for wideband values generation   *
 *************************************************/

void LteAmc::writeCqiWeight(const double weight)
{
    // set the CQI weight
    cqiComputationWeight_ = weight;
}

Cqi LteAmc::readWbCqi(const CqiVector& cqi)
{
    // during the process, consider
    // - the cqi value which will be returned
    Cqi cqiRet = NOSIGNALCQI;
    // - a counter to obtain the mean
    Cqi cqiCounter = NOSIGNALCQI;
    // - the mean value
    Cqi cqiMean = NOSIGNALCQI;
    // - the min value
    Cqi cqiMin = NOSIGNALCQI;
    // - the max value
    Cqi cqiMax = NOSIGNALCQI;

    // consider the cqi of each band
    unsigned int bands = cqi.size();
    for (Band b = 0; b < bands; ++b)
    {
        EV << "LteAmc::getWbCqi - Cqi " << cqi.at(b) << " on band " << (int)b << endl;

        cqiCounter += cqi.at(b);
        cqiMin = cqiMin < cqi.at(b) ? cqiMin : cqi.at(b);
        cqiMax = cqiMax > cqi.at(b) ? cqiMax : cqi.at(b);
    }

        // when casting a double to an unsigned int value, consider the closest one

        // is the module lower than the half of the divisor ? ceil, otherwise floor
    cqiMean = (double) (cqiCounter % bands) > (double) bands / 2.0 ? cqiCounter / bands + 1 : cqiCounter / bands;

    EV << "LteAmc::getWbCqi - Cqi mean " << cqiMean << " minimum " << cqiMin << " maximum " << cqiMax << endl;

    // the 0.0 weight is used in order to obtain the mean
    if (cqiComputationWeight_ == 0.0)
        cqiRet = cqiMean;
    // the -1.0 weight is used in order to obtain the min
    else if (cqiComputationWeight_ == -1.0)
        cqiRet = cqiMin;
    // the 1.0 weight is used in order to obtain the max
    else if (cqiComputationWeight_ == 1.0)
        cqiRet = cqiMax;
    // the following weight is used in order to obtain a value between the min and the mean
    else if (-1.0 < cqiComputationWeight_ && cqiComputationWeight_ < 0.0)
    {
        cqiRet = cqiMin;
        // ceil or floor depending on decimal part (casting to unsigned int results in a ceiling)
        double ret = (cqiComputationWeight_ + 1.0) * ((double) cqiMean - (double) cqiMin);
        cqiRet += ret - ((unsigned int) ret) > 0.5 ? (unsigned int) ret + 1 : (unsigned int) ret;
    }
    // the following weight is used in order to obtain a value between the min and the max
    else if (0.0 < cqiComputationWeight_ && cqiComputationWeight_ < 1.0)
    {
        cqiRet = cqiMean;
        // ceil or floor depending on decimal part (casting to unsigned int results in a ceiling)
        double ret = (cqiComputationWeight_) * ((double) cqiMax - (double) cqiMean);
        cqiRet += ret - ((unsigned int) ret) > 0.5 ? (unsigned int) ret + 1 : (unsigned int) ret;
    }
    else
    {
        throw cRuntimeError("LteAmc::getWbCqi(): Unknown weight %d", cqiComputationWeight_);
    }

    EV << "LteAmc::getWbCqi - Cqi " << cqiRet << " evaluated\n";

    return cqiRet;
}


std::vector<Cqi>  LteAmc::readMultiBandCqi(MacNodeId id, const Direction dir)
{
    return pilot_->getMultiBandCqi(id,dir);
}

void LteAmc::writePmiWeight(const double weight)
{
    // set the PMI weight
    pmiComputationWeight_ = weight;
}

Pmi LteAmc::readWbPmi(const PmiVector& pmi)
{
    // during the process, consider
    // - the pmi value which will be returned
    Pmi pmiRet = NOPMI;
    // - a counter to obtain the mean
    Pmi pmiCounter = NOPMI;
    // - the mean value
    Pmi pmiMean = NOPMI;
    // - the min value
    Pmi pmiMin = NOPMI;
    // - the max value
    Pmi pmiMax = NOPMI;

    // consider the pmi of each band
    unsigned int bands = pmi.size();
    for (Band b = 0; b < bands; ++b)
    {
        pmiCounter += pmi.at(b);
        pmiMin = pmiMin < pmi.at(b) ? pmiMin : pmi.at(b);
        pmiMax = pmiMax > pmi.at(b) ? pmiMax : pmi.at(b);
    }

    // when casting a double to an unsigned int value, consider the closest one

    // is the module lower than the half of the divisor ? ceil, otherwise floor
    pmiMean = (double) (pmiCounter % bands) > (double) bands / 2.0 ? pmiCounter / bands + 1 : pmiCounter / bands;

    // the 0.0 weight is used in order to obtain the mean
    if (pmiComputationWeight_ == 0.0)
        pmiRet = pmiMean;
    // the -1.0 weight is used in order to obtain the min
    else if (pmiComputationWeight_ == -1.0)
        pmiRet = pmiMin;
    // the 1.0 weight is used in order to obtain the max
    else if (pmiComputationWeight_ == 1.0)
        pmiRet = pmiMax;
    // the following weight is used in order to obtain a value between the min and the mean
    else if (-1.0 < pmiComputationWeight_ && pmiComputationWeight_ < 0.0)
    {
        pmiRet = pmiMin;
        // ceil or floor depending on decimal part (casting to unsigned int results in a ceiling)
        double ret = (pmiComputationWeight_ + 1.0) * ((double) pmiMean - (double) pmiMin);
        pmiRet += ret - ((unsigned int) ret) > 0.5 ? (unsigned int) ret + 1 : (unsigned int) ret;
    }
    // the following weight is used in order to obtain a value between the min and the max
    else if (0.0 < pmiComputationWeight_ && pmiComputationWeight_ < 1.0)
    {
        pmiRet = pmiMean;
        // ceil or floor depending on decimal part (casting to unsigned int results in a ceiling)
        double ret = (pmiComputationWeight_) * ((double) pmiMax - (double) pmiMean);
        pmiRet += ret - ((unsigned int) ret) > 0.5 ? (unsigned int) ret + 1 : (unsigned int) ret;
    }
    else
    {
        throw cRuntimeError("LteAmc::readWbPmi(): Unknown weight %d", pmiComputationWeight_);
    }

    EV << "LteAmc::getWbPmi - Pmi " << pmiRet << " evaluated\n";

    return pmiRet;
}

/****************************
 *    Handover support
 ****************************/

void LteAmc::detachUser(MacNodeId nodeId, Direction dir)
{
    EV << "##################################" << endl;
    EV << "# LteAmc::detachUser. Id: " << nodeId << ", direction: " << dirToA(dir) << endl;
    EV << "##################################" << endl;
    try
    {
        ConnectedUesMap *connectedUe;
        std::vector<UserTxParams> *userInfoVec;
        History_ *history;
        std::map<MacNodeId, History_>* d2dHistory;
        unsigned int nodeIndex;



        if(dir==DL)
        {
            connectedUe = &dlConnectedUe_;
            userInfoVec = &dlTxParams_;
            history = &dlFeedbackHistory_;
            nodeIndex = dlNodeIndex_.at(nodeId);
        }
        else if(dir==UL)
        {
            connectedUe = &ulConnectedUe_;
            userInfoVec = &ulTxParams_;
            history = &ulFeedbackHistory_;
            nodeIndex = ulNodeIndex_.at(nodeId);
        }
        else if(dir==D2D)
        {
            connectedUe = &d2dConnectedUe_;
            userInfoVec = &d2dTxParams_;
            d2dHistory = &d2dFeedbackHistory_;
            nodeIndex = d2dNodeIndex_.at(nodeId);
        }
        else
        {
            throw cRuntimeError("LteAmc::detachUser(): Unrecognized direction");
        }
        // UE is no more connected
        (*connectedUe).at(nodeId) = false;

        // clear feedback data from history
        if (dir == UL || dir == DL)
        {
            RemoteSet::iterator it = remoteSet_.begin();
            RemoteSet::iterator et = remoteSet_.end();
            for(; it!=et; it++ )
            {
                (*history).at(*it).at(nodeIndex).clear();
            }
        }
        else   // D2D
        {
            std::map<MacNodeId, History_>::iterator ht =  d2dHistory->begin();
            for (; ht != d2dHistory->end(); ++ht)
            {
                if (ht->first == 0)  // skip fake UE 0
                    continue;

                history = &(ht->second);
                RemoteSet::iterator it = remoteSet_.begin();
                RemoteSet::iterator et = remoteSet_.end();
                for(; it!=et; it++ )
                {
                    (*history).at(*it).at(nodeIndex).clear();
                }
            }
        }
        // clear user transmission parameters for this UE
        (*userInfoVec).at(nodeIndex).restoreDefaultValues();
    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in LteAmc::detachUser(): %s", e.what());
    }
}

void LteAmc::attachUser(MacNodeId nodeId, Direction dir)
{
    EV << "##################################" << endl;
    EV << "# LteAmc::attachUser. Id: " << nodeId << ", direction: " << dirToA(dir) << endl;
    EV << "##################################" << endl;

    ConnectedUesMap *connectedUe;
    std::map<MacNodeId, unsigned int> *nodeIndexMap;
    std::vector<MacNodeId> *revIndexVec;
    std::vector<UserTxParams> *userInfoVec;
    History_ *history;
    std::map<MacNodeId, History_>* d2dHistory;
    unsigned int nodeIndex;
    unsigned int fbhbCapacity;
    unsigned int numTxModes;

    if(dir==DL)
    {
        connectedUe = &dlConnectedUe_;
        nodeIndexMap = &dlNodeIndex_;
        revIndexVec = &dlRevNodeIndex_;
        userInfoVec = &dlTxParams_;
        history = &dlFeedbackHistory_;
        fbhbCapacity = fbhbCapacityDl_;
        numTxModes = DL_NUM_TXMODE;
    }
    else if(dir==UL)
    {
        connectedUe = &ulConnectedUe_;
        nodeIndexMap = &ulNodeIndex_;
        revIndexVec = &ulRevNodeIndex_;
        userInfoVec = &ulTxParams_;
        history = &ulFeedbackHistory_;
        fbhbCapacity = fbhbCapacityUl_;
        numTxModes = UL_NUM_TXMODE;
    }
    else if(dir==D2D)
    {
        connectedUe = &d2dConnectedUe_;
        nodeIndexMap = &d2dNodeIndex_;
        revIndexVec = &d2dRevNodeIndex_;
        userInfoVec = &d2dTxParams_;
        d2dHistory = &d2dFeedbackHistory_;
        fbhbCapacity = fbhbCapacityD2D_;
        numTxModes = UL_NUM_TXMODE;
    }
    else
    {
        throw cRuntimeError("LteAmc::attachUser(): Unrecognized direction");
    }

    // Prepare iterators and empty feedback data
    RemoteSet::iterator it = remoteSet_.begin();
    RemoteSet::iterator et = remoteSet_.end();
    LteSummaryBuffer b = LteSummaryBuffer(fbhbCapacity, MAXCW, numBands_, lb_, ub_);
    std::vector<LteSummaryBuffer> v = std::vector<LteSummaryBuffer>(numTxModes, b);

    // check if the UE is known (it has been here before)
    if( (*connectedUe).find(nodeId) != (*connectedUe).end() )
    {
        EV << "LteAmc::attachUser. Id " << nodeId << " is known (he has been here before)." << endl;

        // user is known, get his index
        nodeIndex = (*nodeIndexMap).at(nodeId);

        // clear user transmission parameters for this UE
        (*userInfoVec).at(nodeIndex).restoreDefaultValues();

        // initialize empty feedback structures
        if (dir == UL || dir == DL)
        {
            for(; it!=et; it++ )
            {
                (*history)[*it].at(nodeIndex) = v;
            }
        }
        else // D2D
        {
            std::map<MacNodeId, History_>::iterator ht = d2dHistory->begin();
            for (; ht != d2dHistory->end(); ++ht)
            {
                if (ht->first == 0)  // skip fake UE 0
                    continue;

                history = &(ht->second);
                RemoteSet::iterator it = remoteSet_.begin();
                RemoteSet::iterator et = remoteSet_.end();
                for(; it!=et; it++ )
                {
                    (*history)[*it].at(nodeIndex) = v;
                }
            }
        }
    }
    else
    {
        EV << "LteAmc::attachUser. Id " << nodeId << " is not known (it is the first time we see him)." << endl;

        // new user: [] operator insert a new element in the map
        (*nodeIndexMap)[nodeId] = (*revIndexVec).size();
        (*revIndexVec).push_back(nodeId);
        (*userInfoVec).push_back(UserTxParams());

        // get newly created index
        nodeIndex = (*nodeIndexMap).at(nodeId);

        // initialize empty feedback structures
        if (dir == UL || dir == DL)
        {
            for(; it!=et; it++ )
            {
                (*history)[*it].push_back(v); // XXX DEBUG THIS!!
            }
        }
        else // D2D
        {
            // initialize an empty feedback for a fake user (id 0), in order to manage
            // the case of transmission before a feedback has been reported
            History_ hist;
            (*d2dHistory)[0] = hist;
            std::map<MacNodeId, History_>::iterator ht =  d2dHistory->begin();
            for (; ht != d2dHistory->end(); ++ht)
            {
                history = &(ht->second);
                RemoteSet::iterator it = remoteSet_.begin();
                RemoteSet::iterator et = remoteSet_.end();
                for(; it!=et; it++ )
                {
                    (*history)[*it].push_back(v); // XXX DEBUG THIS!!
                }
            }
        }
    }
    // Operation done in any case: use [] because new elements may be created
    (*connectedUe)[nodeId] = true;
}

void LteAmc::testUe(MacNodeId nodeId, Direction dir)
{
    EV << "##################################" << endl;
    EV << "LteAmc::testUe (" << dirToA(dir) << ")" << endl;

    ConnectedUesMap *connectedUe;
    std::map<MacNodeId, unsigned int> *nodeIndexMap;
    std::vector<MacNodeId> *revIndexVec;
    std::vector<UserTxParams> *userInfoVec;
    History_ *history;
    std::map<MacNodeId, History_>* d2dHistory;
    int numTxModes;

    if(dir==DL)
    {
        connectedUe = &dlConnectedUe_;
        nodeIndexMap = &dlNodeIndex_;
        revIndexVec = &dlRevNodeIndex_;
        userInfoVec = &dlTxParams_;
        history = &dlFeedbackHistory_;
        numTxModes = DL_NUM_TXMODE;
    }
    else if(dir==UL)
    {
        connectedUe = &ulConnectedUe_;
        nodeIndexMap = &ulNodeIndex_;
        revIndexVec = &ulRevNodeIndex_;
        userInfoVec = &ulTxParams_;
        history = &ulFeedbackHistory_;
        numTxModes = UL_NUM_TXMODE;
    }
    else if(dir==D2D)
    {
        connectedUe = &d2dConnectedUe_;
        nodeIndexMap = &d2dNodeIndex_;
        revIndexVec = &d2dRevNodeIndex_;
        userInfoVec = &d2dTxParams_;
        d2dHistory = &d2dFeedbackHistory_;
        numTxModes = UL_NUM_TXMODE;
    }
    else
    {
        throw cRuntimeError("LteAmc::attachUser(): Unrecognized direction");
    }

    unsigned int nodeIndex = (*nodeIndexMap).at(nodeId);
    bool isConnected = (*connectedUe).at(nodeId);
    MacNodeId revIndex = (*revIndexVec).at(nodeIndex);

    EV << "Id: " << nodeId << endl;
    EV << "Index: " << nodeIndex << endl;
    EV << "Reverse index: " << revIndex << " (should be the same of ID)" << endl;
    EV << "Is connected: " << (isConnected?"TRUE":"FALSE") << endl;

    if(!isConnected)
    return;

    // If connected compute and print user transmission parameters and history
    computeTxParams(nodeId,dir);
    UserTxParams info = (*userInfoVec).at(nodeIndex);
    EV << "UserTxParams" << endl;
    info.print("LteAmc::testUe");

    if (dir == UL || dir == DL)
    {
        RemoteSet::iterator it = remoteSet_.begin();
        RemoteSet::iterator et = remoteSet_.end();
        std::vector<LteSummaryBuffer> feedback;

        EV << "History" << endl;
        for(; it!=et; it++ )
        {
            EV << "Remote: " << dasToA(*it) << endl;
            feedback = (*history).at(*it).at(nodeIndex);
            for(int i=0; i<numTxModes; i++)
            {
                // Print only non empty feedback summary! (all cqi are != NOSIGNALCQI)
                Cqi testCqi = (feedback.at(i).get()).getCqi(Codeword(0),Band(0));
                if(testCqi==NOSIGNALCQI)
                continue;

                feedback.at(i).get().print(0,nodeId,dir,TxMode(i),"LteAmc::testUe");
            }
        }
    }
    else // D2D
    {
        std::map<MacNodeId, History_>::iterator ht =  d2dHistory->begin();
        for (; ht != d2dHistory->end(); ++ht)
        {
            history = &(ht->second);
            RemoteSet::iterator it = remoteSet_.begin();
            RemoteSet::iterator et = remoteSet_.end();
            std::vector<LteSummaryBuffer> feedback;

            EV << "History" << endl;
            for(; it!=et; it++ )
            {
                EV << "Remote: " << dasToA(*it) << endl;
                feedback = (*history).at(*it).at(nodeIndex);
                for(int i=0; i<numTxModes; i++)
                {
                    // Print only non empty feedback summary! (all cqi are != NOSIGNALCQI)
                    Cqi testCqi = (feedback.at(i).get()).getCqi(Codeword(0),Band(0));
                    if(testCqi==NOSIGNALCQI)
                    continue;

                    feedback.at(i).get().print(0,nodeId,dir,TxMode(i),"LteAmc::testUe");
                }
            }
        }
    }
    EV << "##################################" << endl;
}
