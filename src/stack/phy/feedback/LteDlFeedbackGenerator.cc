//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "stack/phy/layer/LtePhyUe.h"

Define_Module(LteDlFeedbackGenerator);

/*****************************
 *    PRIVATE FUNCTIONS
 *****************************/

void LteDlFeedbackGenerator::createFeedback(FbPeriodicity per)
{
    EV << NOW << " LteDlFeedbackGenerator::createFeedback " << periodicityToA(per) << endl;

    LteFeedbackDoubleVector *fb;

    if (per == PERIODIC)
    {
        fb = &periodicFeedback;
    }
    else if (per == APERIODIC)
    {
        fb = &aperiodicFeedback;
    }
    else
    {
        error("UNKNOWN PERIODICITY");
    }
    if (feedbackComputationPisa_)
    return;
    // Clear feedback map before feedback computation.
    fb->clear();
    //Feedback computation
    //get number of RU
    int nRus = deployer_->getNumRus();
    std::vector<double> dummy;
    EV << NOW << " LteDlFeedbackGenerator::createFeedback " << fbGeneratorTypeToA(generatorType_)
       << " Feedback " << endl;

    if (generatorType_ == IDEAL)
    {
        *fb = lteFeedbackComputation_->computeFeedback(fbType_,
            rbAllocationType_, currentTxMode_, antennaCws_,
            numPreferredBands_, generatorType_, nRus, dummy);
    }
    else if (generatorType_ == REAL)
    {
        EV << NOW << " Remote Units: " << nRus << endl;
        //for each RU is called the computation feedback function
        RemoteSet::iterator it;
        fb->resize(dasFilter_->getReportingSet().size());
        for (it = dasFilter_->getReportingSet().begin();
            it != dasFilter_->getReportingSet().end(); ++it)
        {
            (*fb)[(*it)].resize((int) currentTxMode_);
            (*fb)[(*it)][(int) currentTxMode_] =
            lteFeedbackComputation_->computeFeedback(*it,
                currentTxMode_, fbType_, rbAllocationType_,
                antennaCws_[*it], numPreferredBands_,
                generatorType_, nRus, dummy);
        }
    }
    // the reports are computed only for the antenna in the reporting set
    else if (generatorType_ == DAS_AWARE)
    {
        EV << NOW << " Remote Units: " << nRus << endl;
        RemoteSet::iterator it;
        fb->resize(dasFilter_->getReportingSet().size());
        for (it = dasFilter_->getReportingSet().begin();
            it != dasFilter_->getReportingSet().end(); ++it)
        {
            (*fb)[(*it)] = lteFeedbackComputation_->computeFeedback(*it,
                fbType_, rbAllocationType_, currentTxMode_,
                antennaCws_[*it], numPreferredBands_, generatorType_, nRus,
                dummy);
        }
    }
    EV << "LteDlFeedbackGenerator::createFeedback Feedback Generated for nodeId: " << nodeId_
       << " with generator type " << fbGeneratorTypeToA(generatorType_)
       << " Fb size: " << fb->size() << endl;
}

    /******************************
     *    PROTECTED FUNCTIONS
     ******************************/

void LteDlFeedbackGenerator::initialize(int stage)
{
    EV << "DlFeedbackGenerator stage: " << stage << endl;
    if (stage == 0)
    {
        // Read NED parameters
        fbPeriod_ = (simtime_t)(int(par("fbPeriod")) * TTI);// TTI -> seconds
        fbDelay_ = (simtime_t)(int(par("fbDelay")) * TTI);// TTI -> seconds
        if (fbPeriod_ <= fbDelay_)
        {
            error("Feedback Period MUST be greater than Feedback Delay");
        }
        fbType_ = getFeedbackType(par("feedbackType").stringValue());
        rbAllocationType_ = getRbAllocationType(
            par("rbAllocationType").stringValue());
        usePeriodic_ = par("usePeriodic");
        currentTxMode_ = aToTxMode(par("initialTxMode"));

        generatorType_ = getFeedbackGeneratorType(
            par("feedbackGeneratorType").stringValue());

        masterId_ = getAncestorPar("masterId");
        nodeId_ = getAncestorPar("macNodeId");

        /** Initialize timers **/

        tPeriodicSensing_ = new TTimer(this);
        tPeriodicSensing_->setTimerId(PERIODIC_SENSING);

        tPeriodicTx_ = new TTimer(this);
        tPeriodicTx_->setTimerId(PERIODIC_TX);

        tAperiodicTx_ = new TTimer(this);
        tAperiodicTx_->setTimerId(APERIODIC_TX);
        feedbackComputationPisa_ = false;
        WATCH(fbType_);
        WATCH(rbAllocationType_);
        WATCH(fbPeriod_);
        WATCH(fbDelay_);
        WATCH(usePeriodic_);
        WATCH(currentTxMode_);
    }
    else if (stage == 1)
    {
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " init" << endl;
        deployer_ = getDeployer(masterId_);
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " deployer taken" << endl;
        antennaCws_ = deployer_->getAntennaCws();
        numBands_ = deployer_->getNumBands();
        numPreferredBands_ = deployer_->getNumPreferredBands();
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " used deployer: bands " << numBands_ << " preferred bands "
           << numPreferredBands_ << endl;
        LtePhyUe* tmp = dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule(
                "phy"));
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " phyUe taken" << endl;
        dasFilter_ = tmp->getDasFilter();
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " phyUe used" << endl;
        initializeFeedbackComputation(par("feedbackComputation").xmlValue());
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " feedback computation initialize" << endl;
        WATCH(numBands_);
        WATCH(numPreferredBands_);
        if (usePeriodic_)
        {
            tPeriodicSensing_->start(0);
        }
    }
}

void LteDlFeedbackGenerator::handleMessage(cMessage *msg)
{
    TTimerMsg *tmsg = check_and_cast<TTimerMsg*>(msg);
    FbTimerType type = (FbTimerType) tmsg->getTimerId();

    if (type == PERIODIC_SENSING)
    {
        EV << NOW << " Periodic Sensing" << endl;
        tPeriodicSensing_->handle();
        tPeriodicSensing_->start(fbPeriod_);
        sensing(PERIODIC);
    }
    else if (type == PERIODIC_TX)
    {
        EV << NOW << " Periodic Tx" << endl;
        tPeriodicTx_->handle();
        sendFeedback(periodicFeedback, PERIODIC);
    }
    else if (type == APERIODIC_TX)
    {
        EV << NOW << " Aperiodic Tx" << endl;
        tAperiodicTx_->handle();
        sendFeedback(aperiodicFeedback, APERIODIC);
    }
    delete tmsg;
}

void LteDlFeedbackGenerator::sensing(FbPeriodicity per)
{
    if (per == PERIODIC && tAperiodicTx_->busy()
        && tAperiodicTx_->elapsed() < 0.001)
    {
        /* In this TTI an APERIODIC sensing has been done
         * (an APERIODIC tx is scheduled).
         * Ignore this PERIODIC request.
         */
        EV << NOW << " Aperiodic before Periodic in the same TTI: ignore Periodic" << endl;
        return;
    }

    if (per == APERIODIC && tAperiodicTx_->busy())
    {
        /* An APERIODIC tx is already scheduled:
         * ignore this request.
         */
        EV << NOW << " Aperiodic overlapping: ignore second Aperiodic" << endl;
        return;
    }

    if (per == APERIODIC && tPeriodicTx_->busy()
        && tPeriodicTx_->elapsed() < 0.001)
    {
        /* In this TTI a PERIODIC sensing has been done.
         * Deschedule the PERIODIC tx and continue with APERIODIC.
         */
        EV << NOW << " Periodic before Aperiodic in the same TTI: remove Periodic" << endl;
        tPeriodicTx_->stop();
    }

    // Create feedback
    createFeedback(per);

    // Schedule feedback transmission
    if (per == PERIODIC)
    tPeriodicTx_->start(fbDelay_);
    else if (per == APERIODIC)
    tAperiodicTx_->start(fbDelay_);
}

        /***************************
         *    PUBLIC FUNCTIONS
         ***************************/

LteDlFeedbackGenerator::LteDlFeedbackGenerator()
{
    tPeriodicSensing_ = NULL;
    tPeriodicTx_ = NULL;
    tAperiodicTx_ = NULL;
}

LteDlFeedbackGenerator::~LteDlFeedbackGenerator()
{
    delete tPeriodicSensing_;
    delete tPeriodicTx_;
    delete tAperiodicTx_;
}

void LteDlFeedbackGenerator::aperiodicRequest()
{
    Enter_Method("aperiodicRequest()");
    EV << NOW << " Aperiodic request" << endl;
    sensing(APERIODIC);
}

void LteDlFeedbackGenerator::setTxMode(TxMode newTxMode)
{
    Enter_Method("setTxMode()");
    currentTxMode_ = newTxMode;
}

void LteDlFeedbackGenerator::sendFeedback(LteFeedbackDoubleVector fb,
    FbPeriodicity per)
{
    EV << "sendFeedback() in DL" << endl;
    EV << "Periodicity: " << periodicityToA(per) << " nodeId: " << nodeId_ << endl;

    FeedbackRequest feedbackReq;
    if (feedbackComputationPisa_)
    {
        feedbackReq.request = true;
        feedbackReq.genType = getFeedbackGeneratorType(
            getAncestorPar("feedbackGeneratorType").stringValue());
        feedbackReq.type = getFeedbackType(par("feedbackType").stringValue());
        feedbackReq.txMode = currentTxMode_;
        feedbackReq.rbAllocationType = rbAllocationType_;
    }
    else
    {
        feedbackReq.request = false;
    }
    //use PHY function to send feedback
    (dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("phy")))->sendFeedback(
        fb, fb, feedbackReq);
}

// TODO adjust default value
LteFeedbackComputation* LteDlFeedbackGenerator::getFeedbackComputationFromName(
    std::string name, ParameterMap& params)
{
    ParameterMap::iterator it;
    if (name == "DUMMY")
    {
        feedbackComputationPisa_ = false;
        //default value
        double channelVariationProb = 0.1;
        it = params.find("variation");
        if (it != params.end())
        {
            channelVariationProb = params["variation"].doubleValue();
        }
        int channelVariationStep = 2;
        it = params.find("maxStepVariation");
        if (it != params.end())
        {
            channelVariationStep = (int) params["maxStepVariation"].longValue();
        }
        int txDivMin = 2;
        it = params.find("txDivMin");
        if (it != params.end())
        {
            txDivMin = (int) params["txDivMin"].longValue();
        }
        int txDivMax = 7;
        it = params.find("txDivMax");
        if (it != params.end())
        {
            txDivMax = (int) params["txDivMax"].longValue();
        }
        int sMuxMin = -5;
        it = params.find("sMuxMin");
        if (it != params.end())
        {
            sMuxMin = (int) params["sMuxMin"].longValue();
        }
        int sMuxMax = -2;
        it = params.find("sMuxMax");
        if (it != params.end())
        {
            sMuxMax = (int) params["sMuxMax"].longValue();
        }
        int muMimoMin = -2;
        it = params.find("muMimoMin");
        if (it != params.end())
        {
            muMimoMin = (int) params["muMimoMin"].longValue();
        }
        int muMimoMax = 0;
        it = params.find("muMimoMax");
        if (it != params.end())
        {
            muMimoMax = (int) params["muMimoMax"].longValue();
        }
        double rankVariation = 0;
        it = params.find("rankVariationProb");
        if (it != params.end())
        {
            rankVariation = params["rankVariationProb"].doubleValue();
        }
        LteFeedbackComputation* fbcomp = new LteFeedbackComputationDummy(
            channelVariationProb, channelVariationStep, txDivMin, txDivMax,
            sMuxMin, sMuxMax, muMimoMin, muMimoMax, rankVariation,
            numBands_);
        return fbcomp;
    }
    else if (name == "REAL")
    {
        feedbackComputationPisa_ = true;
        return 0;
    }
    else
        return 0;
}

void LteDlFeedbackGenerator::initializeFeedbackComputation(
    cXMLElement* xmlConfig)
{
    lteFeedbackComputation_ = 0;

    if (xmlConfig == 0)
        throw cRuntimeError("No feedback computation configuration file specified.");

    cXMLElementList fbComputationList = xmlConfig->getElementsByTagName("FeedbackComputation");

    if (fbComputationList.empty())
        throw cRuntimeError("No feedback computation configuration found in configuration file.");

    if (fbComputationList.size() > 1)
        throw cRuntimeError("More than one feedback computation configuration found in configuration file.");

    cXMLElement* fbComputationData = fbComputationList.front();

    const char* name = fbComputationData->getAttribute("type");

    if (name == 0)
        throw cRuntimeError("Could not read type of feedback computation from configuration file.");

    ParameterMap params;
    getParametersFromXML(fbComputationData, params);

    lteFeedbackComputation_ = getFeedbackComputationFromName(name, params);

    if (lteFeedbackComputation_ == 0 && !feedbackComputationPisa_)
        throw cRuntimeError("Could not find a feedback computation with the name \"%s\".", name);

    EV << "Feedback Computation \"" << name << "\" loaded." << endl;
}

void LteDlFeedbackGenerator::handleHandover(MacCellId newEnbId)
{
    masterId_ = newEnbId;
    deployer_ = getDeployer(masterId_);

    EV << NOW << " LteDlFeedbackGenerator::handleHandover - Master ID updated to " << masterId_ << endl;
}
