//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/amc/AmcPilotD2D.h"

void AmcPilotD2D::setPreconfiguredTxParams(Cqi cqi)
{
    usePreconfiguredTxParams_ = true;
    preconfiguredTxParams_ = new UserTxParams();

    // default parameters for D2D
    preconfiguredTxParams_->isSet() = true;
//    preconfiguredTxParams_->setD2DEnabled(true);
    preconfiguredTxParams_->writeTxMode(TRANSMIT_DIVERSITY);
    Rank ri = 1;                                              // rank for TxD is one
    preconfiguredTxParams_->writeRank(ri);
    preconfiguredTxParams_->writePmi(intuniform(getEnvir()->getRNG(0),1, pow(ri, (double) 2)));   // taken from LteFeedbackComputationRealistic::computeFeedback

    if (cqi < 0 || cqi > 15)
        throw cRuntimeError("AmcPilotD2D::setPreconfiguredTxParams - CQI %s is not a valid value. Aborting", cqi);
    preconfiguredTxParams_->writeCqi(std::vector<Cqi>(1,cqi));

    BandSet b;
    for (Band i = 0; i < amc_->getSystemNumBands(); ++i) b.insert(i);
        preconfiguredTxParams_->writeBands(b);

    RemoteSet antennas;
    antennas.insert(MACRO);
    preconfiguredTxParams_->writeAntennas(antennas);
}

const UserTxParams& AmcPilotD2D::computeTxParams(MacNodeId id, const Direction dir)
{
    EV << NOW << " AmcPilot" << getName() << "::computeTxParams for UE " << id << ", direction " << dirToA(dir) << endl;

    if ((dir == D2D || dir == D2D_MULTI) && usePreconfiguredTxParams_)
    {
        EV << NOW << " AmcPilot" << getName() << "::computeTxParams Use preconfigured Tx params for D2D connections" << endl;
        return *preconfiguredTxParams_;
    }

    // Check if user transmission parameters have been already allocated
    if(amc_->existTxParams(id, dir))
    {
        EV << NOW << " AmcPilot" << getName() << "::computeTxParams The Information for this user have been already assigned" << endl;
        return amc_->getTxParams(id, dir);
    }
    // TODO make it configurable from NED
    // default transmission mode
    TxMode txMode = TRANSMIT_DIVERSITY;

    /**
     *  Select the band which has the best summary
     *  Note: this pilot is not DAS aware, so only MACRO antenna
     *  is used.
     */

    // FIXME The eNodeB knows the CQI for N D2D links originating from this UE, but it does not
    // know which UE will be addressed in this transmission. Which feedback should it select?
    // This is an open issue.
    // Here, we will get the feedback of the first peer in the map. However, algorithms may be
    // designed to solve this problem

    MacNodeId peerId = 0;  // FIXME this way, the getFeedbackD2D() function will return the first feedback available

    LteSummaryFeedback sfb = (dir==UL || dir==DL) ? amc_->getFeedback(id, MACRO, txMode, dir) : amc_->getFeedbackD2D(id, MACRO, txMode, peerId);

    if (TxMode(txMode)==MULTI_USER) // Initialize MuMiMoMatrix
        amc_->muMimoMatrixInit(dir,id);

    sfb.print(0,id,dir,txMode,"AmcPilotD2D::computeTxParams");

    // get a vector of  CQI over first CW
    std::vector<Cqi> summaryCqi = sfb.getCqi(0);

    Cqi chosenCqi;
    BandSet b;
    if (mode_ == AVG_CQI)
    {
        // MEAN cqi computation method
        chosenCqi = getBinder()->meanCqi(sfb.getCqi(0),id,dir);
        for (Band i = 0; i < sfb.getCqi(0).size(); ++i)
            b.insert(i);
    }
    else
    {
        // MIN/MAX cqi computation method
        Band band = 0;
        chosenCqi = summaryCqi.at(band);
        unsigned int bands = summaryCqi.size();// number of bands
        for(Band b = 1; b < bands; ++b)
        {
            // For all LBs
            double s = (double)summaryCqi.at(b);
            if((mode_ == MIN_CQI && s < chosenCqi) || (mode_ == MAX_CQI && s > chosenCqi))
            {
                band = b;
                chosenCqi = s;
            }

        }
        b.insert(band);
    }

    // Set user transmission parameters
    UserTxParams info;
    info.writeTxMode(txMode);
    info.writeRank(sfb.getRi());
    info.writeCqi(std::vector<Cqi>(1,chosenCqi));
    info.writePmi(sfb.getPmi(0));
    info.writeBands(b);
    RemoteSet antennas;
    antennas.insert(MACRO);
    info.writeAntennas(antennas);

    // DEBUG
    EV << NOW << " AmcPilot" << getName() << "::computeTxParams NEW values assigned! - CQI =" << chosenCqi << "\n";
    info.print("AmcPilotD2D::computeTxParams");

    //return amc_->setTxParams(id, dir, info,user_type); OLD solution
    // Debug
    const UserTxParams& info2 = amc_->setTxParams(id, dir, info);

    return info2;
}
