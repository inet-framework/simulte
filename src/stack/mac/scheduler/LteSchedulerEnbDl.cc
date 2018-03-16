//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/scheduler/LteSchedulerEnbDl.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/buffer/LteMacBuffer.h"

bool
LteSchedulerEnbDl::checkEligibility(MacNodeId id, Codeword& cw)
{
    // check if harq buffer have already been created for this node
    if (mac_->getHarqTxBuffers()->find(id) != mac_->getHarqTxBuffers()->end())
    {
        LteHarqBufferTx* dlHarq = mac_->getHarqTxBuffers()->at(id);
        UnitList freeUnits = dlHarq->firstAvailable();

        if (freeUnits.first != HARQ_NONE)
        {
            if (freeUnits.second.empty())
                // there is a process currently selected for user <id> , but all of its cws have been already used.
                return false;
            // retrieving the cw index
            cw = freeUnits.second.front();
            // DEBUG check
            if (cw > MAX_CODEWORDS)
                throw cRuntimeError("LteSchedulerEnbDl::checkEligibility(): abnormal codeword id %d", (int) cw);
            return true;
        }
    }
    return true;
}

unsigned int
LteSchedulerEnbDl::schedulePerAcidRtx(MacNodeId nodeId, Codeword cw, unsigned char acid,
    std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    // Get user transmission parameters
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);    // get the user info
    // TODO SK Get the number of codewords - FIX with correct mapping
    unsigned int codewords = txParams.getLayers().size();                // get the number of available codewords

    std::string bands_msg = "BAND_LIMIT_SPECIFIED";

    std::vector<BandLimit> tempBandLim;

    Codeword remappedCw = (codewords == 1) ? 0 : cw;

    if (bandLim == NULL)
    {
        bands_msg = "NO_BAND_SPECIFIED";
        // Create a vector of band limit using all bands
        bandLim = &tempBandLim;

        unsigned int numBands = mac_->getCellInfo()->getNumBands();
        // for each band of the band vector provided
        for (unsigned int i = 0; i < numBands; i++)
        {
            BandLimit elem;
            // copy the band
            elem.band_ = Band(i);
            EV << "Putting band " << i << endl;
            // mark as unlimited
            for (Codeword i = 0; i < MAX_CODEWORDS; ++i)
            {
                elem.limit_.push_back(-1);
            }
            bandLim->push_back(elem);
        }
    }
    EV << NOW << "LteSchedulerEnbDl::rtxAcid - Node [" << mac_->getMacNodeId() << "], User[" << nodeId << "],  Codeword [" << cw << "]  of [" << codewords << "] , ACID [" << (int)acid << "] " << endl;
    //! \test REALISTIC!!!  Multi User MIMO support
    if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER))
    {
        // request amc for MU_MIMO pairing
        MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId);
        if (peer != nodeId)
        {
            // this user has a valid pairing
            //1) register pairing  - if pairing is already registered false is returned
            if (allocator_->configureMuMimoPeering(nodeId, peer))
            {
                EV << "LteSchedulerEnb::grant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
            }
            else
            {
                EV << "LteSchedulerEnb::grant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
            }
        }
        else
        {
            EV << "LteSchedulerEnb::grant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
        }
    }
                //!\test experimental DAS support
                // registering DAS spaces to the allocator
    Plane plane = allocator_->getOFDMPlane(nodeId);
    allocator_->setRemoteAntenna(plane, antenna);

    // blocks to allocate for each band
    std::vector<unsigned int> assignedBlocks;
    // bytes which blocks from the preceding vector are supposed to satisfy
    std::vector<unsigned int> assignedBytes;
    LteHarqBufferTx* currHarq = mac_->getHarqTxBuffers()->at(nodeId);

    // bytes to serve
    unsigned int bytes = currHarq->pduLength(acid, cw);

    // check selected process status.
    std::vector<UnitStatus> pStatus = currHarq->getProcess(acid)->getProcessStatus();
    std::vector<UnitStatus>::iterator vit = pStatus.begin(), vet = pStatus.end();

    Codeword allocatedCw = 0;
    // search for already allocated codeword
//    for (;vit!=vet;++vit)
//    {
//        // skip current codeword
//        if (vit->first==cw) continue;
//
//        if (vit->second == TXHARQ_PDU_SELECTED)
//        {
//            allocatedCw=vit->first;
//            break;
//        }
//
//    }
    if (allocatedCws_.find(nodeId) != allocatedCws_.end())
    {
        allocatedCw = allocatedCws_.at(nodeId);
    }
    // for each band
    unsigned int size = bandLim->size();

    for (unsigned int i = 0; i < size; ++i)
    {
        // save the band and the relative limit
        Band b = bandLim->at(i).band_;

        int limit = bandLim->at(i).limit_.at(remappedCw);
        unsigned int available = 0;
        // if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
        if ((allocatedCw != 0))
        {
            // get band allocated blocks
            int b1 = allocator_->getBlocks(antenna, b, nodeId);

            // limit eventually allocated blocks on other codeword to limit for current cw
            //b1 = (limitBl ? (b1>limit?limit:b1) : b1);
            available = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, remappedCw, b1, direction_);
        }
        else
            available = availableBytes(nodeId, antenna, b, remappedCw, direction_, (limitBl) ? limit : -1);    // available space

        // use the provided limit as cap for available bytes, if it is not set to unlimited
        if (limit >= 0 && !limitBl)
            available = limit < (int) available ? limit : available;

        EV << NOW << "LteSchedulerEnbDl::rtxAcid ----- BAND " << b << "-----" << endl;
        EV << NOW << "LteSchedulerEnbDl::rtxAcid To serve: " << bytes << " bytes" << endl;
        EV << NOW << "LteSchedulerEnbDl::rtxAcid Available: " << available << " bytes" << endl;

        unsigned int allocation = 0;
        if (available < bytes)
        {
            allocation = available;
            bytes -= available;
        }
        else
        {
            allocation = bytes;
            bytes = 0;
        }

        if ((allocatedCw == 0))
        {
            unsigned int blocks = mac_->getAmc()->computeReqRbs(nodeId, b, remappedCw, allocation, direction_);

            EV << NOW << "LteSchedulerEnbDl::rtxAcid Assigned blocks: " << blocks << "  blocks" << endl;

            // assign only on the first codeword
            assignedBlocks.push_back(blocks);
            assignedBytes.push_back(allocation);
        }

        if (bytes == 0)
            break;
    }

    if (bytes > 0)
    {
        // process couldn't be served
        EV << NOW << "LteSchedulerEnbDl::rtxAcid Cannot serve HARQ Process" << acid << endl;
        return 0;
    }

        // record the allocation if performed
    size = assignedBlocks.size();
    // For each LB with assigned blocks
    for (unsigned int i = 0; i < size; ++i)
    {
        if (allocatedCw == 0)
        {
            // allocate the blocks
            allocator_->addBlocks(antenna, bandLim->at(i).band_, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
        }
        // store the amount
        bandLim->at(i).limit_.at(remappedCw) = assignedBytes.at(i);
    }

    UnitList signal;
    signal.first = acid;
    signal.second.push_back(cw);

    EV << NOW << " LteSchedulerEnbDl::rtxAcid HARQ Process " << (int)acid << "  codeword  " << cw << " marking for retransmission " << endl;

    // if allocated codewords is not MAX_CODEWORDS, then there's another allocated codeword , update the codewords variable :

    if (allocatedCw != 0)
    {
        // TODO fixme this only works if MAX_CODEWORDS ==2
        --codewords;
        if (codewords <= 0)
            throw cRuntimeError("LteSchedulerEnbDl::rtxAcid(): erroneus codeword count %d", codewords);
    }

    // signal a retransmission
    currHarq->markSelected(signal, codewords);

    // mark codeword as used
    if (allocatedCws_.find(nodeId) != allocatedCws_.end())
    {
        allocatedCws_.at(nodeId)++;
        }
    else
    {
        allocatedCws_[nodeId] = 1;
    }

    bytes = currHarq->pduLength(acid, cw);

    EV << NOW << " LteSchedulerEnbDl::rtxAcid HARQ Process " << (int)acid << "  codeword  " << cw << ", " << bytes << " bytes served!" << endl;

    return bytes;
}

bool
LteSchedulerEnbDl::rtxschedule()
{
    EV << NOW << " LteSchedulerEnbDl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
    EV << NOW << " LteSchedulerEnbDl::rtxschedule Cell:  " << mac_->getMacCellId() << endl;
    EV << NOW << " LteSchedulerEnbDl::rtxschedule Direction: " << (direction_ == DL ? "DL" : "UL") << endl;

    // retrieving reference to HARQ entities
    HarqTxBuffers* harqQueues = mac_->getHarqTxBuffers();

    HarqTxBuffers::iterator it = harqQueues->begin();
    HarqTxBuffers::iterator et = harqQueues->end();

    // examination of HARQ process in rtx status, adding them to scheduling list
    for(; it != et; ++it)
    {
        // For each UE
        MacNodeId nodeId = it->first;

        OmnetId id = binder_->getOmnetId(nodeId);
        if(id == 0){
            // UE has left the simulation, erase HARQ-queue
            it = harqQueues->erase(it);
            if(it == et)
                break;
            else
                continue;
        }
        LteHarqBufferTx* currHarq = it->second;
        std::vector<LteHarqProcessTx *> * processes = currHarq->getHarqProcesses();

        // Get user transmission parameters
        const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_);// get the user info
        // TODO SK Get the number of codewords - FIX with correct mapping
        unsigned int codewords = txParams.getLayers().size();// get the number of available codewords

        EV << NOW << " LteSchedulerEnbDl::rtxschedule  UE: " << nodeId << endl;
        EV << NOW << " LteSchedulerEnbDl::rtxschedule Number of codewords: " << codewords << endl;

        // get the number of HARQ processes
        unsigned int process=0;
        unsigned int maxProcesses = currHarq->getNumProcesses();
        for(process = 0; process < maxProcesses; ++process )
        {
            // for each HARQ process
            LteHarqProcessTx * currProc = (*processes)[process];

            if (allocatedCws_[nodeId] == codewords)
                break;
            for (Codeword cw = 0; cw < codewords; ++cw)
            {

                if (allocatedCws_[nodeId]==codewords)
                    break;
                EV << NOW << " LteSchedulerEnbDl::rtxschedule process " << process << endl;
                EV << NOW << " LteSchedulerEnbDl::rtxschedule ------- CODEWORD " << cw << endl;

                // skip processes which are not in rtx status
                if (currProc->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED)
                {
                    EV << NOW << " LteSchedulerEnbDl::rtxschedule detected Acid: " << process << " in status " << currProc->getUnitStatus(cw) << endl;
                    continue;
                }

                EV << NOW << " LteSchedulerEnbDl::rtxschedule " << endl;
                EV << NOW << " LteSchedulerEnbDl::rtxschedule detected RTX Acid: " << process << endl;

                // perform the retransmission
                unsigned int bytes = schedulePerAcidRtx(nodeId,cw,process);

                // if a value different from zero is returned, there was a service
                if(bytes > 0)
                {
                    EV << NOW << " LteSchedulerEnbDl::rtxschedule CODEWORD IS NOW BUSY!!!" << endl;
                    // do not process this HARQ process anymore
                    // go to next codeword
                    break;
                }
            }
        }
    }

    unsigned int availableBlocks = allocator_->computeTotalRbs();
    EV << " LteSchedulerEnbDl::rtxschedule OFDM Space: " << availableBlocks << endl;
    EV << "    LteSchedulerEnbDl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::-------------------- " << endl;

    return (availableBlocks == 0);
}

/*  COMPLETE:        grant(cid,bytes,terminate,active,eligible,band_limit,antenna);
 *  ANTENNA UNAWARE: grant(cid,bytes,terminate,active,eligible,band_limit);
 *  BAND UNAWARE:    grant(cid,bytes,terminate,active,eligible);
 */
unsigned int LteSchedulerEnbDl::scheduleGrant(MacCid cid, unsigned int bytes,
        bool& terminate, bool& active, bool& eligible, std::vector<BandLimit>* bandLim,
        Remote antenna, bool limitBl)
{
    // Get the node ID and logical connection ID
    MacNodeId nodeId = MacCidToNodeId(cid);
    LogicalCid flowId = MacCidToLcid(cid);
    // Get user transmission parameters
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId,
        direction_);
    //get the number of codewords
    unsigned int numCodewords = txParams.getLayers().size();

    // TEST: check the number of codewords
    numCodewords = 1;

    std::string bands_msg = "BAND_LIMIT_SPECIFIED";

    std::vector<BandLimit> tempBandLim;

    if (bandLim == NULL)
    {
        bands_msg = "NO_BAND_SPECIFIED";
        // Create a vector of band limit using all bands
        bandLim = &tempBandLim;

        txParams.print("grant()");

        unsigned int numBands = mac_->getCellInfo()->getNumBands();
        // for each band of the band vector provided
        for (unsigned int i = 0; i < numBands; i++)
        {
            BandLimit elem;
            // copy the band
            elem.band_ = Band(i);
            EV << "Putting band " << i << endl;
            // mark as unlimited
            for (unsigned int j = 0; j < numCodewords; j++)
            {
                EV << "- Codeword " << j << endl;
                elem.limit_.push_back(-1);
            }
            bandLim->push_back(elem);
        }
    }
    EV << "LteSchedulerEnbDl::grant(" << cid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << "," << dasToA(antenna) << ")" << endl;

    // Perform normal operation for grant

    // Get virtual buffer reference (it shouldn't be direction UL)
    LteMacBuffer* conn = ((direction_ == DL) ? vbuf_->at(cid) : bsrbuf_->at(cid));

    // get the buffer size
    unsigned int queueLength = conn->getQueueOccupancy(); // in bytes

    // get traffic descriptor
    FlowControlInfo connDesc = mac_->getConnDesc().at(cid);

    EV << "LteSchedulerEnbDl::grant --------------------::[ START GRANT ]::--------------------" << endl;
    EV << "LteSchedulerEnbDl::grant Cell: " << mac_->getMacCellId() << endl;
    EV << "LteSchedulerEnbDl::grant CID: " << cid << "(UE: " << nodeId << ", Flow: " << flowId << ") current Antenna [" << dasToA(antenna) << "]" << endl;

    //! Multiuser MIMO support
    if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER))
    {
        // request AMC for MU_MIMO pairing
        MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId, direction_);
        if (peer != nodeId)
        {
            // this user has a valid pairing
            //1) register pairing  - if pairing is already registered false is returned
            if (allocator_->configureMuMimoPeering(nodeId, peer))
            {
                EV << "LteSchedulerEnbDl::grant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
            }
            else
            {
                EV << "LteSchedulerEnbDl::grant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
            }
        }
        else
        {
            EV << "LteSchedulerEnbDl::grant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
        }
    }

                // registering DAS spaces to the allocator
    Plane plane = allocator_->getOFDMPlane(nodeId);
    allocator_->setRemoteAntenna(plane, antenna);

    unsigned int cwAlredyAllocated = 0;
    // search for already allocated codeword

    if (allocatedCws_.find(nodeId) != allocatedCws_.end())
    {
        cwAlredyAllocated = allocatedCws_.at(nodeId);
    }
    // Check OFDM space
    // OFDM space is not zero if this if we are trying to allocate the second cw in SPMUX or
    // if we are tryang to allocate a peer user in mu_mimo plane
    if (allocator_->computeTotalRbs() == 0 && (((txParams.readTxMode() != OL_SPATIAL_MULTIPLEXING &&
        txParams.readTxMode() != CL_SPATIAL_MULTIPLEXING) || cwAlredyAllocated == 0) &&
        (txParams.readTxMode() != MULTI_USER || plane != MU_MIMO_PLANE)))
    {
        terminate = true; // ODFM space ended, issuing terminate flag
        EV << "LteSchedulerEnbDl::grant Space ended, no schedulation." << endl;
        return 0;
    }

    // TODO This is just a BAD patch
    // check how a codeword may be reused (as in the if above) in case of non-empty OFDM space
    // otherwise check why an UE is stopped being scheduled while its buffer is not empty
    if (cwAlredyAllocated > 0)
    {
        terminate = true;
        return 0;
    }

    // DEBUG OUTPUT
    if (limitBl)
        EV << "LteSchedulerEnbDl::grant blocks: " << bytes << endl;
        else
        EV << "LteSchedulerEnbDl::grant Bytes: " << bytes << endl;
    EV << "LteSchedulerEnbDl::grant Bands: {";
    unsigned int size = (*bandLim).size();
    if (size > 0)
    {
        EV << (*bandLim).at(0).band_;
        for(unsigned int i = 1; i < size; i++)
        EV << ", " << (*bandLim).at(i).band_;
    }
    EV << "}\n";

    EV << "LteSchedulerEnbDl::grant TxMode: " << txModeToA(txParams.readTxMode()) << endl;
    EV << "LteSchedulerEnbDl::grant Available codewords: " << numCodewords << endl;

    bool stop = false;
    unsigned int totalAllocatedBytes = 0; // total allocated data (in bytes)
    unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)
    Codeword cw = 0; // current codeword, modified by reference by the checkeligibility function

    // Retrieve the first free codeword checking the eligibility - check eligibility could modify current cw index.
    if (!checkEligibility(nodeId, cw) || cw >= numCodewords)
    {
        eligible = false;

        EV << "LteSchedulerEnbDl::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;
        EV << "LteSchedulerEnbDl::grant Total allocation: " << totalAllocatedBytes << "bytes" << endl;
        EV << "LteSchedulerEnbDl::grant NOT ELIGIBLE!!!" << endl;
        EV << "LteSchedulerEnbDl::grant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes; // return the total number of served bytes
    }

    for (; cw < numCodewords; ++cw)
    {
        unsigned int cwAllocatedBytes = 0;
        // used by uplink only, for signaling cw blocks usage to schedule list
        unsigned int cwAllocatedBlocks = 0;
        // per codeword vqueue item counter (UL: BSRs DL: SDUs)
        unsigned int vQueueItemCounter = 0;

        unsigned int allocatedCws = 0;

        std::list<Request> bookedRequests;

        // band limit structure

        EV << "LteSchedulerEnbDl::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;

        unsigned int size = (*bandLim).size();

        unsigned int toBook;
        // Check whether the virtual buffer is empty
        if (queueLength == 0)
        {
            active = false; // all user data have been served
            EV << "LteSchedulerEnbDl::grant scheduled connection is no more active . Exiting grant " << endl;
            stop = true;
            break;
        }
        else
        {
            // we need to consider also the size of RLC and MAC headers

            if (connDesc.getRlcType() == UM)
                queueLength += RLC_HEADER_UM;
            else if (connDesc.getRlcType() == AM)
                queueLength += RLC_HEADER_AM;
            queueLength += MAC_HEADER;

            toBook = queueLength;
        }

        EV << "LteSchedulerEnbDl::grant bytes to be allocated: " << toBook << endl;

        // Book bands for this connection
        for (unsigned int i = 0; i < size; ++i)
        {
            // for sulle bande
            unsigned int bandAllocatedBytes = 0;

            // save the band and the relative limit
            Band b = (*bandLim).at(i).band_;
            int limit = (*bandLim).at(i).limit_.at(cw);

            EV << "LteSchedulerEnbDl::grant --- BAND " << b << " LIMIT " << limit << "---" << endl;

            // if the limit flag is set to skip, jump off
            if (limit == -2)
            {
                EV << "LteSchedulerEnbDl::grant skipping logical band according to limit value" << endl;
                continue;
            }


            // search for already allocated codeword

            if (allocatedCws_.find(nodeId) != allocatedCws_.end())
            {
                allocatedCws = allocatedCws_.at(nodeId);
            }
            unsigned int bandAvailableBytes = 0;
            unsigned int bandAvailableBlocks = 0;
            // if there is a previous blocks allocation on the first codeword, blocks allocation is already available
            if (allocatedCws != 0)
            {
                // get band allocated blocks
                int b1 = allocator_->getBlocks(antenna, b, nodeId);
                // limit eventually allocated blocks on other codeword to limit for current cw
                bandAvailableBlocks = (limitBl ? (b1 > limit ? limit : b1) : b1);

                //    bandAvailableBlocks=b1;

                bandAvailableBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, bandAvailableBlocks, direction_);
            }
            // if limit is expressed in blocks, limit value must be passed to availableBytes function
            else
            {
                bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_,(limitBl) ? limit : -1); // available space (in bytes)
                bandAvailableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
            }

            // if no allocation can be performed, notify to skip
            // the band on next processing (if any)

            if (bandAvailableBytes == 0)
            {
                EV << "LteSchedulerEnbDl::grant Band " << b << "will be skipped since it has no space left." << endl;
                (*bandLim).at(i).limit_.at(cw) = -2;
                continue;
            }
                //if bandLimit is expressed in bytes
            if (!limitBl)
            {
                // use the provided limit as cap for available bytes, if it is not set to unlimited
                if (limit >= 0 && limit < (int) bandAvailableBytes)
                {
                    bandAvailableBytes = limit;
                    EV << "LteSchedulerEnbDl::grant Band space limited to " << bandAvailableBytes << " bytes according to limit cap" << endl;
                }
            }
            else
            {
                // if bandLimit is expressed in blocks
                if(limit >= 0 && limit < (int) bandAvailableBlocks)
                {
                    bandAvailableBlocks=limit;
                    EV << "LteSchedulerEnbDl::grant Band space limited to " << bandAvailableBlocks << " blocks according to limit cap" << endl;
                }
            }

            EV << "LteSchedulerEnbDl::grant Available Bytes: " << bandAvailableBytes << " available blocks " << bandAvailableBlocks << endl;




            // book resources on this band

            unsigned int blocksAdded = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bandAllocatedBytes,
                direction_);

            if (blocksAdded > bandAvailableBlocks)
                throw cRuntimeError("band %d GRANT allocation overflow : avail. blocks %d alloc. blocks %d", b,
                    bandAvailableBlocks, blocksAdded);

            EV << "LteSchedulerEnbDl::grant Booking band available blocks" << (bandAvailableBlocks-blocksAdded) << " [" << bandAvailableBytes << " bytes] for future use, going to next band" << endl;
            // enable booking  here
            bookedRequests.push_back(Request(b, bandAvailableBytes, bandAvailableBlocks - blocksAdded));


            // update the counter of bytes to be served
            toBook = (bandAvailableBytes > toBook) ? 0 : toBook - bandAvailableBytes;

            if (toBook == 0)
            {
                // all bytes booked, go to allocation
                stop = true;
                active = false;
                break;
            }
            // else continue booking (if there are available bands)
        }

        // allocation of the booked resources

        // compute here total booked requests
        unsigned int totalBooked = 0;
        unsigned int bookedUsed = 0;

        std::list<Request>::iterator li = bookedRequests.begin(), le = bookedRequests.end();
        for (; li != le; ++li)
        {
            totalBooked += li->bytes_;
            EV << "LteSchedulerEnbDl::grant Band " << li->b_ << " can contribute with " << li->bytes_ << " of booked resources " << endl;
        }

        // get resources to allocate
        unsigned int toServe = queueLength - toBook;

        EV << "LteSchedulerEnbDl::grant servicing " << toServe << " bytes with " << totalBooked << " booked bytes " << endl;

        // decrease booking value - if totalBooked is greater than 0, we used booked resources for scheduling the pdu
        if (totalBooked>0)
        {
            // reset booked resources iterator.
            li=bookedRequests.begin();

            EV << "LteSchedulerEnbDl::grant Making use of booked resources [" << totalBooked << "] for inter-band data allocation" << endl;
            // updating booked requests structure
            while ((li!=le) && (bookedUsed<=toServe))
            {
                Band u = li->b_;
                unsigned int uBytes = ((li->bytes_ > toServe )? toServe : li->bytes_ );

                EV << "LteSchedulerEnbDl::grant allocating " << uBytes << " prev. booked bytes on band " << (unsigned short)u << endl;

                // mark here the usage of booked resources
                bookedUsed+= uBytes;
                cwAllocatedBytes+=uBytes;

                unsigned int uBlocks = mac_->getAmc()->computeReqRbs(nodeId,u, cw, uBytes, direction_);

                if (uBlocks <= li->blocks_)
                {
                    li->blocks_-=uBlocks;
                }
                else
                {
                    li->blocks_=uBlocks=0;
                }

                // add allocated blocks for this codeword
                cwAllocatedBlocks += uBlocks;

                // update limit
                if (uBlocks>0)
                {
                    unsigned int j=0;
                    for (;j<(*bandLim).size();++j) if ((*bandLim).at(j).band_==u) break;

                    if((*bandLim).at(j).limit_.at(cw) > 0)
                    {
                        (*bandLim).at(j).limit_.at(cw) -= uBlocks;

                        if ((*bandLim).at(j).limit_.at(cw) < 0)
                        throw cRuntimeError("Limit decreasing error during booked resources allocation on band %d : new limit %d, due to blocks %d ",
                            u,(*bandLim).at(j).limit_.at(cw),uBlocks);
                    }
                }

                if(allocatedCws == 0)
                {
                    // mark here allocation
                    allocator_->addBlocks(antenna,u,nodeId,uBlocks,uBytes);
                }

                // update reserved status

                if (li->bytes_>toServe)
                {
                    li->bytes_-=toServe;

                    li++;
                }
                else
                {
                    std::list<Request>::iterator erase = li;
                    // increment pointer.
                    li++;
                    // erase element from list
                    bookedRequests.erase(erase);

                    EV << "LteSchedulerEnbDl::grant band " << (unsigned short)u << " depleted all its booked resources " << endl;
                }
            }
            vQueueItemCounter++;
        }

        // update virtual buffer
        unsigned int alloc = toServe;
        alloc -= MAC_HEADER;
        if (connDesc.getRlcType() == UM)
            alloc -= RLC_HEADER_UM;
        else if (connDesc.getRlcType() == AM)
            alloc -= RLC_HEADER_AM;
        // alloc is the number of effective bytes allocated (without overhead)
        while (!conn->isEmpty() && alloc > 0)
        {
            unsigned int vPktSize = conn->front().first;
            if (vPktSize <= alloc)
            {
                // serve the entire vPkt
                conn->popFront();
                alloc -= vPktSize;
            }
            else
            {
                // serve partial vPkt

                // update pkt info
                PacketInfo newPktInfo = conn->popFront();
                newPktInfo.first = newPktInfo.first - alloc;
                conn->pushFront(newPktInfo);
                alloc = 0;
            }
        }

        EV << "LteSchedulerEnbDl::grant Codeword allocation: " << cwAllocatedBytes << "bytes" << endl;

        if (cwAllocatedBytes > 0)
        {
            // mark codeword as used
            if (allocatedCws_.find(nodeId) != allocatedCws_.end())
            {
                allocatedCws_.at(nodeId)++;
            }
            else
            {
                allocatedCws_[nodeId] = 1;
            }

            totalAllocatedBytes += cwAllocatedBytes;

            std::pair<unsigned int, Codeword> scListId;
            scListId.first = cid;
            scListId.second = cw;

            if (scheduleList_.find(scListId) == scheduleList_.end())
                scheduleList_[scListId] = 0;

            // if direction is DL , then schedule list contains number of to-be-trasmitted SDUs ,
            // otherwise it contains number of granted blocks
            scheduleList_[scListId] += ((direction_ == DL) ? vQueueItemCounter : cwAllocatedBlocks);

            EV << "LteSchedulerEnbDl::grant CODEWORD IS NOW BUSY: GO TO NEXT CODEWORD." << endl;
            if (allocatedCws_.at(nodeId) == MAX_CODEWORDS)
            {
                eligible = false;
                stop = true;
            }
        }
        else
        {
            EV << "LteSchedulerEnbDl::grant CODEWORD IS FREE: NO ALLOCATION IS POSSIBLE IN NEXT CODEWORD." << endl;
            eligible = false;
            stop = true;
        }
        if (stop)
            break;


    } // end for codeword

    EV << "LteSchedulerEnbDl::grant Total allocation: " << totalAllocatedBytes << " bytes, " << totalAllocatedBlocks << " blocks" << endl;
    EV << "LteSchedulerEnbDl::grant --------------------::[  END GRANT  ]::--------------------" << endl;

    return totalAllocatedBytes;
}
