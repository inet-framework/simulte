//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTECELLINFO_H_
#define _LTE_LTECELLINFO_H_

#include <string.h>
#include <omnetpp.h>
#include <math.h>
#include "inet/networklayer/contract/ipv4/IPv4Address.h"
#include "stack/phy/das/RemoteAntennaSet.h"
#include "corenetwork/binder/LteBinder.h"
#include "common/LteCommon.h"

class DasFilter;

/**
 * @class LteCellInfo
 * @brief There is one LteCellInfo module for each eNB (thus one for each cell). Keeps cross-layer information about the cell
 */
class LteCellInfo : public cSimpleModule
{
  private:
    /// reference to the global module binder
    LteBinder *binder_;
    /// Remote Antennas for eNB
    RemoteAntennaSet *ruSet_;

    /// Cell Id
    MacCellId cellId_;

    // MACRO_ENB or MICRO_ENB
    EnbType eNbType_;

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
     * This is a virtual deployment: the cellInfo needs only to inform
     * the eNB nic module about the position of the deployed antennas and
     * their TX power. These parameters are configured via the cellInfo, but
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
    // Used by remote Units only
    void calculateNodePosition(double centerX, double centerY, int nTh,
        int totalNodes, int range, double startingAngle, double *xPos,
        double *yPos);

    void createAntennaCwMap();

  public:

    LteCellInfo();

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

    void setEnbType(EnbType t)
    {
        eNbType_ = t;
    }

    EnbType getEnbType()
    {
        return eNbType_;
    }

    inet::Coord getUePosition(MacNodeId id)
    {
        return uePosition[id];
    }
    void setUePosition(MacNodeId id, inet::Coord c)
    {
        uePosition[id] = c;
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
    //---------------------------------------------------------------

    void detachUser(MacNodeId nodeId);
    void attachUser(MacNodeId nodeId);

    ~LteCellInfo();
};

#endif
