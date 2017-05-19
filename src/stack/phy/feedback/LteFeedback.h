//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEFEEDBACK_H_
#define _LTE_LTEFEEDBACK_H_

#include "common/LteCommon.h"
#include "stack/mac/amc/UserTxParams.h"
#include <map>
#include <vector>

class LteFeedback;
typedef std::vector<LteFeedback> LteFeedbackVector;
typedef std::vector<LteFeedbackVector> LteFeedbackDoubleVector;

//! LTE feedback message exchanged between PHY and AMC.
class LteFeedback
{
  protected:
    //! Feedback status type enumerator.
    enum
    {
        EMPTY = 0x0000,
        RANK_INDICATION = 0x0001,
        WB_CQI = 0x0002,
        WB_PMI = 0x0004,
        BAND_CQI = 0x0008,
        BAND_PMI = 0x0010,
        PREFERRED_CQI = 0x0020,
        PREFERRED_PMI = 0x0040
    };

    //! Rank indicator.
    Rank rank_;

    //! Wide-band CQI, one per codeword.
    CqiVector wideBandCqi_;
    //! Wide-band PMI.
    Pmi wideBandPmi_;

    //! Per-band CQI. (bandCqi_[cw][band])
    std::vector<CqiVector> perBandCqi_;
    //! Per-band PMI.
    PmiVector perBandPmi_;

    //! Per subset of preferred bands CQI.
    CqiVector preferredCqi_;
    //! Per subset of preferred bands PMI.
    Pmi preferredPmi_;
    //! Set of preferred bands.
    BandSet preferredBands_;

    //! Current status.
    unsigned int status_;
    //! Current transmission mode.
    TxMode txMode_;

    //! Periodicity of the feedback message.
    bool periodicFeedback_;
    //! \test DAS SUPPORT - Antenna identifier
    Remote remoteAntennaId_;

  public:

    //! Create an empty feedback message.
    LteFeedback()
    {
        status_ = EMPTY;
        txMode_ = SINGLE_ANTENNA_PORT0;
        periodicFeedback_ = true;
        remoteAntennaId_ = MACRO;
    }

    //! Reset this feedback message as empty.
    void reset()
    {
        wideBandCqi_.clear();
        perBandCqi_.clear();
        preferredCqi_.clear();
        preferredBands_.clear();

        status_ = EMPTY;
        periodicFeedback_ = true;
        txMode_ = SINGLE_ANTENNA_PORT0;
        remoteAntennaId_ = MACRO;
    }

    //! Return true if feedback is empty.
    bool isEmptyFeedback() const
    {
        return !(status_);
    }
    //! Return true if is periodic.
    bool isPeriodicFeedback() const
    {
        return periodicFeedback_;
    }

    /// check existence of elements
    bool hasRankIndicator() const
    {
        return (status_ & RANK_INDICATION);
    }
    bool hasWbCqi() const
    {
        return (status_ & WB_CQI);
    }
    bool hasWbPmi() const
    {
        return (status_ & WB_PMI);
    }
    bool hasBandCqi() const
    {
        return (status_ & BAND_CQI);
    }
    bool hasBandPmi() const
    {
        return (status_ & BAND_PMI);
    }
    bool hasPreferredCqi() const
    {
        return (status_ & PREFERRED_CQI);
    }
    bool hasPreferredPmi() const
    {
        return (status_ & PREFERRED_PMI);
    }

    // **************** GETTERS ****************
    //! Get the rank indication. Does not check if valid.
    Rank getRankIndicator() const
    {
        return rank_;
    }
    //! Get the wide-band CQI. Does not check if valid.
    CqiVector getWbCqi() const
    {
        return wideBandCqi_;
    }
    //! Get the wide-band CQI for one codeword. Does not check if valid.
    Cqi getWbCqi(Codeword cw) const
    {
        return wideBandCqi_[cw];
    }
    //! Get the wide-band PMI. Does not check if valid.
    Pmi getWbPmi() const
    {
        return wideBandPmi_;
    }
    //! Get the per-band CQI. Does not check if valid.
    std::vector<CqiVector> getBandCqi() const
    {
        return perBandCqi_;
    }
    //! Get the per-band CQI for one codeword. Does not check if valid.
    CqiVector getBandCqi(Codeword cw) const
    {
        return perBandCqi_[cw];
    }
    //! Get the per-band PMI. Does not check if valid.
    PmiVector getBandPmi() const
    {
        return perBandPmi_;
    }
    //! Get the per preferred band CQI. Does not check if valid.
    CqiVector getPreferredCqi() const
    {
        return preferredCqi_;
    }
    //! Get the per preferred band CQI for one codeword. Does not check if valid.
    Cqi getPreferredCqi(Codeword cw) const
    {
        return preferredCqi_[cw];
    }
    //! Get the per preferred band PMI. Does not check if valid.
    Pmi getPreferredPmi() const
    {
        return preferredPmi_;
    }
    //! Get the set of preferred bands. Does not check if valid.
    BandSet getPreferredBands() const
    {
        return preferredBands_;
    }
    //! Get the transmission mode.
    TxMode getTxMode() const
    {
        return txMode_;
    }
    //! \test DAS support - Get the remote Antenna identifier
    Remote getAntennaId() const
    {
        return remoteAntennaId_;
    }
    // *****************************************

    // **************** SETTERS ****************
    //! Set the rank indicator.
    void setRankIndicator(const Rank rank)
    {
        rank_ = rank;
        status_ |= RANK_INDICATION;
    }
    //! Set the wide-band CQI.
    void setWideBandCqi(const CqiVector wbCqi)
    {
        wideBandCqi_ = wbCqi;
        status_ |= WB_CQI;
    }
    //! Set the wide-band CQI for one codeword. Does not check if valid.
    void setWideBandCqi(const Cqi cqi, const Codeword cw)
    {
        if (wideBandCqi_.size() == cw)
            wideBandCqi_.push_back(cqi);
        else
            wideBandCqi_[cw] = cqi;

        status_ |= WB_CQI;
    }
    //! Set the wide-band PMI.
    void setWideBandPmi(const Pmi wbPmi)
    {
        wideBandPmi_ = wbPmi;
        status_ |= WB_PMI;
    }
    //! Set the per-band CQI.
    void setPerBandCqi(const std::vector<CqiVector> bandCqi)
    {
        perBandCqi_ = bandCqi;
        status_ |= BAND_CQI;
    }
    //! Set the per-band CQI for one codeword. Does not check if valid.
    void setPerBandCqi(const CqiVector bandCqi, const Codeword cw)
    {
        if (perBandCqi_.size() == cw)
            perBandCqi_.push_back(bandCqi);
        else
            perBandCqi_[cw] = bandCqi;

        status_ |= BAND_CQI;
    }
    //! Set the per-band PMI.
    void setPerBandPmi(const PmiVector bandPmi)
    {
        perBandPmi_ = bandPmi;
        status_ |= BAND_PMI;
    }

    //! Set the per preferred band CQI.
    void setPreferredCqi(const CqiVector preferredCqi)
    {
        preferredCqi_ = preferredCqi;
        status_ |= PREFERRED_CQI;
    }
    //! Set the per preferred band CQI for one codeword. Does not check if valid.
    void setPreferredCqi(const Cqi preferredCqi, const Codeword cw)
    {
        if (preferredCqi_.size() == cw)
            preferredCqi_.push_back(preferredCqi);
        else
            preferredCqi_[cw] = preferredCqi;

        status_ |= PREFERRED_CQI;
    }
    //! Set the per preferred band PMI. Invoke this function only after setPreferredCqi().
    void setPreferredPmi(const Pmi preferredPmi)
    {
        preferredPmi_ = preferredPmi;
        status_ |= PREFERRED_PMI;
    }
    //! Set the per preferred bands. Invoke this function everytime you invoke setPreferredCqi().
    void setPreferredBands(const BandSet preferredBands)
    {
        preferredBands_ = preferredBands;
    }

    //! Set the transmission mode.
    void setTxMode(const TxMode txMode)
    {
        txMode_ = txMode;
    }

    //! \test DAS support - Set the remote Antenna identifier
    void setAntenna(const Remote antenna)
    {
        remoteAntennaId_ = antenna;
    }

    //! Set the periodicity type.
    void setPeriodicity(const bool type)
    {
        periodicFeedback_ = type;
    }
    // *****************************************

    /** Print debug information about the LteFeedback instance.
     *  @param cellId The cell ID.
     *  @param nodeId The node ID.
     *  @param dir The link direction.
     *  @param s The name of the function that requested the debug.
     */
    void print(MacCellId cellId, MacNodeId nodeId, Direction dir, const char* s) const
    {
        EV << NOW << " " << s << "         LteFeedback\n";
        EV << NOW << " " << s << " CellId: " << cellId << "\n";
        EV << NOW << " " << s << " NodeId: " << nodeId << "\n";
        EV << NOW << " " << s << " Antenna: " << dasToA(getAntennaId()) << "\n"; // XXX Generoso
        EV << NOW << " " << s << " Direction: " << dirToA(dir) << "\n";
        EV << NOW << " " << s << " -------------------------\n";
        EV << NOW << " " << s << " TxMode: " << txModeToA(getTxMode()) << "\n";
        EV << NOW << " " << s << " Type: " << (isPeriodicFeedback() ? "PERIODIC": "APERIODIC") << "\n";
        EV << NOW << " " << s << " -------------------------\n";

        if(isEmptyFeedback())
        {
            EV << NOW << " " << s << " EMPTY!\n";
        }

        if(hasRankIndicator())
        EV << NOW << " " << s << " RI = " << getRankIndicator() << "\n";

        if(hasPreferredCqi())
        {
            CqiVector cqi = getPreferredCqi();
            unsigned int codewords = cqi.size();
            for(Codeword cw = 0; cw < codewords; ++cw)
            EV << NOW << " " << s << " Preferred CQI[" << cw << "] = " << cqi.at(cw) << "\n";
        }

        if(hasWbCqi())
        {
            CqiVector cqi = getWbCqi();
            unsigned int codewords = cqi.size();
            for(Codeword cw = 0; cw < codewords; ++cw)
            EV << NOW << " " << s << " Wideband CQI[" << cw << "] = " << cqi.at(cw) << "\n";
        }

        if(hasBandCqi())
        {
            std::vector<CqiVector> cqi = getBandCqi();
            unsigned int codewords = cqi.size();
            for(Codeword cw = 0; cw < codewords; ++cw)
            {
                EV << NOW << " " << s << " Band CQI[" << cw << "] = {";
                unsigned int bands = cqi[0].size();
                if(bands > 0)
                {
                    EV << cqi.at(cw).at(0);
                    for(Band b = 1; b < bands; ++b)
                    EV << ", " << cqi.at(cw).at(b);
                }
                EV << "}\n";
            }
        }

        if(hasPreferredPmi())
        {
            Pmi pmi = getPreferredPmi();
            EV << NOW << " " << s << " Preferred PMI = " << pmi << "\n";
        }

        if(hasWbPmi())
        {
            Pmi pmi = getWbPmi();
            EV << NOW << " " << s << " Wideband PMI = " << pmi << "\n";
        }

        if(hasBandCqi())
        {
            PmiVector pmi = getBandPmi();
            EV << NOW << " " << s << " Band PMI = {";
            unsigned int bands = pmi.size();
            if(bands > 0)
            {
                EV << pmi.at(0);
                for(Band b = 1; b < bands; ++b)
                EV << ", " << pmi.at(b);
            }
            EV << "}\n";
        }

        if(hasPreferredCqi() || hasPreferredPmi())
        {
            BandSet band = getPreferredBands();
            BandSet::iterator it = band.begin();
            BandSet::iterator et = band.end();
            EV << NOW << " " << s << " Preferred Bands = {";
            if(it != et)
            {
                EV << *it;
                it++;
                for(;it != et; ++it)
                EV << ", " << *it;
            }
            EV << "}\n";
        }
    }
};

class LteSummaryFeedback
{
    //! confidence function lower bound
    simtime_t confidenceLowerBound_;
    //! confidence function upper bound
    simtime_t confidenceUpperBound_;

  protected:

    //! Maximum number of codewords.
    unsigned char totCodewords_;
    //! Number of logical bands.
    unsigned int logicalBandsTot_;

    //! Rank Indication.
    Rank ri_;
    //! Channel Quality Indicator (per-codeword).
    std::vector<CqiVector> cqi_;
    //! Precoding Matrix Index.
    PmiVector pmi_;

    //! time elapsed from last refresh of RI.
    simtime_t tRi_;
    //! time elapsed from last refresh of CQI.
    std::vector<std::vector<simtime_t> > tCqi_;
    //! time elapsed from last refresh of PMI.
    std::vector<simtime_t> tPmi_;
    // valid flag
    bool valid_;

    /** Calculate the confidence factor.
     *  @param n the feedback age in tti
     */
    double confidence(simtime_t creationTime) const
        {
        simtime_t delta = simTime() - creationTime;

        if (delta < confidenceLowerBound_)
            return 1.0;
        if (delta > confidenceUpperBound_)
            return 0.0;
        return (1.0 - (delta - confidenceLowerBound_) / (confidenceUpperBound_ - confidenceLowerBound_));
    }

  public:

    //! Create an empty feedback message.
    LteSummaryFeedback(unsigned char cw, unsigned int b, simtime_t lb, simtime_t ub)
    {
        totCodewords_ = cw;
        logicalBandsTot_ = b;
        confidenceLowerBound_ = lb;
        confidenceUpperBound_ = ub;
        reset();
    }

    //! Reset this summary feedback.
    void reset()
    {
        ri_ = NORANK;
        tRi_ = simTime();

        cqi_ = std::vector<CqiVector>(totCodewords_, CqiVector(logicalBandsTot_, NOSIGNALCQI)); // XXX DUMMY VALUE USED FOR TESTING: replace with NOSIGNALCQI
        tCqi_ = std::vector<std::vector<simtime_t> >(totCodewords_,
            std::vector<simtime_t>(logicalBandsTot_, simTime()));

        pmi_ = PmiVector(logicalBandsTot_, NOPMI);
        tPmi_ = std::vector<simtime_t>(logicalBandsTot_, simTime());
        valid_ = false;
    }

    /*************
     *  Setters
     *************/

    //! Set the RI.
    void setRi(Rank ri)
    {
        ri_ = ri;
        tRi_ = simTime();
        //set cw
        totCodewords_ = ri > 1 ? 2 : 1;
    }

    //! Set single-codeword/single-band CQI.
    void setCqi(Cqi cqi, Codeword cw, Band band)
    {
        // note: it is impossible to receive cqi == 0!
        cqi_[cw][band] = cqi;
        tCqi_[cw][band] = simTime();
        valid_ = true;
    }

    //! Set single-band PMI.
    void setPmi(Pmi pmi, Band band)
    {
        pmi_[band] = pmi;
        tPmi_[band] = simTime();
    }

    /*************
     *  Getters
     *************/

    //! Get the number of codewords.
    unsigned char getTotCodewords() const
    {
        return totCodewords_;
    }

    //! Get the number of logical bands.
    unsigned char getTotLogicalBands() const
    {
        return logicalBandsTot_;
    }

    //! Get the RI.
    Rank getRi() const
    {
        return ri_;
    }

    //! Get the RI confidence value.
    double getRiConfidence() const
    {
        return confidence(tRi_);
    }

    //! Get single-codeword/single-band CQI.
    Cqi getCqi(Codeword cw, Band band) const
    {
        return cqi_.at(cw).at(band);
    }

    //! Get single-codeword CQI vector.
    const CqiVector& getCqi(Codeword cw) const
    {
        return cqi_.at(cw);
    }

    //! Get single-codeword/single-band CQI confidence value.
    double getCqiConfidence(Codeword cw, Band band) const
    {
        return confidence(tCqi_.at(cw).at(band));
    }

    //! Get single-band PMI.
    Pmi getPmi(Band band) const
    {
        return pmi_.at(band);
    }

    //! Get the PMI vector.
    const PmiVector& getPmi() const
    {
        return pmi_;
    }

    //! Get single-band PMI confidence value.
    double getPmiConfidence(Band band) const
    {
        return confidence(tPmi_.at(band));
    }

    bool isValid()
    {
        return valid_;
    }

    /** Print debug information about the LteSummaryFeedback instance.
     *  @param cellId The cell ID.
     *  @param nodeId The node ID.
     *  @param dir The link direction.
     *  @param txm The transmission mode.
     *  @param s The name of the function that requested the debug.
     */
    void print(MacCellId cellId, MacNodeId nodeId, const Direction dir, TxMode txm, const char* s) const
        {
        EV << NOW << " " << s << "     LteSummaryFeedback\n";
        EV << NOW << " " << s << " CellId: " << cellId << "\n";
        EV << NOW << " " << s << " NodeId: " << nodeId << "\n";
        EV << NOW << " " << s << " Direction: " << dirToA(dir) << "\n";
        EV << NOW << " " << s << " TxMode: " << txModeToA(txm) << "\n";
        EV << NOW << " " << s << " -------------------------\n";

        Rank ri = getRi();
        double c = getRiConfidence();
        EV << NOW << " " << s << " RI = " << ri << " [" << c << "]\n";

        unsigned char codewords = getTotCodewords();
        unsigned char bands = getTotLogicalBands();
        for(Codeword cw = 0; cw < codewords; ++cw)
        {
            EV << NOW << " " << s << " CQI[" << cw << "] = {";
            if(bands > 0)
            {
                EV << getCqi(cw, 0);
                for(Band b = 1; b < bands; ++b)
                EV << ", " << getCqi(cw, b);
            }
            EV << "} [{";
            if(bands > 0)
            {
                c = getCqiConfidence(cw, 0);
                EV << c;
                for(Band b = 1; b < bands; ++b)
                {
                    c = getCqiConfidence(cw, b);
                    EV << ", " << c;
                }
            }
            EV << "}]\n";
        }

        EV << NOW << " " << s << " PMI = {";
        if(bands > 0)
        {
            EV << getPmi(0);
            for(Band b = 1; b < bands; ++b)
            EV << ", " << getPmi(b);
        }
        EV << "} [{";
        if(bands > 0)
        {
            c = getPmiConfidence(0);
            EV << c;
            for(Band b = 1; b < bands; ++b)
            {
                c = getPmiConfidence(b);
                EV << ", " << c;
            }
        }
        EV << "}]\n";
    }
};

class LteSummaryBuffer
{
  protected:
    //! Buffer dimension
    unsigned char bufferSize_;
    //! The buffer
    std::list<LteFeedback> buffer_;
    //! Number of codewords.
    double totCodewords_;
    //! Number of bands.
    double totBands_;
    //! Cumulative summary feedback.
    LteSummaryFeedback cumulativeSummary_;
    void createSummary(LteFeedback fb);

  public:

    LteSummaryBuffer(unsigned char dim, unsigned char cw, unsigned int b, simtime_t lb, simtime_t ub) :
        cumulativeSummary_(cw, b, lb, ub)
    {
        bufferSize_ = dim;
        totCodewords_ = cw;
        totBands_ = b;
    }

    //! Put a feedback into the buffer and update current summary feedback
    void put(LteFeedback fb)
    {
        if (bufferSize_ > 0)
        {
            buffer_.push_back(fb);
        }
        if (buffer_.size() > bufferSize_)
        {
            buffer_.pop_front();
        }
        createSummary(fb);
    }

    //! Get the current summary feedback
    LteSummaryFeedback get() const
    {
        return cumulativeSummary_;
    }
};

/**
 * @class LteMuMimoMatrix
 *
 * MU-MIMO compatibility matrix structure .
 * it holds MU-MIMO pairings computed by the MU-MIMO matching function
 */
class LteMuMimoMatrix
{
    typedef std::vector<MacNodeId> MuMatrix;
    protected:
    MuMatrix muMatrix_;
    MacNodeId maxNodeId_;

    MacNodeId toNodeId(unsigned int i)
    {
        return i + UE_MIN_ID;
    }
    unsigned int toIndex(MacNodeId node)
    {
        return node - UE_MIN_ID;
    }
  public:
    LteMuMimoMatrix()
    {
        maxNodeId_ = 0;
    }

    void initialize(MacNodeId node)
    {
        muMatrix_.clear();
        muMatrix_.resize(toIndex(node) + 1, 0);
    }

    void addPair(MacNodeId id1, MacNodeId id2)
    {
        muMatrix_[toIndex(id1)] = id2;
        muMatrix_[toIndex(id2)] = id1;
    }
    MacNodeId getMuMimoPair(MacNodeId id)
    {
        return muMatrix_[toIndex(id)];
    }
    void print(const char *s)
    {
        EV << NOW << " " << s << " ################" << endl;
        EV << NOW << " " << s << " LteMuMimoMatrix" << endl;
        EV << NOW << " " << s << " ################" << endl;
        for (unsigned int i=1025;i<maxNodeId_;i++)
        EV << NOW << "" << i;
        EV << endl;
        for (unsigned int i=1025;i<maxNodeId_;i++)
        EV << NOW << "" << muMatrix_[i];
    }
};

#endif
