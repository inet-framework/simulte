//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEDEPLOYER_H_
#define _LTE_LTEDEPLOYER_H_

#include <string.h>
#include <omnetpp.h>
#include <math.h>
#include "inet/networklayer/contract/ipv4/IPv4Address.h"
#include "stack/phy/das/RemoteAntennaSet.h"
#include "corenetwork/binder/LteBinder.h"
#include "common/LteCommon.h"

class DasFilter;

/**
 * @class LteDeployer
 * @brief Lte network elements dynamic deployer. There is one deplayer for each eNB (thus one for each cell)
 *
 *
 * eNB module which can dynamically create UEs and relays
 * and can deploy them into the playground.
 *
 * Keeps general information about the cell
 */
// TODO move all the parameters to their own modules
class LteDeployer : public cSimpleModule
{
  private:
    /// reference to the global module binder
    LteBinder *binder_;
    /// Remote Antennas for eNB
    RemoteAntennaSet *ruSet_;

    /// Cell Id
    MacCellId cellId_;

    //------------- INTERCELL INTERFERENCE SUPPORT ------------------
    /*
     * reference to macro Enb. Used for interference computation purposes.
     * NULL if this is a macro node,
     */
    EnbInfo *refEnb_;

    /*
     * list of micro eNbs within a cell.
     * empty if this is a micro node
     */
    std::vector<EnbInfo*> microList_;

    // MACRO_ENB or MICRO_ENB
    EnbType eNbType_;
    //---------------------------------------------------------------

    /// x playground lower bound
    double pgnMinX_;
    /// y playground lower bound
    double pgnMinY_;
    /// x playground upper bound
    double pgnMaxX_;
    /// y playground upper bound
    double pgnMaxY_;
    /// z playground size
//    double pgnZ_;
    /// x eNB position
    double nodeX_;
    /// y eNB position
    double nodeY_;
    /// z eNB position
    double nodeZ_;

    /// number fo relays attached to eNB
    int numRelays_;
    /// relay distance from eNB
    int relayDistance_;

    /// Number of DAS RU
    int numRus_;
    /// Remote and its CW
    std::map<Remote, int> antennaCws_;

    /// number of cell logical bands
    int numBands_;
    /// number of preferred bands to use (meaningful only in PREFERRED mode)
    int numPreferredBands_;

    /// number of RB, DL
    int numRbDl_;
    /// number of RB, UL
    int numRbUl_;
    /// number of sub-carriers per RB, DL
    int rbyDl_;
    /// number of sub-carriers per RB, UL
    int rbyUl_;
    /// number of OFDM symbols per slot, DL
    int rbxDl_;
    /// number of OFDM symbols per slot, UL
    int rbxUl_;
    /// number of pilot REs per RB, DL
    int rbPilotDl_;
    /// number of pilot REs per RB, UL
    int rbPilotUl_;
    /// number of signaling symbols for RB, DL
    int signalDl_;
    /// number of signaling symbols for RB, UL
    int signalUl_;
    /// MCS scale UL
    double mcsScaleUl_;
    /// MCS scale DL
    double mcsScaleDl_;

    // Position of each UE
    std::map<MacNodeId, inet::Coord> uePosition;

    std::map<MacNodeId, Lambda> lambdaMap_;
    protected:

    virtual void initialize();

    virtual void handleMessage(cMessage *msg)
    {
    }

    /**
     * Deploys remote antennas.
     *
     * This is a virtual deployment: the deployer needs only to inform
     * the eNB nic module about the position of the deployed antennas and
     * their TX power. These parameters are configured via the deployer, but
     * no NED module is created here.
     *
     * @param nodeX x coordinate of the center of the master node
     * @param nodeY y coordinate of the center of the master node
     * @param numRu number of remote units to be deployed
     * @param ruRange distance between eNB and RUs
     */
    virtual void deployRu(double nodeX, double nodeY, int numRu, int ruRange);
    virtual void calculateMCSScale(double *mcsUl, double *mcsDl);
    virtual void updateMCSScale(double *mcs, double signalRe,
        double signalCarriers = 0, Direction dir = DL);

  private:

    /**
     * Relay creation.
     *
     * Dinamically creates a relay node, set its parameters, registers it to the binder
     * and initializes its channels.
     *
     * @param x x coordinate of the center of the master node
     * @param y y coordinate of the center of the master node
     * @param nUes number of UEs attached to this relay
     * @return relay macNodeId assigned by the binder
     */
    uint16_t createRelayAt(double x, double y);

    /**
     * Calculates node position around a circumference.
     *
     * @param centerX x coordinate of the center
     * @param centerY y coordinate of the center
     * @param nTh ordering number of the UE to be placed in this circumference
     * @param totalNodes total number of nodes that will be placed
     * @param range circumference range
     * @param startingAngle angle of the first deployed node (degrees)
     * @param[out] xPos calculated x coordinate
     * @param[out] yPos calculated y coordinate
     */
    void calculateNodePosition(double centerX, double centerY, int nTh,
        int totalNodes, int range, double startingAngle, double *xPos,
        double *yPos);

    /**
     * UE creation.
     *
     * Dynamically creates a UE node, set its parameters, registers it to the binder,
     * initializes its channels and adds the mobility module.
     *
     * @param x x coordinate of the UE
     * @param y y coordinate of the UE
     * @param mobType mobility module to be used for this user, configured via NED string between
     *        3 values: static, circular, linear
     * @param appType is the application type configured via NED. At the
     *        moment only available are: UDPBasicApp, UDPSink
     * @param centerX x coordinate of the UE
     * @param centerY y coordinate of the UE
     * @param masterId ID of the master node
     */
    cModule* createUeAt(double x, double y, std::string mobType, double centerX,
        double centerY, MacNodeId masterId, double speed);

    /**
     * Attaches the mobility module to a UE module.
     *
     * @param parentModule module to which attach mobility
     * @param mobType mobility module type
     * @param x UE's position
     * @param y UE's position
     * @param centerX x center position (used for circle mobility)
     * @param centerY y center position (used for circle mobility)
     */
    void attachMobilityModule(cModule *parentModule, std::string mobType,
        double x, double y, double centerX, double centerY, double speed);

    void createAntennaCwMap();

  public:
    /**
     * All of the deployer operations are performed in pre-initialization
     */
    virtual void preInitialize();

    LteDeployer();

    /**
     * Relay deployment.
     *
     * It deploys numRelays_ relays (ned parameter),
     * each one with a number of associated UEs read from relayUes string ned parameter.
     * Relays are deployed at a relayDistance_ distance from eNB (ned parameter),
     * and are equally spaced over the circumference perimeter.
     */
    virtual int deployRelays(double startAngle, int i, int num, double *xPos,
        double *yPos);

    /**
     * Deploys UEs around a given center.
     *
     * @param centerX x coordinate of the center of the master node
     * @param centerY y coordinate of the center of the master node
     * @param nUes number of UEs to be deployed
     * @param mobType mobility module to be used for this user, configured via NED string between
     *        3 values: static, circular, linear
     * @param appType is the application type configured via NED. At the
     *        moment only availables are: UDPBasicApp, UDPSink
     * @param range distance between center and UEs
     * @param masterId MacNodeId of the master of this UE
     */
    virtual cModule* deployUes(double centerX, double centerY, int Ue,
        int totalNumOfUes, std::string mobType, int range,
        uint16_t masterId, double speed);

    // Getters
    int getNumRbDl()
    {
        return numRbDl_;
    }
    int getNumRbUl()
    {
        return numRbUl_;
    }
    int getRbyDl()
    {
        return rbyDl_;
    }
    int getRbyUl()
    {
        return rbyUl_;
    }
    int getRbxDl()
    {
        return rbxDl_;
    }
    int getRbxUl()
    {
        return rbxUl_;
    }
    int getRbPilotDl()
    {
        return rbPilotDl_;
    }
    int getRbPilotUl()
    {
        return rbPilotUl_;
    }
    int getSignalDl()
    {
        return signalDl_;
    }
    int getSignalUl()
    {
        return signalUl_;
    }
    int getNumBands()
    {
        return numBands_;
    }
    double getMcsScaleUl()
    {
        return mcsScaleUl_;
    }
    double getMcsScaleDl()
    {
        return mcsScaleDl_;
    }
    int getNumRus()
    {
        return numRus_;
    }

    MacCellId getCellId()
    {
        return cellId_;
    }

    std::map<Remote, int> getAntennaCws()
    {
        return antennaCws_;
    }

    int getNumPreferredBands()
    {
        return numPreferredBands_;
    }

    RemoteAntennaSet* getRemoteAntennaSet()
    {
        return ruSet_;
    }
//MODIFICATO
    double getNodeX()
    {
        return nodeX_;
    }

    double getNodeY()
    {
        return nodeY_;
    }
    inet::Coord getUePosition(MacNodeId id)
    {
        return uePosition[id];
    }
    void setUePosition(MacNodeId id, inet::Coord c)
    {
        uePosition[id] = c;
    }

    // changes eNb position (used for micro deployment)
    void setEnbPosition(inet::Coord c)
    {
        nodeX_ = c.x;
        nodeY_ = c.y;
    }

    void lambdaUpdate(MacNodeId id, unsigned int index)
    {
        lambdaMap_[id].lambdaMax = binder_->phyPisaData.getLambda(index, 0);
        lambdaMap_[id].index = index;
        lambdaMap_[id].lambdaMin = binder_->phyPisaData.getLambda(index, 1);
        lambdaMap_[id].lambdaRatio = binder_->phyPisaData.getLambda(index, 2);
    }
    void lambdaIncrease(MacNodeId id, unsigned int i)
    {
        lambdaMap_[id].index = lambdaMap_[id].lambdaStart + i;
        lambdaUpdate(id, lambdaMap_[id].index);
    }
    void lambdaInit(MacNodeId id, unsigned int i)
    {
        lambdaMap_[id].lambdaStart = i;
        lambdaMap_[id].index = lambdaMap_[id].lambdaStart;
        lambdaUpdate(id, lambdaMap_[id].index);
    }
    void channelUpdate(MacNodeId id, unsigned int in)
    {
        unsigned int index = in % binder_->phyPisaData.maxChannel2();
        lambdaMap_[id].channelIndex = index;
    }
    void channelIncrease(MacNodeId id)
    {
        unsigned int i = getNumBands();
        channelUpdate(id, lambdaMap_[id].channelIndex + i);
    }
    Lambda* getLambda(MacNodeId id)
    {
        return &(lambdaMap_.at(id));
    }

    std::map<MacNodeId, Lambda>* getLambda()
    {
        return &lambdaMap_;
    }

    //------------- INTERCELL INTERFERENCE SUPPORT ------------------
    bool setMacroNode(MacNodeId id, cModule * eNodeB)
    {
        refEnb_ = new EnbInfo();
        refEnb_->id = id;            // cell ID
        refEnb_->type = MACRO_ENB;    // eNb Type
        refEnb_->init = false;        // flag for phy initialization

        refEnb_->eNodeB = eNodeB;    // reference to the Macro Node
        refEnb_->x2 = eNodeB->findGate("x2");    // gate for X2 communications

        // TODO add node type check
        return true;
    }

    EnbInfo * getMacroNode()
    {
        return refEnb_;
    }

    bool addMicroNode(MacNodeId id, cModule * eNodeB)
    {
        EnbInfo * elem = new EnbInfo();
        elem->id = id;             // cell ID
        elem->type = MICRO_ENB;    // eNb Type
        elem->init = false;        // flag for phy initialization

        elem->eNodeB = eNodeB;     // reference to the Micro Node
        elem->x2 = eNodeB->findGate("x2"); // gate for X2 communications

        microList_.push_back(elem);

        // TODO add node type check
        return true;
    }

    void setEnbType(EnbType t)
    {
        eNbType_ = t;
    }

    EnbType getEnbType()
    {
        return eNbType_;
    }

    std::vector<EnbInfo*> * getMicroList()
    {
        return &microList_;
    }
    //---------------------------------------------------------------

    void detachUser(MacNodeId nodeId);
    void attachUser(MacNodeId nodeId);

    ~LteDeployer();
};

#endif
