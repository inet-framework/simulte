//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEAMC_H_
#define _LTE_LTEAMC_H_

#include <omnetpp.h>

#include "corenetwork/lteCellInfo/LteCellInfo.h"
#include "stack/phy/feedback/LteFeedback.h"
#include "stack/mac/amc/AmcPilot.h"
#include "stack/mac/amc/LteMcs.h"
#include "stack/mac/amc/UserTxParams.h"
#include "corenetwork/binder/LteBinder.h"

/// Forward declaration of AmcPilot class, used by LteAmc.
class AmcPilot;
/// Forward declaration of LteCellInfo class, used by LteAmc.
class LteCellInfo;
/// Forward declaration of LteMacEnb class, used by LteAmc.
class LteMacEnb;

/**
 * @class LteAMC
 * @brief Lte AMC module for Omnet++ simulator
 *
 * TODO
 */
class LteAmc
{
  private:
    AmcPilot *getAmcPilot(const cPar& amcMode);
    MacNodeId getNextHop(MacNodeId dst);
    public:
    void printParameters();
    void printFbhb(Direction dir);
    void printTxParams(Direction dir);
    void printMuMimoMatrix(const char *s);
    protected:
    LteMacEnb *mac_;
    LteBinder *binder_;
    LteCellInfo *cellInfo_;
    AmcPilot *pilot_;
    RbAllocationType allocationType_;
    int numBands_;
    MacNodeId nodeId_;
    MacCellId cellId_;
    McsTable dlMcsTable_;
    McsTable ulMcsTable_;
    McsTable d2dMcsTable_;
    double mcsScaleDl_;
    double mcsScaleUl_;
    double mcsScaleD2D_;
    int numAntennas_;
    RemoteSet remoteSet_;
    Cqi kCqi_;
    ConnectedUesMap dlConnectedUe_;
    ConnectedUesMap ulConnectedUe_;
    ConnectedUesMap d2dConnectedUe_;
    std::map<MacNodeId, unsigned int> dlNodeIndex_;
    std::map<MacNodeId, unsigned int> ulNodeIndex_;
    std::map<MacNodeId, unsigned int> d2dNodeIndex_;
    std::vector<MacNodeId> dlRevNodeIndex_;
    std::vector<MacNodeId> ulRevNodeIndex_;
    std::vector<MacNodeId> d2dRevNodeIndex_;
    std::vector<UserTxParams> dlTxParams_;
    std::vector<UserTxParams> ulTxParams_;
    std::vector<UserTxParams> d2dTxParams_;
    typedef std::map<Remote, std::vector<std::vector<LteSummaryBuffer> > > History_;

    int fType_; //CQI synchronization Debugging
    History_ dlFeedbackHistory_;
    History_ ulFeedbackHistory_;
    std::map<MacNodeId, History_> d2dFeedbackHistory_;
    unsigned int fbhbCapacityDl_;
    unsigned int fbhbCapacityUl_;
    unsigned int fbhbCapacityD2D_;
    simtime_t lb_;
    simtime_t ub_;
    double pmiComputationWeight_;
    double cqiComputationWeight_;
    LteMuMimoMatrix muMimoDlMatrix_;
    LteMuMimoMatrix muMimoUlMatrix_;
    LteMuMimoMatrix muMimoD2DMatrix_;
    public:
    LteAmc(LteMacEnb *mac, LteBinder *binder, LteCellInfo *cellInfo, int numAntennas);
    void initialize();
    ~LteAmc();
    void sefType(int f)
    {
        fType_ = f;
    }
    int getfType()
    {
        return fType_;
    }

    // CodeRate MCS rescaling
    void rescaleMcs(double rePerRb, Direction dir = DL);

    void pushFeedback(MacNodeId id, Direction dir, LteFeedback fb);
    void pushFeedbackD2D(MacNodeId id, LteFeedback fb, MacNodeId peerId);
    LteSummaryFeedback getFeedback(MacNodeId id, Remote antenna, TxMode txMode, const Direction dir);
    LteSummaryFeedback getFeedbackD2D(MacNodeId id, Remote antenna, TxMode txMode, MacNodeId peerId);

    //used when is necessary to know if the requested feedback exists or not
    // LteSummaryFeedback getFeedback(MacNodeId id, Remote antenna, TxMode txMode, const Direction dir,bool& valid);

    MacNodeId computeMuMimoPairing(const MacNodeId nodeId, Direction dir = DL);

    const RemoteSet *getAntennaSet()
    {
        return &remoteSet_;
    }

    Cqi getKCqi()
    {
        return kCqi_;
    }


    bool existTxParams(MacNodeId id, const Direction dir);
    const UserTxParams & getTxParams(MacNodeId id, const Direction dir);
    const UserTxParams & setTxParams(MacNodeId id, const Direction dir, UserTxParams & info);
    const UserTxParams & computeTxParams(MacNodeId id, const Direction dir);
    void cleanAmcStructures(Direction dir, ActiveSet aUser);
    unsigned int computeReqRbs(MacNodeId id, Band b, Codeword cw, unsigned int bytes, const Direction dir);
    unsigned int computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir);
    unsigned int computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir);
    unsigned int computeBytesOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir);
    unsigned int computeBytesOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir);

    // multiband version of the above function. It returns the number of bytes that can fit in the given "blocks" of the given "band"
    unsigned int computeBytesOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir);
    unsigned int computeBitsOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir);
    bool setPilotUsableBands(MacNodeId id , std::vector<unsigned short> usableBands);
    bool getPilotUsableBands(MacNodeId id, std::vector<unsigned short>*& usableBands);

    // utilities - do not involve pilot invocation
    unsigned int getItbsPerCqi(Cqi cqi, const Direction dir);

    double readCoderate(MacNodeId id, Codeword cw, unsigned int bytes, const Direction dir);

    /*
     * Access the correct itbs2tbs conversion table given cqi and layer numer
     */
    const unsigned int* readTbsVect(Cqi cqi, unsigned int layers, Direction dir);

    /*
     * given <cqi> and <layers> returns bytes allocable in <blocks>
     */
    unsigned int blockGain(Cqi cqi, unsigned int layers, unsigned int blocks, Direction dir);

    /*
     * given <cqi> and <layers> returns blocks capable of carrying  <bytes>
     */
    unsigned int bytesGain(Cqi cqi, unsigned int layers, unsigned int bytes, Direction dir);

    // ---------------------------
    void writeCqiWeight(double weight);
    Cqi readWbCqi(const CqiVector & cqi);
    void writePmiWeight(double weight);
    Pmi readWbPmi(const PmiVector & pmi);
    void detachUser(MacNodeId nodeId, Direction dir);
    void attachUser(MacNodeId nodeId, Direction dir);
    void testUe(MacNodeId nodeId, Direction dir);
    AmcPilot *getPilot() const
    {
        return pilot_;
    }

    inet::Coord getUePosition(MacNodeId id)
    {
        return cellInfo_->getUePosition(id);
    }

    void muMimoMatrixInit(Direction dir, MacNodeId nodeId)
    {
        if (dir == DL)
            muMimoDlMatrix_.initialize(nodeId);
        else if (dir == UL)
            muMimoUlMatrix_.initialize(nodeId);
        else if (dir == D2D)
            muMimoD2DMatrix_.initialize(nodeId);
    }
    void addMuMimoPair(Direction dir, MacNodeId id1, MacNodeId id2)
    {
        if (dir == DL)
            muMimoDlMatrix_.addPair(id1, id2);
        else if (dir == UL)
            muMimoUlMatrix_.addPair(id1, id2);
        else if (dir == D2D)
            muMimoD2DMatrix_.addPair(id1,id2);
    }

    std::vector<Cqi>  readMultiBandCqi(MacNodeId id, const Direction dir);

    int getSystemNumBands() { return numBands_; }
};

#endif
