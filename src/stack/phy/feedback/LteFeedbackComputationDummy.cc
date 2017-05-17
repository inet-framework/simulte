//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/phy/feedback/LteFeedbackComputationDummy.h"
#include "stack/phy/feedback/LteFeedback.h"

LteFeedbackComputationDummy::LteFeedbackComputationDummy(double channelVariationProb, int channelVariationStep,
    int txDivMin, int txDivMax, int sMuxMin, int sMuxMax, int muMimoMin, int muMimoMax, double rankVariationProb,
    int numband)
{
    channelVariationProb_ = channelVariationProb;
    channelVariationStep_ = channelVariationStep;
    txDivMin_ = txDivMin;
    txDivMax_ = txDivMax;
    sMuxMin_ = sMuxMin;
    sMuxMax_ = sMuxMax;
    muMimoMin_ = muMimoMin;
    muMimoMax_ = muMimoMax;
    rankVariationProb_ = rankVariationProb;
    lastCqi_.resize(numband, 15);
    lastRank_ = 1;
    numBands_ = numband;
}

LteFeedbackComputationDummy::~LteFeedbackComputationDummy()
{
}

LteFeedbackDoubleVector LteFeedbackComputationDummy::computeFeedback(FeedbackType fbType,
    RbAllocationType rbAllocationType,
    TxMode currentTxMode, std::map<Remote, int> antennaCws, int numPreferredBands,
    FeedbackGeneratorType feedbackGeneratortype, int numRus, std::vector<double> snr, MacNodeId id)
{
    //add enodeB to the number of antenna
    numRus++;
    // New Feedback
    LteFeedbackDoubleVector fbvv;
    fbvv.resize(numRus);
    //resize all the vectors
    for (int i = 0; i < numRus; i++)
        fbvv[i].resize(DL_NUM_TXMODE);
    //for each Remote
    for (int j = 0; j < numRus; j++)
    {
        // Generate a new Rank
        throwRank();
        //Generate a new cqi for each band and for each cw
        throwCqi(fbType, numBands_);
        LteFeedback fb;
        //for each txmode we generate a feedback
        for (int z = 0; z < DL_NUM_TXMODE; z++)
        {
            //reset the feedback object
            fb.reset();
            if (z == OL_SPATIAL_MULTIPLEXING || z == CL_SPATIAL_MULTIPLEXING)
            {
                if (lastRank_ == 1)
                {
                    fbvv[j][z] = fb;
                    continue;
                }
                //set the rank
                fb.setRankIndicator(lastRank_);
            }
            else
                fb.setRankIndicator(1);
            //set the remote
            fb.setAntenna((Remote) j);
            //set the pmi
            fb.setWideBandPmi(intuniform(getEnvir()->getRNG(0), 1, pow(lastRank_, (double) 2)));

            //generate feedback for txmode z
            generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws[(Remote) j], rbAllocationType,
                (TxMode) z);
            // add the feedback to the feedback structure
            fbvv[j][z] = fb;
        }
    }
    return fbvv;
}

LteFeedbackVector LteFeedbackComputationDummy::computeFeedback(const Remote remote, FeedbackType fbType,
    RbAllocationType rbAllocationType,
    TxMode currentTxMode, int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype,
    int numRus, std::vector<double> snr, MacNodeId id)
{
    // New Feedback
    LteFeedbackVector fbv;
    //resize
    fbv.resize(DL_NUM_TXMODE);
    // Generate a new Rank
    throwRank();
    //Generate a new cqi for each band and for each cw
    throwCqi(fbType, numBands_);
    LteFeedback fb;
    //for each txmode we generate a feedback
    for (int z = 0; z < DL_NUM_TXMODE; z++)
    {
        //reset the feedback object
        fb.reset();
        if (z == OL_SPATIAL_MULTIPLEXING || z == CL_SPATIAL_MULTIPLEXING)
        {
            if (lastRank_ == 1)
            {
                fbv[z] = fb;
                continue;
            }
            //set the rank
            fb.setRankIndicator(lastRank_);
        }
        else
            fb.setRankIndicator(1);
        //set the remote
        fb.setAntenna(remote);
        //set the pmi
        fb.setWideBandPmi(intuniform(getEnvir()->getRNG(0), 1, pow(lastRank_, (double) 2)));
        //set the rank
        fb.setRankIndicator(lastRank_);
        //generate feedback for txmode z
        generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws, rbAllocationType, (TxMode) z);
        // add the feedback to the feedback structure
        fbv[z] = fb;
    }
    return fbv;
}

LteFeedback LteFeedbackComputationDummy::computeFeedback(const Remote remote, TxMode txmode, FeedbackType fbType,
    RbAllocationType rbAllocationType,
    int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
    std::vector<double> snr, MacNodeId id)
{
    // New Feedback
    LteFeedback fb;
    // Generate a new Rank
    throwRank();
    // set the rank for all the tx mode except for SISO, and Tx div
    if (txmode == OL_SPATIAL_MULTIPLEXING || txmode == CL_SPATIAL_MULTIPLEXING)
    {
        if (lastRank_ == 1)
            return fb;
        //Set Rank
        fb.setRankIndicator(lastRank_);
    }
    //Generate a new cqi for each band and for each cw
    throwCqi(fbType, numBands_);
    //set the remote in feedback object
    fb.setAntenna(remote);
    generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws, rbAllocationType, txmode);
    // set pmi only for cl smux and mumimo
    if (txmode == CL_SPATIAL_MULTIPLEXING || txmode == MULTI_USER)
    {
        //Set PMI
        fb.setWideBandPmi(intuniform(getEnvir()->getRNG(0), 1, pow(lastRank_, (double) 2)));
    }
    return fb;
}

void LteFeedbackComputationDummy::throwRank()
{
    // rank selection
    double rankVariation = uniform(getEnvir()->getRNG(0), 0.0, 1.0);
    int newrank;
    //Test if we should change rank
    if (rankVariation <= rankVariationProb_)
    {
        newrank = intuniform(getEnvir()->getRNG(0), 1, 2);
        //if Rank was 1 then new rank will be 2 or 4
        if (lastRank_ == 1)
            lastRank_ = newrank * 2;
        //If rank was 2...
        else if (lastRank_ == 2)
        {
            //then new rank will be 1
            if (newrank == 1)
                lastRank_ = 1;
            //or 4
            else
                lastRank_ = 4;
        }
        else if (lastRank_ == 4)
        {
            //if Rank was 4 then new rank will be 1 or 2
            lastRank_ = newrank;
        }
    }
}

void LteFeedbackComputationDummy::throwCqi(FeedbackType fbType, int numBands_)
{
    double channelVariation;
    // we generate a cqi for each band
    for (int i = 0; i < numBands_; i++)
    {
        channelVariation = uniform(getEnvir()->getRNG(0), 0.0, 1.0);
        // The channel will change
        if (channelVariation < channelVariationProb_)
        {
            // the channel will change according to the ned parameters
            int shift = intuniform(getEnvir()->getRNG(0), (channelVariationStep_ * (-1)), channelVariationStep_);
            if ((int) lastCqi_[i] + shift > 15)
                lastCqi_[i] = 15;
            else if ((int) lastCqi_[i] + shift < 0)
                lastCqi_[i] = 0;
            else
                lastCqi_[i] += shift;
        }
        //if fbtype is wideband only one cqi will eventually change
        if (fbType == WIDEBAND && i == 0)
            break;
    }
}

void LteFeedbackComputationDummy::generateBaseFeedback(int numBands, int numPreferredBands,
    LteFeedback& fb, FeedbackType fbType, int cw, RbAllocationType rbAllocationType, TxMode txmode)
{
    std::vector<int> newCqi;
    if (txmode == CL_SPATIAL_MULTIPLEXING || txmode == OL_SPATIAL_MULTIPLEXING)
        newCqi.resize(cw);
    else
        newCqi.resize(1);
    std::vector<CqiVector> cqiTemp;
    cqiTemp.resize(cw);
    unsigned int j;
    for (j = 0; j < newCqi.size(); j++)
    {
        // reset cqi vector for each cw
        long int cc;
        cqiTemp[j].resize(numBands, 0);
        for (int i = 0; i < numBands; i++)
        {
            // we change cqi for each band only if allocation type is localized and
            // only for one  band for distributed
            if ((rbAllocationType == TYPE2_DISTRIBUTED && i == 0) || rbAllocationType == TYPE2_LOCALIZED)
            {
                // recompute the cqi variation according to the txmode used
                if (txmode == TRANSMIT_DIVERSITY)
                {
                    newCqi[j] = intuniform(getEnvir()->getRNG(0), txDivMin_, txDivMax_);
                }
                else if (txmode == SINGLE_ANTENNA_PORT0)
                {
                    newCqi[j] = 0;
                }
                else if (txmode == CL_SPATIAL_MULTIPLEXING)
                {
                    cc = intuniform(getEnvir()->getRNG(0), sMuxMin_, sMuxMax_);
                    if (j == 1)
                        newCqi[j] = newCqi[0];
                    newCqi[j] = cc * -1;
                }
                else if (txmode == OL_SPATIAL_MULTIPLEXING)
                {
                    cc = intuniform(getEnvir()->getRNG(0), sMuxMin_, sMuxMax_);
                    if (j == 1)
                        newCqi[j] = newCqi[0];
                    newCqi[j] = cc * -1;
                }
                else if (txmode == MULTI_USER)
                {
                    cc = intuniform(getEnvir()->getRNG(0), muMimoMin_, muMimoMax_);
                    newCqi[j] = cc;
                }
                else if (txmode == SINGLE_ANTENNA_PORT5)
                {
                    newCqi[j] = 0;
                }
                else
                {
                    throw cRuntimeError("Wrong TxMode %d", txmode);
                }
            }
            fb.setTxMode(txmode);
            // set new cqi but only if greater than zero
            if ((int) lastCqi_[i] + newCqi[j] >= 15)
                cqiTemp[j][i] = 15;
            else if ((int) lastCqi_[i] + newCqi[j] < 0)
                cqiTemp[j][i] = 0;
            else
                cqiTemp[j][i] = lastCqi_[i] + newCqi[j];
        }
    }
    if (fbType == ALLBANDS)
    {
        for (j = 0; j < newCqi.size(); j++)
            // set the feedback for each band and for each cw
            fb.setPerBandCqi(cqiTemp[j], j);
    }
    else if (fbType == PREFERRED)
    {
        //TODO:: DA VERIFICARE DA VERIFICARE E COMMENTARE
        CqiVector tmp;
        tmp.resize(numBands, 0);
        BandSet bands;
        int max;
        int pos;
        int total;
        //std::sort(cqiTemp.begin(),cqiTemp.end(), greater<int>());
        for (int j = 0; j < numPreferredBands; j++)
        {
            max = 0;
            pos = 0;
            total = 0;
            for (int i = 0; i < numBands; i++)
            {
                for (int z = 0; z < cw; z++)
                    total += cqiTemp[z][i];
                if (total > max)
                {
                    max = total;
                    pos = i;
                }
                total = 0;
            }
            tmp[pos] = max;
            for (unsigned int z = 0; j < newCqi.size(); z++)
                cqiTemp[z][pos] = 0;
            bands.insert(pos);
        }
        //TODO Chiedere come funziona il preffered
        fb.setPreferredCqi(tmp);
        fb.setPreferredBands(bands);
    }
    else if (fbType == WIDEBAND)
    {
        for (j = 0; j < newCqi.size(); j++)
            fb.setWideBandCqi(cqiTemp[j][0], j);
    }
}
