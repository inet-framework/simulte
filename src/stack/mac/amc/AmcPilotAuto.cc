//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/amc/AmcPilotAuto.h"

const UserTxParams& AmcPilotAuto::computeTxParams(MacNodeId id, const Direction dir)
{
    EV << NOW << " AmcPilot" << getName() << "::computeTxParams for UE " << id << ", direction " << dirToA(dir) << endl;

    // Check if user transmission parameters have been already allocated
    if(amc_->existTxParams(id, dir))
    {
        EV << NOW << " AmcPilot" << getName() << "::computeTxParams The Information for this user have been already assigned \n";
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
    LteSummaryFeedback sfb = amc_->getFeedback(id, MACRO, txMode, dir);

    if (TxMode(txMode)==MULTI_USER) // Initialize MuMiMoMatrix
    amc_->muMimoMatrixInit(dir,id);

    sfb.print(0,id,dir,txMode,"AmcPilotAuto::computeTxParams");

    // get a vector of  CQI over first CW
    std::vector<Cqi> summaryCqi = sfb.getCqi(0);

    // get the usable bands for this user
    UsableBands* usableB = getUsableBands(id);

    Band chosenBand = 0;
    double chosenCqi = 0;
    // double max = 0;
    BandSet bandSet;

    /// TODO collapse the following part into a single part (e.g. do not fork on mode_ or usableBandsList_ size)

    // check which CQI computation policy is to be used
    if(mode_ == MAX_CQI)
    {
        // if there are no usable bands, compute the final CQI through all the bands
        if (usableB == NULL || usableB->empty())
        {
            chosenBand = 0;
            chosenCqi = summaryCqi.at(chosenBand);
            unsigned int bands = summaryCqi.size();// number of bands
            // computing MAX
            for(Band b = 1; b < bands; ++b)
            {
                // For all Bands
                double s = (double)summaryCqi.at(b);
                if(chosenCqi < s)
                {
                    chosenBand = b;
                    chosenCqi = s;
                }
            }
            EV << NOW <<" AmcPilotAuto::computeTxParams - no UsableBand available for this user." << endl;
        }
        else
        {
            // TODO Add MIN and MEAN cqi computation methods
            int bandIt = 0;
            unsigned short currBand = (*usableB)[bandIt];
            chosenBand = currBand;
            chosenCqi = summaryCqi.at(currBand);
            for(; bandIt < usableB->size(); ++bandIt)
            {
                currBand = (*usableB)[bandIt];
                // For all available band
                double s = (double)summaryCqi.at(currBand);
                if(chosenCqi < s)
                {
                    chosenBand = currBand;
                    chosenCqi = s;
                }
            }
            EV << NOW <<" AmcPilotAuto::computeTxParams - UsableBand of size " << usableB->size() << " available for this user" << endl;
        }
    }
    else if(mode_ == MIN_CQI)
    {
        // if there are no usable bands, compute the final CQI through all the bands
        if (usableB == NULL || usableB->empty())
        {
            chosenBand = 0;
            chosenCqi = summaryCqi.at(chosenBand);
            unsigned int bands = summaryCqi.size();// number of bands
            // computing MIN
            for(Band b = 1; b < bands; ++b)
            {
                // For all LBs
                double s = (double)summaryCqi.at(b);
                if(chosenCqi > s)
                {
                    chosenBand = b;
                    chosenCqi = s;
                }
            }
            EV << NOW <<" AmcPilotAuto::computeTxParams - no UsableBand available for this user." << endl;
        }
        else
        {
            int bandIt = 0;
            unsigned short currBand = (*usableB)[bandIt];
            chosenBand = currBand;
            chosenCqi = summaryCqi.at(currBand);
            for(; bandIt < usableB->size(); ++bandIt)
            {
                currBand = (*usableB)[bandIt];
                // For all available band
                double s = (double)summaryCqi.at(currBand);
                if(chosenCqi > s)
                {
                    chosenBand = currBand;
                    chosenCqi = s;
                }
            }

            EV << NOW <<" AmcPilotAuto::computeTxParams - UsableBand of size " << usableB->size() << " available for this user" << endl;
        }
    }

    // save the chosen band
    bandSet.insert(chosenBand);

    // Set user transmission parameters only for the best band
    UserTxParams info;
    info.writeTxMode(txMode);
    info.writeRank(sfb.getRi());
    info.writeCqi(std::vector<Cqi>(1, sfb.getCqi(0, chosenBand)));
    info.writePmi(sfb.getPmi(chosenBand));
    info.writeBands(bandSet);
    RemoteSet antennas;
    antennas.insert(MACRO);
    info.writeAntennas(antennas);

    // DEBUG
    EV << NOW << " AmcPilot" << getName() << "::computeTxParams NEW values assigned! - CQI =" << chosenCqi << "\n";
    info.print("AmcPilotAuto::computeTxParams");

    return amc_->setTxParams(id, dir, info);
}

std::vector<Cqi> AmcPilotAuto::getMultiBandCqi(MacNodeId id , const Direction dir)
{
    EV << NOW << " AmcPilot" << getName() << "::getMultiBandCqi for UE " << id << ", direction " << dirToA(dir) << endl;

    // TODO make it configurable from NED
    // default transmission mode
    TxMode txMode = TRANSMIT_DIVERSITY;

    /**
     *  Select the band which has the best summary
     *  Note: this pilot is not DAS aware, so only MACRO antenna
     *  is used.
     */
    LteSummaryFeedback sfb = amc_->getFeedback(id, MACRO, txMode, dir);

    // get a vector of  CQI over first CW
    return sfb.getCqi(0);
}

void AmcPilotAuto::setUsableBands(MacNodeId id , UsableBands usableBands)
{
    EV << NOW << " AmcPilotAuto::setUsableBands - setting Usable bands: for node " << id<< " [" ;
    for(int i = 0 ; i<usableBands.size() ; ++i)
    {
        EV << usableBands[i] << ",";
    }
    EV << "]"<<endl;
    UsableBandsList::iterator it = usableBandsList_.find(id);

    // if usable bands for this node are already setm delete it (probably unnecessary)
    if(it!=usableBandsList_.end())
        usableBandsList_.erase(id);
    usableBandsList_.insert(std::pair<MacNodeId,UsableBands>(id,usableBands));
}

UsableBands* AmcPilotAuto::getUsableBands(MacNodeId id)
{
    EV << NOW << " AmcPilotAuto::getUsableBands - getting Usable bands for node " << id;

    bool found = false;
    UsableBandsList::iterator it = usableBandsList_.find(id);
    if(it!=usableBandsList_.end())
    {
        found = true;
    }
    else
    {
        // usable bands for this id not found
        if (getNodeTypeById(id) == UE)
        {
            // if it is a UE, look for its serving cell
            MacNodeId cellId = getBinder()->getNextHop(id);
            it = usableBandsList_.find(cellId);
            if(it!=usableBandsList_.end())
                found = true;
        }
    }

    if (found)
    {
        EV << " [" ;
        for(unsigned int i = 0 ; i < it->second.size() ; ++i)
        {
            EV << it->second[i] << ",";
        }
        EV << "]"<<endl;

        return &(it->second);
    }
    EV << " [All bands are usable]" << endl ;
    return NULL;
}
