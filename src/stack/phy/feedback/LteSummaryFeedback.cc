/*
 * LteSummaryFeedback.cc
 *
 *  Created on: Oct 3, 2019
 *      Author: q
 */

#include <iostream>
#include "stack/phy/feedback/LteSummaryFeedback.h"
#include "stack/phy/feedback/LteSummaryBuffer.h"
using namespace omnetpp;

void LteSummaryBuffer::createSummary(LteFeedback fb) {
    try {
        // RI
        if (fb.hasRankIndicator()) {
            Rank ri(fb.getRankIndicator());
            cumulativeSummary_.setRi(ri);
            if (ri > 1)
                totCodewords_ = 2;
        }

        // CQI
        if (fb.hasBandCqi()) // Per-band
        {
            std::vector<CqiVector> cqi = fb.getBandCqi();
            unsigned int n = cqi.size();
            for (Codeword cw = 0; cw < n; ++cw)
                for (Band i = 0; i < totBands_; ++i)
                    cumulativeSummary_.setCqi(cqi.at(cw).at(i), cw, i);
        } else {
            if (fb.hasWbCqi()) // Wide-band
            {
                CqiVector cqi(fb.getWbCqi());
                unsigned int n = cqi.size();
                for (Codeword cw = 0; cw < n; ++cw)
                    for (Band i = 0; i < totBands_; ++i)
                        cumulativeSummary_.setCqi(cqi.at(cw), cw, i); // ripete lo stesso wb cqi su ogni banda della stessa cw
            }
            if (fb.hasPreferredCqi()) // Preferred-band
            {
                CqiVector cqi(fb.getPreferredCqi());
                BandSet bands(fb.getPreferredBands());
                unsigned int n = cqi.size();
                BandSet::iterator et = bands.end();
                for (Codeword cw = 0; cw < n; ++cw)
                    for (BandSet::iterator it = bands.begin(); it != et; ++it)
                        cumulativeSummary_.setCqi(cqi.at(cw), cw, *it); // mette lo stesso cqi solo sulle bande preferite della stessa cw
            }
        }

        // Per il PMI si comporta in modo analogo

        // PMI
        if (fb.hasBandPmi()) // Per-band
        {
            PmiVector pmi(fb.getBandPmi());
            for (Band i = 0; i < totBands_; ++i)
                cumulativeSummary_.setPmi(pmi.at(i), i);
        } else {
            if (fb.hasWbPmi()) {
                // Wide-band
                Pmi pmi(fb.getWbPmi());
                for (Band i = 0; i < totBands_; ++i)
                    cumulativeSummary_.setPmi(pmi, i);
            }
            if (fb.hasPreferredPmi()) {
                // Preferred-band
                Pmi pmi(fb.getPreferredPmi());
                BandSet bands(fb.getPreferredBands());
                BandSet::iterator et = bands.end();
                for (BandSet::iterator it = bands.begin(); it != et; ++it)
                    cumulativeSummary_.setPmi(pmi, *it);
            }
        }
    } catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSummaryBuffer::summarize(): %s",
                e.what());
    }
}

void LteSummaryFeedback::print(MacCellId cellId, MacNodeId nodeId,
        const Direction dir, TxMode txm, const char* s) const {
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
    for (Codeword cw = 0; cw < codewords; ++cw) {
        EV << NOW << " " << s << " CQI[" << cw << "] = {";
        if (bands > 0) {
            EV << getCqi(cw, 0);
            for (Band b = 1; b < bands; ++b)
                EV << ", " << getCqi(cw, b);
        }
        EV << "} [{";
        if (bands > 0) {
            c = getCqiConfidence(cw, 0);
            EV << c;
            for (Band b = 1; b < bands; ++b) {
                c = getCqiConfidence(cw, b);
                EV << ", " << c;
            }
        }
        EV << "}]\n";
    }

    EV << NOW << " " << s << " PMI = {";
    if (bands > 0) {
        EV << getPmi(0);
        for (Band b = 1; b < bands; ++b)
            EV << ", " << getPmi(b);
    }
    EV << "} [{";
    if (bands > 0) {
        c = getPmiConfidence(0);
        EV << c;
        for (Band b = 1; b < bands; ++b) {
            c = getPmiConfidence(b);
            EV << ", " << c;
        }
    }
    EV << "}]\n";
}
