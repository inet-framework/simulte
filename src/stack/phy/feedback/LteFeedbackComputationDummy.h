//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEFEEDBACKCOMPUTATIONDUMMY_H_
#define _LTE_LTEFEEDBACKCOMPUTATIONDUMMY_H_

#include "stack/phy/feedback/LteFeedbackComputation.h"

class LteFeedbackComputationDummy : public LteFeedbackComputation
{
  protected:
    //last advertised cqi
    CqiVector lastCqi_;
    //Channel variation probability
    double channelVariationProb_;
    //Channel Variation Step
    int channelVariationStep_;
    //TxDiv minimum gain
    int txDivMin_;
    //TxDiv Maximum gain
    int txDivMax_;
    //SMux minimum gain
    int sMuxMin_;
    //Smux Maximum gain
    int sMuxMax_;
    //MuMIMO minimum gain
    int muMimoMin_;
    //MuMIMO Maximum gain
    int muMimoMax_;
    // rank variation probability
    double rankVariationProb_;
    // current rank
    Rank lastRank_;
    unsigned int numBands_;
    // Cqi computation
    void throwCqi(FeedbackType fbType, int numBands);
    // Rank computation
    void throwRank();
    // Generate base feedback for all types of feedback(allbands, preferred, wideband)
    void generateBaseFeedback(int numBands, int numPreferredBabds, LteFeedback& fb, FeedbackType fbType, int cw,
        RbAllocationType rbAllocationType, TxMode txmode);
    public:
    LteFeedbackComputationDummy(double channelVariationProb, int channelVariationStep, int txDivMin, int txDivMax,
        int sMuxMin, int sMuxMax, int muMimoMin, int muMimoMax, double rankVariationProb, int numband);
    virtual ~LteFeedbackComputationDummy();
    /**
     * Performs Random Feedback computation using Pagano's rules
     *
     * @return Vector of Vector of LteFeedback indexes: Ru and Txmode
     */
    virtual LteFeedbackDoubleVector computeFeedback(FeedbackType fbType, RbAllocationType rbAllocationType,
        TxMode currentTxMode,
        std::map<Remote, int> antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype,
        int numRus, std::vector<double> snr, MacNodeId id = 0);
    /**
     * Performs Random Feedback computation using Pagano's rules
     *
     * @param Remote antenna remote
     * @return Vector of LteFeedback indexes: Txmode
     */
    virtual LteFeedbackVector computeFeedback(const Remote remote, FeedbackType fbType,
        RbAllocationType rbAllocationType, TxMode currentTxMode,
        int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
        std::vector<double> snr, MacNodeId id = 0);
    /**
     * Performs Random Feedback computation using Pagano's rules
     *
     * @param Remote antenna remote
     * @param Txmode txmode
     * @return  LteFeedback
     */
    virtual LteFeedback computeFeedback(const Remote remote, TxMode txmode, FeedbackType fbType,
        RbAllocationType rbAllocationType,
        int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
        std::vector<double> snr, MacNodeId id = 0);
};

#endif
