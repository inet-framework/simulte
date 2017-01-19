//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/phy/layer/LtePhyEnb.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/phy/das/DasFilter.h"
#include "common/LteCommon.h"

Define_Module(LtePhyEnb);

LtePhyEnb::LtePhyEnb()
{
    das_ = NULL;
    bdcStarter_ = NULL;
}

LtePhyEnb::~LtePhyEnb()
{
    cancelAndDelete(bdcStarter_);
    if(lteFeedbackComputation_){
        delete lteFeedbackComputation_;
        lteFeedbackComputation_ = NULL;
    }
    if(das_){
        delete das_;
        das_ = NULL;
    }
}

DasFilter* LtePhyEnb::getDasFilter()
{
    return das_;
}

void LtePhyEnb::initialize(int stage)
{
    LtePhyBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL)
    {
        // get local id
        nodeId_ = getAncestorPar("macNodeId");
        EV << "Local MacNodeId: " << nodeId_ << endl;
        std::cout << "Local MacNodeId: " << nodeId_ << endl;

        nodeType_ = ENODEB;
        deployer_ = getDeployer(nodeId_);
        deployer_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
        das_ = new DasFilter(this, binder_, deployer_->getRemoteAntennaSet(), 0);

        WATCH(nodeType_);
        WATCH(das_);
    }
    else if (stage == 1)
    {
        initializeFeedbackComputation(par("feedbackComputation").xmlValue());

        //check eNb type and set TX power
        if (deployer_->getEnbType() == MICRO_ENB)
            txPower_ = microTxPower_;
        else
            txPower_ = eNodeBtxPower_;

        // set TX direction
        std::string txDir = par("txDirection");
        if (txDir.compare(txDirections[OMNI].txDirectionName)==0)
        {
            txDirection_ = OMNI;
        }
        else   // ANISOTROPIC
        {
            txDirection_ = ANISOTROPIC;

            // set TX angle
            txAngle_ = par("txAngle");
        }

        bdcUpdateInterval_ = deployer_->par("broadcastMessageInterval");
        if (bdcUpdateInterval_ != 0 && par("enableHandover").boolValue()) {
            // self message provoking the generation of a broadcast message
            bdcStarter_ = new cMessage("bdcStarter");
            scheduleAt(NOW, bdcStarter_);
        }
    }
}

void LtePhyEnb::handleSelfMessage(cMessage *msg)
{
    if (msg->isName("bdcStarter"))
    {
        // send broadcast message
        LteAirFrame *f = createHandoverMessage();
        sendBroadcast(f);
        scheduleAt(NOW + bdcUpdateInterval_, msg);
    }
    else
    {
        delete msg;
    }
}

bool LtePhyEnb::handleControlPkt(UserControlInfo* lteinfo, LteAirFrame* frame)
{
    EV << "Received control pkt " << endl;
    MacNodeId senderMacNodeId = lteinfo->getSourceId();
    try
    {
        binder_->getOmnetId(senderMacNodeId);
    }
    catch (std::out_of_range& e)
    {
        std::cerr << "Sender (" << senderMacNodeId << ") does not exist anymore!" << std::endl;
        delete frame;
        return true;    // FIXME ? make sure that nodes that left the simulation do not send
    }
    if (lteinfo->getFrameType() == HANDOVERPKT)
    {
        // handover broadcast frames must not be relayed or processed by eNB
        delete frame;
        return true;
    }
    // send H-ARQ feedback up
    if (lteinfo->getFrameType() == HARQPKT
        || lteinfo->getFrameType() == RACPKT)
    {
        handleControlMsg(frame, lteinfo);
        return true;
    }
    //handle feedback pkt
    if (lteinfo->getFrameType() == FEEDBACKPKT)
    {
        handleFeedbackPkt(lteinfo, frame);
        delete frame;
        return true;
    }
    return false;
}

void LtePhyEnb::handleAirFrame(cMessage* msg)
{
    UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->removeControlInfo());
    if (!lteInfo)
    {
        return;
    }

    LteAirFrame* frame = static_cast<LteAirFrame*>(msg);

    EV << "LtePhy: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    // handle broadcast packet sent by another eNB
    if (lteInfo->getFrameType() == HANDOVERPKT)
    {
        EV << "LtePhyEnb::handleAirFrame - received handover packet from another eNodeB. Ignore it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    /*
     * This could happen if the ue associates with a new master while it has
     * already scheduled a packet for the old master: the packet is in the air
     * while the ue changes master.
     * Event timing:      TTI x: packet scheduled and sent by the UE (tx time = 1ms)
     *                     TTI x+0.1: ue changes master
     *                     TTI x+1: packet from UE arrives at the old master
     */
    if (binder_->getNextHop(lteInfo->getSourceId()) != nodeId_)
    {
        EV << "WARNING: frame from a UE that is leaving this cell (handover): deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "Master MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    connectedNodeId_ = lteInfo->getSourceId();

    int sourceId = getBinder()->getOmnetId(connectedNodeId_);
    int senderId = getBinder()->getOmnetId(lteInfo->getDestId());
    if(sourceId == 0 || senderId == 0){
        // either source or destination have left the simulation
        delete msg;
        return;
    }

    //handle all control pkt
    if (handleControlPkt(lteInfo, frame))
        return; // If frame contain a control pkt no further action is needed

    bool result = true;
    RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
    if (r.size() > 1)
    {
        // Use DAS
        // Message from ue
        for (RemoteSet::iterator it = r.begin(); it != r.end(); it++)
        {
            EV << "LtePhy: Receiving Packet from antenna " << (*it) << "\n";

            /*
             * On eNodeB set the current position
             * to the receiving das antenna
             */
            //move.setStart(
            cc->setRadioPosition(myRadioRef, das_->getAntennaCoord(*it));

            RemoteUnitPhyData data;
            data.txPower = lteInfo->getTxPower();
            data.m = getRadioPosition();
            frame->addRemoteUnitPhyDataVector(data);
        }
        result = channelModel_->errorDas(frame, lteInfo);
    }
    else
    {
        result = channelModel_->error(frame, lteInfo);
    }
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    cPacket* pkt = frame->decapsulate();

    // here frame has to be destroyed since it is no more useful
    delete frame;

    // attach the decider result to the packet as control info
    lteInfo->setDeciderResult(result);
    pkt->setControlInfo(lteInfo);

    // send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
    updateDisplayString();
}
void LtePhyEnb::requestFeedback(UserControlInfo* lteinfo, LteAirFrame* frame,
    LteFeedbackPkt* pkt)
{
    EV << NOW << " LtePhyEnb::requestFeedback " << endl;
    //get UE Position
    Coord sendersPos = lteinfo->getCoord();
    deployer_->setUePosition(lteinfo->getSourceId(), sendersPos);

    //Apply analog model (pathloss)
    //Get snr for UL direction
    std::vector<double> snr = channelModel_->getSINR(frame, lteinfo);
    FeedbackRequest req = lteinfo->feedbackReq;
    //Feedback computation
    fb_.clear();
    //get number of RU
    int nRus = deployer_->getNumRus();
    TxMode txmode = req.txMode;
    FeedbackType type = req.type;
    RbAllocationType rbtype = req.rbAllocationType;
    std::map<Remote, int> antennaCws = deployer_->getAntennaCws();
    unsigned int numPreferredBand = deployer_->getNumPreferredBands();
    for (Direction dir = UL; dir != UNKNOWN_DIRECTION;
        dir = ((dir == UL )? DL : UNKNOWN_DIRECTION))
    {
        //for each RU is called the computation feedback function
        if (req.genType == IDEAL)
        {
            fb_ = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                antennaCws, numPreferredBand, IDEAL, nRus, snr,
                lteinfo->getSourceId());
        }
        else if (req.genType == REAL)
        {
            RemoteSet::iterator it;
            fb_.resize(das_->getReportingSet().size());
            for (it = das_->getReportingSet().begin();
                it != das_->getReportingSet().end(); ++it)
            {
                fb_[(*it)].resize((int) txmode);
                fb_[(*it)][(int) txmode] =
                lteFeedbackComputation_->computeFeedback(*it, txmode,
                    type, rbtype, antennaCws[*it], numPreferredBand,
                    REAL, nRus, snr, lteinfo->getSourceId());
            }
        }
        // the reports are computed only for the antenna in the reporting set
        else if (req.genType == DAS_AWARE)
        {
            RemoteSet::iterator it;
            fb_.resize(das_->getReportingSet().size());
            for (it = das_->getReportingSet().begin();
                it != das_->getReportingSet().end(); ++it)
            {
                fb_[(*it)] = lteFeedbackComputation_->computeFeedback(*it, type,
                    rbtype, txmode, antennaCws[*it], numPreferredBand,
                    DAS_AWARE, nRus, snr, lteinfo->getSourceId());
            }
        }
        if (dir == UL)
        {
            pkt->setLteFeedbackDoubleVectorUl(fb_);
            //Prepare  parameters for next loop iteration - in order to compute SNR in DL
            lteinfo->setTxPower(txPower_);
            lteinfo->setDirection(DL);

            //Get snr for DL direction
            snr = channelModel_->getSINR(frame, lteinfo);
        }
        else
        pkt->setLteFeedbackDoubleVectorDl(fb_);
    }
    EV << "LtePhyEnb::requestFeedback : Pisa Feedback Generated for nodeId: "
       << nodeId_ << " with generator type "
       << fbGeneratorTypeToA(req.genType) << " Fb size: " << fb_.size() << endl;
}

void LtePhyEnb::handleFeedbackPkt(UserControlInfo* lteinfo,
    LteAirFrame *frame)
{
    EV << "Handled Feedback Packet with ID " << frame->getId() << endl;
    LteFeedbackPkt* pkt = check_and_cast<LteFeedbackPkt*>(frame->decapsulate());
    // here frame has to be destroyed since it is no more useful
    pkt->setControlInfo(lteinfo);
    // if feedback was generated by dummy phy we can send up to mac else nodeb should generate the "real" feddback
    if (lteinfo->feedbackReq.request)
    {
        requestFeedback(lteinfo, frame, pkt);
        //DEBUG
        LteFeedbackDoubleVector::iterator it;
        LteFeedbackVector::iterator jt;
        LteFeedbackDoubleVector vec = pkt->getLteFeedbackDoubleVectorDl();
        for (it = vec.begin(); it != vec.end(); ++it)
        {
            for (jt = it->begin(); jt != it->end(); ++jt)
            {
                MacNodeId id = lteinfo->getSourceId();
                EV << endl << "Node:" << id << endl;
                TxMode t = jt->getTxMode();
                EV << "TXMODE: " << txModeToA(t) << endl;
                if (jt->hasBandCqi())
                {
                    std::vector<CqiVector> vec = jt->getBandCqi();
                    std::vector<CqiVector>::iterator kt;
                    CqiVector::iterator ht;
                    int i;
                    for (kt = vec.begin(); kt != vec.end(); ++kt)
                    {
                        for (i = 0, ht = kt->begin(); ht != kt->end();
                            ++ht, i++)
                        EV << "Banda " << i << " Cqi " << *ht << endl;
                    }
                }
                else if (jt->hasWbCqi())
                {
                    CqiVector v = jt->getWbCqi();
                    CqiVector::iterator ht = v.begin();
                    for (; ht != v.end(); ++ht)
                    EV << "wb cqi " << *ht << endl;
                }
                if (jt->hasRankIndicator())
                {
                    EV << "Rank " << jt->getRankIndicator() << endl;
                }
            }
        }
    }
    // send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);
}

// TODO adjust default value
LteFeedbackComputation* LtePhyEnb::getFeedbackComputationFromName(
    std::string name, ParameterMap& params)
{
    ParameterMap::iterator it;
    if (name == "REAL")
    {
        //default value
        double targetBler = 0.1;
        double lambdaMinTh = 0.02;
        double lambdaMaxTh = 0.2;
        double lambdaRatioTh = 20;
        it = params.find("targetBler");
        if (it != params.end())
        {
            targetBler = params["targetBler"].doubleValue();
        }
        it = params.find("lambdaMinTh");
        if (it != params.end())
        {
            lambdaMinTh = params["lambdaMinTh"].doubleValue();
        }
        it = params.find("lambdaMaxTh");
        if (it != params.end())
        {
            lambdaMaxTh = params["lambdaMaxTh"].doubleValue();
        }
        it = params.find("lambdaRatioTh");
        if (it != params.end())
        {
            lambdaRatioTh = params["lambdaRatioTh"].doubleValue();
        }
        LteFeedbackComputation* fbcomp = new LteFeedbackComputationRealistic(
            targetBler, deployer_->getLambda(), lambdaMinTh, lambdaMaxTh,
            lambdaRatioTh, deployer_->getNumBands());
        return fbcomp;
    }
    else
        return 0;
}

void LtePhyEnb::initializeFeedbackComputation(cXMLElement* xmlConfig)
{
    lteFeedbackComputation_ = 0;

    if (xmlConfig == 0)
    {
        error("No feedback computation configuration file specified.");
        return;
    }

    cXMLElementList fbComputationList = xmlConfig->getElementsByTagName(
        "FeedbackComputation");

    if (fbComputationList.empty())
    {
        error(
            "No feedback computation configuration found in configuration file.");
        return;
    }

    if (fbComputationList.size() > 1)
    {
        error(
            "More than one feedback computation configuration found in configuration file.");
        return;
    }

    cXMLElement* fbComputationData = fbComputationList.front();

    const char* name = fbComputationData->getAttribute("type");

    if (name == 0)
    {
        error(
            "Could not read type of feedback computation from configuration file.");
        return;
    }

    ParameterMap params;
    getParametersFromXML(fbComputationData, params);

    lteFeedbackComputation_ = getFeedbackComputationFromName(name, params);

    EV << "Feedback Computation \"" << name << "\" loaded." << endl;
}

//void LtePhyEnb::setMicroTxPower()
//{
//    EV << "set Tx power for Micro eNb to " << microTxPower_ << endl;
//    txPower_ = microTxPower_;
//}

