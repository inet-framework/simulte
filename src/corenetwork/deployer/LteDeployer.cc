//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteDeployer.h"
#include "world/radio/ChannelControl.h"
#include "world/radio/ChannelAccess.h"
#include "StationaryMobility.h"

Define_Module(LteDeployer);

// TODO: mettere parametro numEnbAntennas da passare a LteMcs::cwMapping (@par antennas)

LteDeployer::LteDeployer()
{
    numRelays_ = 0;
    relayDistance_ = 0;
    nodeX_ = 0;
    nodeY_ = 0;
    nodeZ_ = 0;
    binder_ = NULL;
    mcsScaleDl_ = 0;
    mcsScaleUl_ = 0;
    numRus_ = 0;
    ruSet_ = new RemoteAntennaSet();
    binder_ = getBinder();
}

LteDeployer::~LteDeployer()
{
    binder_ = NULL;
    delete ruSet_;
}

void LteDeployer::preInitialize()
{
//    StationaryMobility* bm =
//        dynamic_cast<StationaryMobility*>(getParentModule()->getModuleByPath(".nic.mobility"));

//    if(eNbType_ == MACRO_ENB)
//    {
//        nodeX_ = bm->par("initialX");
//        nodeY_ = bm->par("initialY");
//    }
//    /// TODO implement a micro node deployment method
//    else //eNbType_ == MICRO_ENB
//    {
//        nodeX_ = 1200;
//        nodeY_ = 1200;
//        bm->par("initialX")=nodeX_;
//        bm->par("initialY")=nodeY_;
//    }

    pgnMinX_ = par("constraintAreaMinX");
    pgnMinY_ = par("constraintAreaMinY");
    pgnMaxX_ = par("constraintAreaMaxX");
    pgnMaxY_ = par("constraintAreaMaxY");

    int ruRange = par("ruRange");
    double nodebTxPower = getAncestorPar("txPower");
    eNbType_ = par("microCell").boolValue() ? MICRO_ENB : MACRO_ENB;
    numRbDl_ = par("numRbDl");
    numRbUl_ = par("numRbUl");
    rbyDl_ = par("rbyDl");
    rbyUl_ = par("rbyUl");
    rbxDl_ = par("rbxDl");
    rbxUl_ = par("rbxUl");
    rbPilotDl_ = par("rbPilotDl");
    rbPilotUl_ = par("rbPilotUl");
    signalDl_ = par("signalDl");
    signalUl_ = par("signalUl");
    numBands_ = par("numBands");
    numRus_ = par("numRus");

    numPreferredBands_ = par("numPreferredBands");

    if (numRus_ > NUM_RUS)
        throw cRuntimeError("The number of Antennas specified exceeds the limit of %d", NUM_RUS);

    EV << "Deployer: eNB coordinates: " << nodeX_ << " " << nodeY_ << " "
       << nodeZ_ << endl;
    EV << "Deployer: playground size: " << pgnMinX_ << "," << pgnMinY_ << " - "
       << pgnMaxX_ << "," << pgnMaxY_ << " " << endl;

    // register the containing eNB  to the binder
    cellId_ = getParentModule()->par("macCellId");

    // first RU to be registered is the MACRO
    ruSet_->addRemoteAntenna(nodeX_, nodeY_, nodebTxPower);

    // deploy RUs
    deployRu(nodeX_, nodeY_, numRus_, ruRange);

    // MCS scaling factor
    calculateMCSScale(&mcsScaleUl_, &mcsScaleDl_);

    createAntennaCwMap();
}

void LteDeployer::initialize()
{
    preInitialize();
    return;
}

int LteDeployer::deployRelays(double startAngle, int i, int num, double *xPos,
    double *yPos)
{
    double startingAngle = startAngle;
    *xPos = 0;
    *yPos = 0;
    unsigned int nRelays = num;
    int relayNodeId = 0;
    calculateNodePosition(nodeX_, nodeY_, i, nRelays, relayDistance_,
        startingAngle, xPos, yPos);
    EV << "Deployer: Relay " << i << " will be deployed at (x,y): " << *xPos
       << ", " << *yPos << endl;
    relayNodeId = createRelayAt(*xPos, *yPos);

    return relayNodeId;
}

cModule* LteDeployer::deployUes(double centerX, double centerY, int Ue,
    int totalNumOfUes, std::string mobType, int range, uint16_t masterId,
    double speed)
{
    double x = 0;
    double y = 0;
    calculateNodePosition(centerX, centerY, Ue, totalNumOfUes, range, 0, &x, &y);

    EV << NOW << " LteDeployer::deployUes deploying Ues: centerX " << centerX
       << " centerY " << centerY << " Ue " << Ue << " total "
       << totalNumOfUes << " x " << x << " y " << y << endl;

    cModule* p = createUeAt(x, y, mobType, centerX, centerY, masterId, speed);
    return p;
}

void LteDeployer::calculateNodePosition(double centerX, double centerY, int nTh,
    int totalNodes, int range, double startingAngle, double *xPos,
    double *yPos)
{
    if (totalNodes == 0)
        error("LteDeployer::calculateNodePosition: divide by 0");
    // radians (minus sign because position 0,0 is top-left, not bottom-left)
    double theta = -startingAngle * M_PI / 180;

    double thetaSpacing = (2 * M_PI) / totalNodes;
    // angle of n-th node
    theta += nTh * thetaSpacing;
    double x = centerX + (range * cos(theta));
    double y = centerY + (range * sin(theta));

    *xPos = (x < pgnMinX_) ? pgnMinX_ : (x > pgnMaxX_) ? pgnMaxX_ : x;
    *yPos = (y < pgnMinY_) ? pgnMinY_ : (y > pgnMaxY_) ? pgnMaxY_ : y;

    EV << NOW << " LteDeployer::calculateNodePosition: Computed node position "
       << *xPos << " , " << *yPos << endl;

    return;
}

uint16_t LteDeployer::createRelayAt(double x, double y)
{
    cModuleType *mt = cModuleType::get("lte.corenetwork.nodes.Relay");
    // parent module is the network itself
    cModule *module = mt->create("relay", getParentModule()->getParentModule());
    module->par("xPos") = x;
    module->par("yPos") = y;
    module->par("masterId") = getAncestorPar("macNodeId");
    module->par("macCellId") = cellId_;
    MacNodeId enbId = getAncestorPar("macNodeId");
    MacNodeId relayNodeId = binder_->registerNode(module, RELAY, enbId);
    module->finalizeParameters();
    module->buildInside();
    initializeAllChannels(module);
    module->scheduleStart(simTime());
    return relayNodeId;
}

cModule* LteDeployer::createUeAt(double x, double y, std::string mobType, /*std::string appType,*/
double centerX, double centerY, MacNodeId masterId, double speed)
{
    cModuleType *mt = cModuleType::get("lte.corenetwork.nodes.Ue");
    cModule *module = mt->create("ue", getParentModule()->getParentModule());
    module->par("masterId") = masterId;
    module->par("macCellId") = cellId_;
    binder_->registerNode(module, UE, masterId);
    module->finalizeParameters();
    module->buildInside();
    initializeAllChannels(module);
    module->scheduleStart(simTime());

    attachMobilityModule(module, mobType, x, y, centerX, centerY, speed);
    return module;
}

void LteDeployer::attachMobilityModule(cModule *parentModule,
    std::string mobType, double x, double y, double centerX, double centerY,
    double speed)
{
    EV << NOW << " LteDeployer::attachMobilityModule called with parameters X "
       << x << " Y " << y << " center X " << centerX << " center Y "
       << centerY << " speed  " << speed << endl;

    // circular
    if (mobType.at(0) == 'c')
    {
        cModuleType *mt = cModuleType::get("inet.mobility.models.CircleMobility");
        cModule *module = mt->create("mobility",
            parentModule->getSubmodule("nic"));
        module->par("debug") = false;
        module->par("cx") = centerX;
        module->par("cy") = centerY;
        module->par("cz") = 0;
        double r = sqrt(pow(fabs(centerX - x), 2) + pow(fabs(centerY - y), 2));
        module->par("r") = r;
        EV << NOW
           << " LteDeployer::attachMobilityModule configured with radius "
           << r << endl;
        module->par("speed") = speed;
        module->par("startAngle") = uniform(0, 360);
        module->par("updateInterval") = par("positionUpdateInterval");
        module->finalizeParameters();
        module->buildInside();
        module->scheduleStart(simTime());
    }

    // linear
    else if (mobType.at(0) == 'l')
    {
        cModuleType *mt = cModuleType::get("inet.mobility.models.ConstSpeedMobility");
        cModule *module = mt->create("mobility",
            parentModule->getSubmodule("nic"));
        module->par("initialX") = x;
        module->par("initialY") = y;
        module->par("initialZ") = 0;
        module->par("initFromDisplayString") = false;
        module->par("debug") = false;
        module->par("speed") = par("speed");
        module->par("updateInterval") = par("positionUpdateInterval");
        module->finalizeParameters();
        module->buildInside();
        module->scheduleStart(simTime());
    }

    // null (default)
    else
    {
        cModuleType *mt = cModuleType::get("inet.mobility.models.StationaryMobility");
        cModule *module = mt->create("mobility", parentModule);
        module->par("initialX") = x;
        module->par("initialY") = y;
        module->par("initialZ") = 0;
        module->par("initFromDisplayString") = false;
        module->finalizeParameters();
        module->buildInside();
        module->scheduleStart(simTime());
    }
}

void LteDeployer::deployRu(double nodeX, double nodeY, int numRu, int ruRange)
{
    if (numRu == 0)
        return;
    double x = 0;
    double y = 0;
    double angle = par("ruStartingAngle");
    std::string txPowersString = par("ruTxPower");
    int *txPowers = new int[numRu];
    parseStringToIntArray(txPowersString, txPowers, numRu, 0);
    for (int i = 0; i < numRu; i++)
    {
        calculateNodePosition(nodeX, nodeY, i, numRu, ruRange, angle, &x, &y);
        ruSet_->addRemoteAntenna(x, y, (double) txPowers[i]);
    }
    delete[] txPowers;
}

void LteDeployer::calculateMCSScale(double *mcsUl, double *mcsDl)
{
    // RBsubcarriers * (TTISymbols - SignallingSymbols) - pilotREs
    int ulRbSubcarriers = par("rbyUl");
    int dlRbSubCarriers = par("rbyDl");
    int ulRbSymbols = par("rbxUl");
    ulRbSymbols *= 2; // slot --> RB
    int dlRbSymbols = par("rbxDl");
    dlRbSymbols *= 2; // slot --> RB
    int ulSigSymbols = par("signalUl");
    int dlSigSymbols = par("signalDl");
    int ulPilotRe = par("rbPilotUl");
    int dlPilotRe = par("rbPilotDl");

    *mcsUl = ulRbSubcarriers * (ulRbSymbols - ulSigSymbols) - ulPilotRe;
    *mcsDl = dlRbSubCarriers * (dlRbSymbols - dlSigSymbols) - dlPilotRe;
    return;
}

void LteDeployer::updateMCSScale(double *mcs, double signalRe,
    double signalCarriers, Direction dir)
{
    // RBsubcarriers * (TTISymbols - SignallingSymbols) - pilotREs

    int rbSubcarriers = ((dir == DL) ? par("rbyDl") : par("rbyUl"));
    int rbSymbols = ((dir == DL) ? par("rbxDl") : par("rbxUl"));

    rbSymbols *= 2; // slot --> RB

    int sigSymbols = signalRe;
    int pilotRe = signalCarriers;

    *mcs = rbSubcarriers * (rbSymbols - sigSymbols) - pilotRe;
    return;
}

void LteDeployer::createAntennaCwMap()
{
    std::string cws = par("antennaCws");
    // values for the RUs including the MACRO
    int dim = numRus_ + 1;
    int *values = new int[dim];
    // default for missing values is 1
    parseStringToIntArray(cws, values, dim, 1);
    for (int i = 0; i < dim; i++)
    {
        antennaCws_[(Remote) i] = values[i];
    }
    delete[] values;
}

// TODO: dynamic application creation and user attachment
/**
 * distinzione fra applicazioni dl e ul, con contatori da passare ad amc
 * che distinguono fra numero di app ul e dl (al momento il contatore e' globale)
 */

void LteDeployer::detachUser(MacNodeId nodeId)
{
    // remove UE from deployer's structures

    std::map<MacNodeId, Coord>::iterator pt = uePosition.find(nodeId);
    if (pt != uePosition.end())
        uePosition.erase(pt);

    std::map<MacNodeId, Lambda>::iterator lt = lambdaMap_.find(nodeId);
    if (lt != lambdaMap_.end())
        lambdaMap_.erase(lt);
}

void LteDeployer::attachUser(MacNodeId nodeId)
{
    // add UE to deployer's structures (lambda maps)
    // position will be added by the eNB while computing feedback

    int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
    lambdaInit(nodeId, index);
}
