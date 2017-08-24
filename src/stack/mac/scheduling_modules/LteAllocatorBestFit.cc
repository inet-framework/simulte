//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/scheduling_modules/LteAllocatorBestFit.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/conflict_graph_utilities/meshMaster.h"

LteAllocatorBestFit::LteAllocatorBestFit()
{
}

void LteAllocatorBestFit::checkHole(Candidate& candidate, Band holeIndex, unsigned int holeLen, unsigned int req)
{
    if (holeLen > req)
    {
        // this hole would satisfy requested load
        if (!candidate.greater)
        {
            // The current candidate would not satisfy the requested load.
            // The hole is a better candidate. Update
            candidate.greater = true;
            candidate.index = holeIndex;
            candidate.len = holeLen;
        }
        else
        {   // The current candidate would satisfy the requested load.
            // compare the length of the hole with that of the best candidate
            if (candidate.len > holeLen)
            {
                // the hole is a better candidate. Update
                candidate.greater = true;
                candidate.index = holeIndex;
                candidate.len = holeLen;
            }
        }
    }
    else
    {
        // this hole would NOT satisfy requested load
        if (!candidate.greater)
        {
            // The current candidate would not satisfy the requested load.
            // compare the length of the hole with that of the best candidate
            if (candidate.len < holeLen)
            {
                // the hole is a better candidate. Update
                candidate.greater = false;
                candidate.index = holeIndex;
                candidate.len = holeLen;
            }
        }
        // else do not update. The current candidate would satisfy the requested load.
    }
//       std::cout << " New candidate is: index[" << candidate.index << "], len["<< candidate.len << "]" << endl;

}

void LteAllocatorBestFit::prepareSchedule()
{
    EV << NOW << " LteAllocatorBestFit::schedule " << eNbScheduler_->mac_->getMacNodeId() << endl;

    if (binder_ == NULL)
        binder_ = getBinder();

    // Initialize SchedulerAllocation structures
    initAndReset();

    // Get the bands occupied by RAC and RTX
    std::set<Band> alreadyAllocatedBands = eNbScheduler_->getOccupiedBands();
    unsigned int firstUnallocatedBand;
    if(!alreadyAllocatedBands.empty())
    {
        // Get the last band in the Set (a set is ordered so the last band is the latest occupied)
        firstUnallocatedBand = *alreadyAllocatedBands.rbegin();
    }
    else firstUnallocatedBand = 0;

    // Start the allocation of IM flows from the end of the frame
    int firstUnallocatedBandIM = eNbScheduler_->getResourceBlocks() - 1;

    // Get the active connection Set
    activeConnectionTempSet_ = activeConnectionSet_;

    // Create a Conflict Map wich, for every nodeId, have a set of conflicting nodes
    const std::map<MacNodeId,std::set<MacNodeId> >* conflictMap = mac_->getMeshMaster()->getConflictMap();

    // record the amount of allocated bytes (for optimal comparison)
    unsigned int totalAllocatedBytes = 0;

    // Resume a MaxCi scoreList build mode
    // Build the score list by cycling through the active connections.
    ScoreList score;
    ScoreList conflicting_score;
    MacCid cid = 0;
    unsigned int blocks = 0;
    unsigned int byPs = 0;

    // Set for deleting inactive connections
    ActiveSet inactive_connections;
    inactive_connections.clear();

    for ( ActiveSet::iterator it1 = activeConnectionTempSet_.begin ();it1 != activeConnectionTempSet_.end (); ++it1 )
    {
        // Current connection.
        cid = *it1;

        MacNodeId nodeId = MacCidToNodeId(cid);
        bool enableFrequencyReuse = binder_->isFrequencyReuseEnabled(nodeId);

//        if( enableFrequencyReuse )
//        {
//            // Abort scheduling if cid is found (if cid is present means that there's priority for Ul RETX)
//            if ( eNbScheduler_->rtxUlMap_.find( MacCidToNodeId(cid) ) != eNbScheduler_->rtxUlMap_.end() )
//            {
//                EV << NOW << " LteAllocatorBestFit::schedule ABORT SCHEDULING: " << MacCidToNodeId(cid)  << endl;
//                continue;
//            }
//        }
        // Get virtual buffer reference
        LteMacBuffer* conn = eNbScheduler_->bsrbuf_->at(cid);
        // Check whether the virtual buffer is empty
        if (conn->isEmpty())
        {
            // The BSR buffer for this node is empty. Abort scheduling for the node: no data to transmit.
            EV << "LteAllocatorBestFit:: scheduled connection is no more active . Exiting grant " << endl;
            // The BSR buffer for this node is empty so the connection is no more active.
            inactive_connections.insert(cid);
            continue;
        }

        // Set the right direction for nodeId
        Direction dir;
        if (enableFrequencyReuse && direction_ == UL)
            dir = (MacCidToLcid(cid) == D2D_SHORT_BSR) ? D2D : (MacCidToLcid(cid) == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : direction_;
        else
            dir = DL;

        // Compute available blocks for the current user
        const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId,dir);
        const std::set<Band>& bands = info.readBands();
        std::set<Band>::const_iterator it = bands.begin(),et=bands.end();
        unsigned int codeword=info.getLayers().size();
        bool cqiNull=false;
        for (unsigned int i=0;i<codeword;i++)
        {
            if (info.readCqiVector()[i]==0) cqiNull=true;
        }
        if (cqiNull)
        {
            EV<<NOW<<"Cqi null, direction: "<<dirToA(dir)<<endl;
            continue;
        }
        // No more free cw
        if ( eNbScheduler_->allocatedCws(nodeId)==codeword )
        {
            EV<<NOW<<"Allocated codeword, direction: "<<dirToA(dir)<<endl;
            continue;
        }

        std::set<Remote>::iterator antennaIt = info.readAntennaSet().begin(), antennaEt=info.readAntennaSet().end();
        // Compute score based on total available bytes
        unsigned int availableBlocks=0;
        unsigned int availableBytes =0;
        // For each antenna
        for (;antennaIt!=antennaEt;++antennaIt)
        {
            // For each logical band
            for (;it!=et;++it)
            {
                availableBlocks += eNbScheduler_->readAvailableRbs(nodeId,*antennaIt,*it);
                availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId,*it, availableBlocks, dir);
            }
        }

        blocks = availableBlocks;
        // current user bytes per slot
        byPs = (blocks>0) ? (availableBytes/blocks ) : 0;

        // Create a new score descriptor for the connection, where the score is equal to the ratio between bytes per slot and long term rate
        ScoreDesc desc(cid,byPs);
        // Insert the cid score in the right list
        conflicting_score.push (desc);

        EV << NOW << " LteAllocatorBestFit::schedule computed for cid " << cid << " score of " << desc.score_ << endl;
    }

    //Delete inactive connections
    for(ActiveSet::iterator in_it = inactive_connections.begin();in_it!=inactive_connections.end();++in_it)
    {
        activeConnectionTempSet_.erase(*in_it);
    }

    // Schedule the connections in score order.
    while ( !conflicting_score.empty()  )
    {
        // Pop the top connection from the list.
        ScoreDesc current;
        if(!conflicting_score.empty())
        {
            current = conflicting_score.top();
        }

        //Set the right direction for nodeId
        Direction dir;
        MacNodeId nodeId = MacCidToNodeId(current.x_);
        bool enableFrequencyReuse = binder_->isFrequencyReuseEnabled(nodeId);
        if( enableFrequencyReuse && direction_ != DL )
        {
            if (MacCidToLcid(current.x_) == D2D_MULTI_SHORT_BSR)
                dir = D2D_MULTI;
            else
                dir = (MacCidToLcid(cid) == D2D_SHORT_BSR) ? D2D : direction_;
        }
        else
        {
            if (direction_ != DL && MacCidToLcid(cid) == D2D_MULTI_SHORT_BSR)
                dir = D2D_MULTI;
            else
                dir = direction_;
        }

        EV << NOW << " LteAllocatorBestFit::schedule scheduling connection " << current.x_ << " with score of " << current.score_ << endl;

        // Get the node ID
        MacCid cid = current.x_;

        // Compute Tx params for the extracted node
        const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId,dir);
        // Get virtual buffer reference
        // TODO After debug reenable conn
        LteMacBuffer* conn = eNbScheduler_->bsrbuf_->at(cid);

        // Get a reference of the first BSR
        PacketInfo vpkt = conn->front();
        // Get the size of the BSR
        unsigned int vQueueFrontSize = vpkt.first + MAC_HEADER;

        // Get the node CQI
        std::vector<Cqi> node_CQI = txParams.readCqiVector();

        // TODO add MIMO support
        Codeword cw = 0;
        unsigned int req_RBs = 0;

        // Calculate the number of Bytes available in a block
        unsigned int req_Bytes1RB = 0;
        req_Bytes1RB = eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId,0,cw,1,dir); // The band (here equals to 0) is useless

        // This calculus is for coherence with the allocation done in the ScheduleGrant function
        req_RBs = (vQueueFrontSize+req_Bytes1RB-1)/req_Bytes1RB;

        // Get the total number of bands
        unsigned int numBands = mac_->getDeployer()->getNumBands();

        unsigned int blocks = 0;
        // Set the band counter to zero
        int band=0;
        // Create the set for booked bands
        std::vector<Band> bookedBands;
        bookedBands.clear();

        // Scan the RBs and find the best candidate "hole" to allocate the UE
        // We need to allocate RBs in the hole with the minimum length, such that the request is satisfied
        // If satisfying the request is not possible, then allocate RBs in the hole the maximum length
        Candidate candidate;
        candidate.index = 0;
        candidate.len = 0;
        candidate.greater = false;

        // info for the current hole considered
        bool newHole = true;
        Band holeIndex = 0;
        unsigned int holeLen = 0;

        // TODO: Find a better way to allocate IM from the end of the frame
        if (enableFrequencyReuse || dir == D2D_MULTI)
        {
//            std::cout << NOW << " UE " << nodeId << " is D2D enabled" << endl;

            // Check if the allocation is possible starting from the first unallocated band
            for( band=firstUnallocatedBand; band<numBands; band++ )
            {
                bool jump_band = false;
                // Jump to the next band if this have been already allocated
                if( alreadyAllocatedBands.find(band) != alreadyAllocatedBands.end() ) { jump_band = true; }
                /*
                 * Jump to the next band if:
                 * - dedicated is true
                 * - the node is a D2D one
                 * - the band is occupied by an INFRASTRCUCTURE node.
                 *  If dedicated is "false" a D2D UE can
                 * share a band with one or more INFRASTRUCTURE UEs.
                 */
                if( enableFrequencyReuse && bandStatusMap_[band].first == CELLT && dedicated_ ) { jump_band = true; }
                /*
                 * Jump to the next band if the current band is occupied by a conflicting node (i.e. there's an edge in the
                 * conflict graph)
                 */
                if( conflictMap->find(nodeId)!=conflictMap->end() )
                {
                    std::set<MacNodeId>::const_iterator conf_it = conflictMap->find(nodeId)->second.begin();
                    std::set<MacNodeId>::const_iterator conf_et = conflictMap->find(nodeId)->second.end();
                    for(;conf_it!=conf_et;++conf_it)
                    {
                        // Check if this band is occupied by an interfering node for the nodeId
                        if(bandStatusMap_[band].second.find(*conf_it) != bandStatusMap_[band].second.end())
                        {
                            // Set jump_band to "true" cause we have to jump to the next band
                            jump_band = true;
                            break;
                        }
                    }
                }

                // Check if this band is occupied by a node for whom the nodeId is an interfering node
                std::set<MacNodeId>::const_iterator it =  bandStatusMap_[band].second.begin();
                for(;it!=bandStatusMap_[band].second.end();++it)
                {
                    if(conflictMap->find(*it)!=conflictMap->end())
                    {
                        if(conflictMap->find(*it)->second.find(nodeId)!=conflictMap->find(*it)->second.end())
                        {
                            jump_band = true;
                            break;
                        }
                    }
                }

                if(jump_band)
                {
//                    std::cout << NOW << " UE " << nodeId << " --- skipping band " << band << endl;

                    if (!newHole)
                    {
                        // found a hole <holeIndex,holeLen>
                        checkHole(candidate, holeIndex, holeLen, req_RBs);

                        // reset
                        newHole = true;
                        holeIndex = 0;
                        holeLen = 0;
                    }
                    jump_band = false;
                    continue;
                }


                // Going here means that this band can be allocated
                if (newHole)
                {
                    holeIndex = band;
                    newHole = false;
                }
                holeLen++;

            }
        }
        else
        {
            bool jump_band = false;

            // Check if the allocation is possible starting from the first unallocated band (going back)
            for( band=firstUnallocatedBandIM; band>=0; band-- )
            {
                // Jump to the next band if this have been already allocated
                if( alreadyAllocatedBands.find(band) != alreadyAllocatedBands.end() ) jump_band = true;
                /*
                 * Jump to the next band if the node is on Infrastructure and the band is already
                 * allocated to an Infrastructure node (As standard, the same bands are not shared
                 *  between two,or more, nodes in Infrastructure mode).
                 */
                if( !enableFrequencyReuse && bandStatusMap_[band].first == CELLT ) jump_band = true;
                /*
                 * Jump to the next band if:
                 * - the node is an Infrastructure one
                 * - dedicated is true
                 * - the band is occupied by a D2D node.
                 *  If dedicated is "false" an Infrastructure UE can share a band with one or more D2D UEs.
                 */
                if( !enableFrequencyReuse && bandStatusMap_[band].first == D2DT && dedicated_ ) jump_band = true;


                if(jump_band)
                {
                    if (!newHole)
                    {
                        // found a hole <holeIndex,holeLen>
                        checkHole(candidate, holeIndex, holeLen, req_RBs);

                        // reset
                        newHole = true;
                        holeIndex = 0;
                        holeLen = 0;
                    }

                    jump_band = false;
                    continue;
                }

                // Going here means that this band can be allocated
                if (newHole)
                {
                    holeIndex = band;
                    newHole = false;
                }
                holeLen++;

            }
        }

        checkHole(candidate, holeIndex, holeLen, req_RBs);

        if (enableFrequencyReuse || dir == D2D_MULTI)
        {
            // allocate contiguous RBs in the best candidate
            for( band=candidate.index; band < candidate.index+candidate.len; band++ )
            {
                blocks++;

                EV << NOW << " LteAllocatorBestFit - UE " << nodeId << ": allocated RB " << band << " [" <<blocks<<"]" << endl;

                // Book the bands that must be allocated
                bookedBands.push_back(band);
                if (blocks==req_RBs) break; // All the blocks and bytes have been allocated
            }
        }
        else
        {
            // allocate contiguous RBs in the best candidate
            unsigned int end = (candidate.len > candidate.index) ? 0 : candidate.index-candidate.len;
//            for( band=candidate.index; band > candidate.index-candidate.len && band >= 0; band-- )
            for( band=candidate.index; band > end; band-- )
            {
                blocks++;

                // Book the bands that must be allocated
                bookedBands.push_back(band);
                if (blocks==req_RBs) break; // All the blocks and bytes have been allocated
            }
        }

        // If the booked request are sufficient to serve entirely or
        // partially a BSR (at least a RB) do the allocation
        if(blocks<=req_RBs && blocks !=0)
        {
            // Going here means that there's room for allocation (here's the true allocation)
            std::sort(bookedBands.begin(),bookedBands.end());
            std::vector<Band>::iterator it_b = bookedBands.begin();
            // For all the bands previously booked
            for(;it_b!=bookedBands.end();++it_b)
            {
                // TODO Find a correct way to specify plane and antenna
                allocatedRbsPerBand_[MAIN_PLANE][MACRO][*it_b].ueAllocatedRbsMap_[nodeId] += 1;
                allocatedRbsPerBand_[MAIN_PLANE][MACRO][*it_b].ueAllocatedBytesMap_[nodeId] += req_Bytes1RB;
                allocatedRbsPerBand_[MAIN_PLANE][MACRO][*it_b].allocated_ += 1;
            }

            // Add the nodeId to the scheduleList (needed for Grants)
            std::pair<unsigned int, Codeword> scListId;
//            scListId.first = nodeId;
            scListId.first = cid;
            scListId.second = 0; // TODO add support for codewords different from 0
            eNbScheduler_->storeScListId(scListId,blocks);
//            eNbScheduler_->storeScListId(scListId,1);


            // If it is a Cell UE we must reserve the bands
            if (dir == DL || dir == UL)
            {
                setAllocationType(bookedBands,CELLT,nodeId);
            }
            else
            {
                setAllocationType(bookedBands,D2DT,nodeId);
            }

            double byte_served = 0.0;

            // Extract the BSR if an entire BSR is served
            if(blocks == req_RBs)
            {
                // All the bytes have been served
                byte_served = conn->front().first;
                conn->popFront();
            }
            else
            {
                byte_served = blocks*req_Bytes1RB;
                conn->front().first -= (blocks*req_Bytes1RB - MAC_HEADER - RLC_HEADER_UM); // Otherwise update the BSR size
            }

            totalAllocatedBytes += byte_served;
        }

        //Extract the node from the right set
        if(!conflicting_score.empty()) conflicting_score.pop();
        // If the lists are empty we have finished the allocation
        if( conflicting_score.empty()  ) break;

    }

    eNbScheduler_->storeAllocationEnb(allocatedRbsPerBand_, &alreadyAllocatedBands);

    // Reset direction to default direction if changed
    direction_ = (direction_ == D2D)? UL : direction_;

    return;
}

void LteAllocatorBestFit::commitSchedule()
{
    activeConnectionSet_ = activeConnectionTempSet_;
}

void LteAllocatorBestFit::updateSchedulingInfo()
{
}

void LteAllocatorBestFit::notifyActiveConnection(MacCid cid)
{
    EV << NOW << "LteAllocatorBestFit::notify CID notified " << cid << endl;
    activeConnectionSet_.insert(cid);
}

void LteAllocatorBestFit::removeActiveConnection(MacCid cid)
{
    EV << NOW << "LteAllocatorBestFit::remove CID removed " << cid << endl;
    activeConnectionSet_.erase(cid);
}

void LteAllocatorBestFit::initAndReset()
{
    // Clear and reinitialize the allocatedBlocks structures and set available planes to 1 (just the main OFDMA space)
    allocatedRbsPerBand_.clear();
    allocatedRbsPerBand_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    allocatedRbsPerBand_.at(MAIN_PLANE).resize(MACRO + 1);

    allocatedRbsUe_.clear();

    // Clear the OFDMA allocated blocks and set available planes to 1 (just the main OFDMA space)
    allocatedRbsMatrix_.clear();
    allocatedRbsMatrix_.resize(MAIN_PLANE + 1);
    // set the available antennas of MAIN plane to 1 (just MACRO antenna)
    allocatedRbsMatrix_.at(MAIN_PLANE).resize(MACRO + 1, 0);

    // Clear the perUEbandStatusMap
    perUEbandStatusMap_.clear();

    // Clear the bandStatusMap
    bandStatusMap_.clear();
    int numbands = eNbScheduler_->mac_->getAmc()->getSystemNumBands();
    //Set all bands to non-exclusive
    for(int i=0;i<numbands;i++)
    {
        bandStatusMap_[i].first = NONE;
    }

    // Set the dedicated parameter
    dedicated_ = false;
}

void LteAllocatorBestFit::setAllocationType(std::vector<Band> bookedBands,AllocationUeType type,MacNodeId nodeId)
{
    std::vector<Band>::iterator it = bookedBands.begin();
    for(;it!=bookedBands.end();++it)
    {
        bandStatusMap_[*it].first = type;
        bandStatusMap_[*it].second.insert(nodeId);
    }
}
