//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/scheduling_modules/LteMaxCiComp.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"

void LteMaxCiComp::getBandLimit(std::vector<BandLimit>* bandLimit, MacNodeId ueId)
{
    bandLimit->clear();

    // get usable bands for this user
    UsableBands* usableBands = eNbScheduler_->mac_->getAmc()->getPilotUsableBands(ueId);
    if (usableBands == NULL)
    {
//        bandLimit = NULL; // all bands available
        // leave the bandLimit empty
        return;
    }

    // check the number of codewords
    unsigned int numCodewords = 1;
    unsigned int numBands = eNbScheduler_->mac_->getDeployer()->getNumBands();
    // for each band of the band vector provided
    for (unsigned int i = 0; i < numBands; i++)
    {
        BandLimit elem;
        elem.band_ = Band(i);

        int limit = -2;

        // check whether band i is in the set of usable bands
        UsableBands::iterator it = usableBands->begin();
        for (; it != usableBands->end(); ++it)
        {
            if (*it == i)
            {
                // band i must be marked as unlimited
                limit = -1;
                break;
            }
        }

        elem.limit_.clear();
        for (unsigned int j = 0; j < numCodewords; j++)
            elem.limit_.push_back(limit);

        bandLimit->push_back(elem);
    }
}

void LteMaxCiComp::prepareSchedule()
{
    EV << NOW << " LteMaxCiComp::schedule " << eNbScheduler_->mac_->getMacNodeId() << endl;

    if (binder_ == NULL)
        binder_ = getBinder();

    activeConnectionTempSet_ = activeConnectionSet_;

    // Build the score list by cycling through the active connections.
    ScoreList score;
    MacCid cid =0;
    unsigned int blocks =0;
    unsigned int byPs = 0;

    for ( ActiveSet::iterator it1 = activeConnectionTempSet_.begin ();it1 != activeConnectionTempSet_.end (); ++it1 )
    {
        // Current connection.
        cid = *it1;

        MacNodeId nodeId = MacCidToNodeId(cid);
        OmnetId id = binder_->getOmnetId(nodeId);
        if(nodeId == 0 || id == 0)
        {
            // node has left the simulation - erase corresponding CIDs
            activeConnectionSet_.erase(cid);
            activeConnectionTempSet_.erase(cid);
            continue;
        }

        // compute available blocks for the current user
        const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId,direction_);
        const std::set<Band>& bands = info.readBands();
        std::set<Band>::const_iterator it = bands.begin(),et=bands.end();
        unsigned int codeword=info.getLayers().size();
        bool cqiNull=false;
        for (unsigned int i=0;i<codeword;i++)
        {
            if (info.readCqiVector()[i]==0)
            cqiNull=true;
        }
        if (cqiNull)
        continue;
        //no more free cw
        if (eNbScheduler_->allocatedCws(nodeId)==codeword)
        continue;

        std::set<Remote>::iterator antennaIt = info.readAntennaSet().begin(), antennaEt=info.readAntennaSet().end();

        // compute score based on total available bytes
        unsigned int availableBlocks=0;
        unsigned int availableBytes =0;
        // for each antenna
        for (;antennaIt!=antennaEt;++antennaIt)
        {
            // for each logical band
            for (;it!=et;++it)
            {
                availableBlocks += eNbScheduler_->readAvailableRbs(nodeId,*antennaIt,*it);
                availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId,*it, availableBlocks, direction_);
            }
        }

        blocks = availableBlocks;
        // current user bytes per slot
        byPs = (blocks>0) ? (availableBytes/blocks ) : 0;

        // Create a new score descriptor for the connection, where the score is equal to the ratio between bytes per slot and long term rate
        ScoreDesc desc(cid,byPs);
        // insert the cid score
        score.push (desc);

        EV << NOW << " LteMaxCiComp::schedule computed for cid " << cid << " score of " << desc.score_ << endl;
    }

    std::vector<BandLimit> usableBands;

    // Schedule the connections in score order.
    while ( ! score.empty () )
    {
        // Pop the top connection from the list.
        ScoreDesc current = score.top ();

        // Get the bandLimit for the current user
        std::vector<BandLimit>* bandLim;
        getBandLimit(&usableBands, MacCidToNodeId(current.x_));
        if (usableBands.empty())
            bandLim = NULL;
        else
            bandLim = &usableBands;

        EV << NOW << " LteMaxCiComp::schedule scheduling connection " << current.x_ << " with score of " << current.score_ << endl;

        // Grant data to that connection.
        bool terminate = false;
        bool active = true;
        bool eligible = true;
        unsigned int granted = requestGrant (current.x_, 4294967295U, terminate, active, eligible, bandLim);

        EV << NOW << "LteMaxCiComp::schedule granted " << granted << " bytes to connection " << current.x_ << endl;

        // Exit immediately if the terminate flag is set.
        if ( terminate ) break;

        // Pop the descriptor from the score list if the active or eligible flag are clear.
        if ( ! active || ! eligible )
        {
            score.pop ();
            EV << NOW << "LteMaxCiComp::schedule  connection " << current.x_ << " was found ineligible" << endl;
        }

        // Set the connection as inactive if indicated by the grant ().
        if ( ! active )
        {
            EV << NOW << "LteMaxCiComp::schedule scheduling connection " << current.x_ << " set to inactive " << endl;

            activeConnectionTempSet_.erase (current.x_);
        }
    }
}

void LteMaxCiComp::commitSchedule()
{
    activeConnectionSet_ = activeConnectionTempSet_;
}

void LteMaxCiComp::updateSchedulingInfo()
{
}

void LteMaxCiComp::notifyActiveConnection(MacCid cid)
{
    EV << NOW << "LteMaxCiComp::notify CID notified " << cid << endl;
    activeConnectionSet_.insert(cid);
}

void LteMaxCiComp::removeActiveConnection(MacCid cid)
{
    EV << NOW << "LteMaxCiComp::remove CID removed " << cid << endl;
    activeConnectionSet_.erase(cid);
}
