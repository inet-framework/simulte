//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/allocator/LteAllocationModuleFrequencyReuse.h"
#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/scheduling_modules/LteDrr.h"
#include "stack/mac/scheduling_modules/LteMaxCi.h"
#include "stack/mac/scheduling_modules/LtePf.h"
#include "stack/mac/scheduling_modules/LteMaxCiMultiband.h"
#include "stack/mac/scheduling_modules/LteMaxCiOptMB.h"
#include "stack/mac/scheduling_modules/LteMaxCiComp.h"
#include "stack/mac/scheduling_modules/LteAllocatorBestFit.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/buffer/LteMacQueue.h"

LteSchedulerEnb::LteSchedulerEnb()
{
    direction_ = DL;
    mac_ = 0;
    allocator_ = 0;
    scheduler_ = 0;
    vbuf_ = 0;
    harqTxBuffers_ = 0;
    harqRxBuffers_ = 0;
    resourceBlocks_ = 0;

    // ********************************
    //    sleepSize_ = 0;
    //    sleepShift_ = 0;
    //    zeroLevel_ = .0;
    //    idleLevel_ = .0;
    //    powerUnit_ = .0;
    //    maxPower_ = .0;
}

LteSchedulerEnb::~LteSchedulerEnb()
{
    delete allocator_;
    if(scheduler_)
        delete scheduler_;
}

void LteSchedulerEnb::initialize(Direction dir, LteMacEnb* mac)
{
    direction_ = dir;
    mac_ = mac;

    binder_ = getBinder();

    vbuf_ = mac_->getMacBuffers();
    bsrbuf_ = mac_->getBsrVirtualBuffers();

    harqTxBuffers_ = mac_->getHarqTxBuffers();
    harqRxBuffers_ = mac_->getHarqRxBuffers();

    // Create LteScheduler
    SchedDiscipline discipline = mac_->getSchedDiscipline(direction_);

    scheduler_ = getScheduler(discipline);
    scheduler_->setEnbScheduler(this);

    // Create Allocator
    if (discipline == ALLOCATOR_BESTFIT)   // NOTE: create this type of allocator for every scheduler using Frequency Reuse
        allocator_ = new LteAllocationModuleFrequencyReuse(mac_, direction_);
    else
        allocator_ = new LteAllocationModule(mac_, direction_);

    // Initialize statistics
    cellBlocksUtilizationDl_ = mac_->registerSignal("cellBlocksUtilizationDl");
    cellBlocksUtilizationUl_ = mac_->registerSignal("cellBlocksUtilizationUl");
    lteAvgServedBlocksDl_ = mac_->registerSignal("avgServedBlocksDl");
    lteAvgServedBlocksUl_ = mac_->registerSignal("avgServedBlocksUl");
    depletedPowerDl_ = mac_->registerSignal("depletedPowerDl");
    depletedPowerUl_ = mac_->registerSignal("depletedPowerUl");
}

LteMacScheduleList* LteSchedulerEnb::schedule()
{
    EV << "LteSchedulerEnb::schedule performed by Node: " << mac_->getMacNodeId() << endl;

    // clearing structures for new scheduling
    scheduleList_.clear();
    allocatedCws_.clear();

    // clean the allocator
    initAndResetAllocator();
    //reset AMC structures
    mac_->getAmc()->cleanAmcStructures(direction_,scheduler_->readActiveSet());

    // scheduling of retransmission and transmission
    EV << "___________________________start RTX __________________________________" << endl;
    if(!(scheduler_->scheduleRetransmissions()))
    {
        EV << "____________________________ end RTX __________________________________" << endl;
        EV << "___________________________start SCHED ________________________________" << endl;
        scheduler_->updateSchedulingInfo();
        scheduler_->schedule();
        EV << "____________________________ end SCHED ________________________________" << endl;
    }

    // record assigned resource blocks statistics
    resourceBlockStatistics();
    return &scheduleList_;
}

    /*  COMPLETE:        grant(cid,bytes,terminate,active,eligible,band_limit,antenna);
     *  ANTENNA UNAWARE: grant(cid,bytes,terminate,active,eligible,band_limit);
     *  BAND UNAWARE:    grant(cid,bytes,terminate,active,eligible);
     */
unsigned int LteSchedulerEnb::scheduleGrant(MacCid cid, unsigned int bytes,
    bool& terminate, bool& active, bool& eligible,
    std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    // Get the node ID and logical connection ID
    MacNodeId nodeId = MacCidToNodeId(cid);
    LogicalCid flowId = MacCidToLcid(cid);

    Direction dir = direction_;
    if (dir == UL)
    {
        // check if this connection is a D2D connection
        if (flowId == D2D_SHORT_BSR)
            dir = D2D;           // if yes, change direction
        if (flowId == D2D_MULTI_SHORT_BSR)
            dir = D2D_MULTI;     // if yes, change direction
    }

    // Get user transmission parameters
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, dir);
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

        unsigned int numBands = mac_->getDeployer()->getNumBands();
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
    EV << "LteSchedulerEnb::grant(" << cid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << "," << dasToA(antenna) << ")" << endl;

    // Perform normal operation for grant

    // Get virtual buffer reference
    LteMacBuffer* conn = ((dir == DL) ? vbuf_->at(cid) : bsrbuf_->at(cid));

    EV << "LteSchedulerEnb::grant --------------------::[ START GRANT ]::--------------------" << endl;
    EV << "LteSchedulerEnb::grant Cell: " << mac_->getMacCellId() << endl;
    EV << "LteSchedulerEnb::grant CID: " << cid << "(UE: " << nodeId << ", Flow: " << flowId << ") current Antenna [" << dasToA(antenna) << "]" << endl;

    //! Multiuser MIMO support
    if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER))
    {
        // request AMC for MU_MIMO pairing
        MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId, dir);
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
        EV << "LteSchedulerEnb::grant Space ended, no schedulation." << endl;
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
        EV << "LteSchedulerEnb::grant blocks: " << bytes << endl;
        else
        EV << "LteSchedulerEnb::grant Bytes: " << bytes << endl;
    EV << "LteSchedulerEnb::grant Bands: {";
    unsigned int size = (*bandLim).size();
    if (size > 0)
    {
        EV << (*bandLim).at(0).band_;
        for(unsigned int i = 1; i < size; i++)
        EV << ", " << (*bandLim).at(i).band_;
    }
    EV << "}\n";

    EV << "LteSchedulerEnb::grant TxMode: " << txModeToA(txParams.readTxMode()) << endl;
    EV << "LteSchedulerEnb::grant Available codewords: " << numCodewords << endl;

    bool stop = false;
    unsigned int totalAllocatedBytes = 0; // total allocated data (in bytes)
    unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)
    Codeword cw = 0; // current codeword, modified by reference by the checkeligibility function

    // Retrieve the first free codeword checking the eligibility - check eligibility could modify current cw index.
    if (!checkEligibility(nodeId, cw) || cw >= numCodewords)
    {
        eligible = false;

        EV << "LteSchedulerEnb::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;
        EV << "LteSchedulerEnb::grant Total allocation: " << totalAllocatedBytes << "bytes" << endl;
        EV << "LteSchedulerEnb::grant NOT ELIGIBLE!!!" << endl;
        EV << "LteSchedulerEnb::grant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes; // return the total number of served bytes
    }

    for (; cw < numCodewords; ++cw)
    {
        unsigned int cwAllocatedBytes = 0;
        // used by uplink only, for signaling cw blocks usage to schedule list
        unsigned int cwAllocatedBlocks = 0;
        // per codeword vqueue item counter (UL: BSRs DL: SDUs)
        unsigned int vQueueItemCounter = 0;

        std::list<Request> bookedRequests;

        // band limit structure

        EV << "LteSchedulerEnb::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;

        unsigned int size = (*bandLim).size();

        bool firstSdu = true;

        for (unsigned int i = 0; i < size; ++i)
        {
            // for sulle bande
            unsigned int bandAllocatedBytes = 0;

            // save the band and the relative limit
            Band b = (*bandLim).at(i).band_;
            int limit = (*bandLim).at(i).limit_.at(cw);

            EV << "LteSchedulerEnb::grant --- BAND " << b << " LIMIT " << limit << "---" << endl;

            // if the limit flag is set to skip, jump off
            if (limit == -2)
            {
                EV << "LteSchedulerEnb::grant skipping logical band according to limit value" << endl;
                continue;
            }

            unsigned int allocatedCws = 0;
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

                bandAvailableBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, bandAvailableBlocks, dir);
            }
            // if limit is expressed in blocks, limit value must be passed to availableBytes function
            else
            {
                bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, dir, (limitBl) ? limit : -1); // available space (in bytes)
                bandAvailableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
            }

            // if no allocation can be performed, notify to skip
            // the band on next processing (if any)

            if (bandAvailableBytes == 0)
            {
                EV << "LteSchedulerEnb::grant Band " << b << "will be skipped since it has no space left." << endl;
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
                    EV << "LteSchedulerEnb::grant Band space limited to " << bandAvailableBytes << " bytes according to limit cap" << endl;
                }
            }
            else
            {
                // if bandLimit is expressed in blocks
                if(limit >= 0 && limit < (int) bandAvailableBlocks)
                {
                    bandAvailableBlocks=limit;
                    EV << "LteSchedulerEnb::grant Band space limited to " << bandAvailableBlocks << " blocks according to limit cap" << endl;
                }
            }

            EV << "LteSchedulerEnb::grant Available Bytes: " << bandAvailableBytes << " available blocks " << bandAvailableBlocks << endl;

            // TODO Consider the MAC PDU header size.

            // Process the SDUs of the specified flow
            while (true)
            {
                // Check whether the virtual buffer is empty
                if (conn->isEmpty())
                {
                    active = false; // all user data have been served
                    EV << "LteSchedulerEnb::grant scheduled connection is no more active . Exiting grant " << endl;
                    stop = true;
                    break;
                }

                PacketInfo vpkt = conn->front();
                unsigned int vQueueFrontSize = vpkt.first;
                unsigned int toServe = vQueueFrontSize;

                // for the first SDU, add 2 byte for the MAC PDU Header
                if (firstSdu)
                {
                    toServe += RLC_HEADER_UM;
                    toServe += MAC_HEADER;
                }

                //   Check if the flow will overflow its request by adding an entire SDU
                if (((vQueueFrontSize + totalAllocatedBytes + cwAllocatedBytes + bandAllocatedBytes) > bytes)
                    && bytes != 0)
                {
                    if (dir == DL)
                    {
                        // can't schedule partial SDUs
                        EV << "LteSchedulerEnb::grant available grant is  :  " << bytes - (totalAllocatedBytes + cwAllocatedBytes + bandAllocatedBytes) << ", insufficient for SDU size : " << vQueueFrontSize << endl;
                        stop = true;
                        break;
                    }
                    else
                    {
                        // update toServe to : bytes requested by grant minus so-far-allocated bytes
                        toServe = bytes - totalAllocatedBytes-cwAllocatedBytes - bandAllocatedBytes;
                    }
                }

                        // compute here total booked requests

                unsigned int totalBooked = 0;
                unsigned int bookedUsed = 0;

                std::list<Request>::iterator li = bookedRequests.begin(), le = bookedRequests.end();

                for (; li != le; ++li)
                {
                    totalBooked += li->bytes_;
                    EV << "LteSchedulerEnb::grant Band " << li->b_ << " can contribute with " << li->bytes_ << " of booked resources " << endl;
                }

                // The current SDU may have a size greater than available space. In this case, book the band.
                // During UL/D2D scheduling, if this is the last band, it means that there is no more space
                // for booking resources, thus we allocate partial BSR
                if (toServe > (bandAvailableBytes + totalBooked) && (direction_ == DL || ((direction_ == UL || direction_ == D2D || direction_ == D2D_MULTI) && b < size-1)))
                {
                    unsigned int blocksAdded = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bandAllocatedBytes,        // substitute bandAllocatedBytes
                        dir);

                    if (blocksAdded > bandAvailableBlocks)
                        throw cRuntimeError("band %d GRANT allocation overflow : avail. blocks %d alloc. blocks %d", b,
                            bandAvailableBlocks, blocksAdded);

                    EV << "LteSchedulerEnb::grant Booking band available blocks" << (bandAvailableBlocks-blocksAdded) << " [" << bandAvailableBytes << " bytes] for future use, going to next band" << endl;
                    // enable booking  here
                    bookedRequests.push_back(Request(b, bandAvailableBytes, bandAvailableBlocks - blocksAdded));
                    //  skipping this band
                    break;
                }
                else
                {
                    EV << "LteSchedulerEnb::grant servicing " << toServe << " bytes with " << bandAvailableBytes << " own bytes and " << totalBooked << " booked bytes " << endl;

                    // decrease booking value - if totalBooked is greater than 0, we used booked resources for scheduling the pdu
                    if (totalBooked>0)
                    {
                        // reset booked resources iterator.
                        li=bookedRequests.begin();

                        EV << "LteSchedulerEnb::grant Making use of booked resources [" << totalBooked << "] for inter-band data allocation" << endl;
                        // updating booked requests structure
                        while ((li!=le) && (bookedUsed<=toServe))
                        {
                            Band u = li->b_;
                            unsigned int uBytes = ((li->bytes_ > toServe )? toServe : li->bytes_ );

                            EV << "LteSchedulerEnb::grant allocating " << uBytes << " prev. booked bytes on band " << (unsigned short)u << endl;

                            // mark here the usage of booked resources
                            bookedUsed+= uBytes;
                            cwAllocatedBytes+=uBytes;

                            unsigned int uBlocks = mac_->getAmc()->computeReqRbs(nodeId,u, cw, uBytes, dir);

                            if (uBlocks <= li->blocks_)
                            {
                                li->blocks_-=uBlocks;
                            }
                            else
                            {
                                li->blocks_=uBlocks=0;
                            }

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
                                cwAllocatedBlocks += uBlocks;
                                totalAllocatedBlocks += uBlocks;
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

                                EV << "LteSchedulerEnb::grant band " << (unsigned short)u << " depleted all its booked resources " << endl;
                            }
                        }
                    }
                    EV << "LteSchedulerEnb::grant band " << (unsigned short)b << " scheduled  " << toServe << " bytes " << " using " << bookedUsed << " resources on other bands and "
                       << (toServe-bookedUsed) << " on its own space which was " << bandAvailableBytes << endl;

                    if (direction_ == UL || direction_ == D2D || direction_ == D2D_MULTI)
                    {
                        // in UL/D2D, serve what it is possible to serve
                        if ((toServe-bookedUsed)>bandAvailableBytes)
                            toServe = bookedUsed + bandAvailableBytes;
                    }

                    if ((toServe-bookedUsed)>bandAvailableBytes)
                    throw cRuntimeError(" Booked resources allocation exited in an abnormal state ");

                    //if (bandAvailableBytes < ((toServe+bookedUsed)))
                    //    throw cRuntimeError (" grant allocation overflow ");

                    // Updating available and allocated data counters
                    bandAvailableBytes -= (toServe-bookedUsed);
                    bandAllocatedBytes += (toServe-bookedUsed);

                    // consume one virtual SDU

                    // check if we are granting less than a full BSR |
                    if ((dir==UL || dir==D2D || dir==D2D_MULTI) && (toServe-MAC_HEADER-RLC_HEADER_UM<vQueueFrontSize))
                    {
                        // update the virtual queue

//                        conn->front().first = (vQueueFrontSize-toServe);

                        // just decrementing the first element is not correct because the queueOccupancy would not be updated
                        // we need to pop the element from the queue and then push it again with a new value
                        unsigned int newSize = conn->front().first - (toServe-MAC_HEADER-RLC_HEADER_UM);
                        PacketInfo info = conn->popFront();

                        // push the element only if the size is > 0
                        if (newSize > 0)
                        {
                            info.first = newSize;
                            conn->pushFront(info);
                        }
                    }
                    else
                    {
                        conn->popFront();
                    }

                    vQueueItemCounter++;

                    EV << "LteSchedulerEnb::grant Consumed: " << vQueueItemCounter << " MAC-SDU (" << vQueueFrontSize << " bytes) [Available: " << bandAvailableBytes << " bytes] [Allocated: " << bandAllocatedBytes << " bytes]" << endl;

                    // if there are no more bands available for UL/D2D scheduling
                    if ((direction_ == UL || direction_ == D2D || direction_ == D2D_MULTI) && b == size-1)
                    {
                        stop = true;
                        break;
                    }

                    if(bytes == 0 && vQueueItemCounter>0)
                    {
                        // Allocate only one SDU
                        EV << "LteSchedulerEnb::grant ONLY ONE VQUEUE ITEM TO ALLOCATE" << endl;
                        stop = true;
                        break;
                    }

                    if (firstSdu)
                    {
                        firstSdu = false;
                    }
                }
            }

            cwAllocatedBytes += bandAllocatedBytes;

            if (bandAllocatedBytes > 0)
            {
                unsigned int blocksAdded = mac_->getAmc()->computeReqRbs(nodeId, b, cw, bandAllocatedBytes, dir);

                if (blocksAdded > bandAvailableBlocks)
                    throw cRuntimeError("band %d GRANT allocation overflow: avail. blocks %d alloc. blocks %d", b,
                        bandAvailableBlocks, blocksAdded);

                // update available blocks
                bandAvailableBlocks -= blocksAdded;

                if (allocatedCws == 0)
                {
                    // Record the allocation only for the first codeword
                    if (allocator_->addBlocks(antenna, b, nodeId, blocksAdded, bandAllocatedBytes))
                    {
                        totalAllocatedBlocks += blocksAdded;
                    }
                    else
                        throw cRuntimeError("band %d ALLOCATOR allocation overflow - allocation failed in allocator",
                            b);
                }
                cwAllocatedBlocks += blocksAdded;

                if ((*bandLim).at(i).limit_.at(cw) > 0)
                {
                    if (limitBl)
                    {
                        EV << "LteSchedulerEnb::grant BandLimit decreasing limit on band " << b << " limit " << (*bandLim).at(i).limit_.at(cw) << " blocks " << blocksAdded << endl;
                        (*bandLim).at(i).limit_.at(cw) -=blocksAdded;
                    }
                    // signal the amount of allocated bytes in the current band
                    else if (!limitBl)
                    {
                        (*bandLim).at(i).limit_.at(cw) -= bandAllocatedBytes;
                    }

                    if ((*bandLim).at(i).limit_.at(cw) <0)
                    throw cRuntimeError("BandLimit Main Band %d decreasing inconsistency : %d",b,(*bandLim).at(i).limit_.at(cw));
                }
            }

            if (stop)
                break;
        }

        EV << "LteSchedulerEnb::grant Codeword allocation: " << cwAllocatedBytes << "bytes" << endl;

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
            scheduleList_[scListId] += ((dir == DL) ? vQueueItemCounter : cwAllocatedBlocks);

            EV << "LteSchedulerEnb::grant CODEWORD IS NOW BUSY: GO TO NEXT CODEWORD." << endl;
            if (allocatedCws_.at(nodeId) == MAX_CODEWORDS)
            {
                eligible = false;
                stop = true;
            }
        }
        else
        {
            EV << "LteSchedulerEnb::grant CODEWORD IS FREE: NO ALLOCATION IS POSSIBLE IN NEXT CODEWORD." << endl;
            eligible = false;
            stop = true;
        }
        if (stop)
            break;
    }

    EV << "LteSchedulerEnb::grant Total allocation: " << totalAllocatedBytes << " bytes, " << totalAllocatedBlocks << " blocks" << endl;
    EV << "LteSchedulerEnb::grant --------------------::[  END GRANT  ]::--------------------" << endl;

    return totalAllocatedBytes;
}

void LteSchedulerEnb::update()
{
    scheduler_->updateSchedulingInfo();
}

void LteSchedulerEnb::backlog(MacCid cid)
{
    EV << "LteSchedulerEnb::backlog - backlogged data for Logical Cid " << cid << endl;
    if(cid == 1)
        return;

    scheduler_->notifyActiveConnection(cid);
}

unsigned int LteSchedulerEnb::readPerUeAllocatedBlocks(const MacNodeId nodeId,
    const Remote antenna, const Band b)
{
    return allocator_->getBlocks(antenna, b, nodeId);
}

unsigned int LteSchedulerEnb::readPerBandAllocatedBlocks(Plane plane, const Remote antenna, const Band band)
{
    return allocator_->getAllocatedBlocks(plane, antenna, band);
}

unsigned int LteSchedulerEnb::getInterferringBlocks(Plane plane, const Remote antenna, const Band band)
{
    return allocator_->getInterferringBlocks(plane, antenna, band);
}

unsigned int LteSchedulerEnb::readAvailableRbs(const MacNodeId id,
    const Remote antenna, const Band b)
{
    return allocator_->availableBlocks(id, antenna, b);
}

unsigned int LteSchedulerEnb::readTotalAvailableRbs()
{
    return allocator_->computeTotalRbs();
}

unsigned int LteSchedulerEnb::readRbOccupation(const MacNodeId id, RbMap& rbMap)
{
    return allocator_->rbOccupation(id, rbMap);
}

/*
 * OFDMA frame management
 */

void LteSchedulerEnb::initAndResetAllocator()
{
    // initialize and reset the allocator
    allocator_->initAndReset(resourceBlocks_,
        mac_->getDeployer()->getNumBands());
}

unsigned int LteSchedulerEnb::availableBytes(const MacNodeId id,
    Remote antenna, Band b, Codeword cw, Direction dir, int limit)
{
    EV << "LteSchedulerEnb::availableBytes MacNodeId " << id << " Antenna " << dasToA(antenna) << " band " << b << " cw " << cw << endl;
    // Retrieving this user available resource blocks
    int blocks = allocator_->availableBlocks(id,antenna,b);
    //Consistency Check
    if (limit>blocks && limit!=-1)
    throw cRuntimeError("LteSchedulerEnb::availableBytes signaled limit inconsistency with available space band b %d, limit %d, available blocks %d",b,limit,blocks);

    if (limit!=-1)
    blocks=(blocks>limit)?limit:blocks;
    unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(id, b, cw, blocks, dir);
    EV << "LteSchedulerEnb::availableBytes MacNodeId " << id << " blocks [" << blocks << "], bytes [" << bytes << "]" << endl;

    return bytes;
}

std::set<Band> LteSchedulerEnb::getOccupiedBands()
{
   return allocator_->getAllocatorOccupiedBands();
}

void LteSchedulerEnb::storeAllocationEnb( std::vector<std::vector<AllocatedRbsPerBandMapA> > allocatedRbsPerBand,std::set<Band>* untouchableBands)
{
    allocator_->storeAllocation(allocatedRbsPerBand, untouchableBands);
}

void LteSchedulerEnb::storeScListId(std::pair<unsigned int, Codeword> scList,unsigned int num_blocks)
{
    scheduleList_[scList]=num_blocks;
}

    /*****************
     * UTILITIES
     *****************/

LteScheduler* LteSchedulerEnb::getScheduler(SchedDiscipline discipline)
{
    EV << "Creating LteScheduler " << schedDisciplineToA(discipline) << endl;

    switch(discipline)
    {
        case DRR:
        return new LteDrr();
        case PF:
        return new LtePf(mac_->par("pfAlpha").doubleValue());
        case MAXCI:
        return new LteMaxCi();
        case MAXCI_MB:
        return new LteMaxCiMultiband();
        case MAXCI_OPT_MB:
        return new LteMaxCiOptMB();
        case MAXCI_COMP:
        return new LteMaxCiComp();
        case ALLOCATOR_BESTFIT:
        return new LteAllocatorBestFit();

        default:
        throw cRuntimeError("LteScheduler not recognized");
        return NULL;
    }
}

void LteSchedulerEnb::resourceBlockStatistics(bool sleep)
{
    if (sleep)
    {
        if (direction_ == DL)
        {
            mac_->emit(cellBlocksUtilizationDl_, 0.0);
            mac_->emit(lteAvgServedBlocksDl_, (long)0);
            mac_->emit(depletedPowerDl_, mac_->getIdleLevel(DL, MBSFN));
        }
        return;
    }
    // Get a reference to the begin and the end of the map which stores the blocks allocated
    // by each UE in each Band. In this case, is requested the pair of iterators which refers
    // to the per-Band (first key) per-Ue (second-key) map
    std::vector<std::vector<unsigned int> >::const_iterator planeIt =
        allocator_->getAllocatedBlocksBegin();
    std::vector<std::vector<unsigned int> >::const_iterator planeItEnd =
        allocator_->getAllocatedBlocksEnd();

    double utilization = 0.0;
    double allocatedBlocks = 0;
    unsigned int plane = 0;
    unsigned int antenna = 0;

    double depletedPower = 0;

    // TODO CHECK IDLE / ACTIVE STATUS
    //if ( ... ) {
    // For Each layer (normal/MU-MIMO)
    //    for (; planeIt != planeItEnd; ++planeIt) {
    std::vector<unsigned int>::const_iterator antennaIt = planeIt->begin();
    std::vector<unsigned int>::const_iterator antennaItEnd = planeIt->end();

    // For each antenna (MACRO/RUs)
    for (; antennaIt != antennaItEnd; ++antennaIt)
    {
        if (plane == MAIN_PLANE && antenna == MACRO)
            if (direction_ == DL)
                mac_->allocatedRB(*antennaIt);
        // collect the antenna utilization for current Layer
        utilization += (double) (*antennaIt);

        allocatedBlocks += (double) (*antennaIt);
        if (direction_ == DL)
        {
            depletedPower += ((mac_->getZeroLevel(direction_,
                mac_->getCurrentSubFrameType()))
                + ((double) (*antennaIt) * mac_->getPowerUnit(direction_,
                    mac_->getCurrentSubFrameType())));
        }
        EV << "LteSchedulerEnb::resourceBlockStatistics collecting utilization for plane" <<
        plane << "antenna" << dasToA((Remote)antenna) << " allocated blocks "
           << allocatedBlocks << " depletedPower " << depletedPower << endl;
        antenna++;
    }
    plane++;
    //    }
    // antenna here is the number of antennas used; the same applies for plane;
    // Compute average OFDMA utilization between layers and antennas
    utilization /= (((double) (antenna)) * ((double) resourceBlocks_));
    if (direction_ == DL)
    {
        mac_->emit(cellBlocksUtilizationDl_, utilization);
        mac_->emit(lteAvgServedBlocksDl_, allocatedBlocks);
        mac_->emit(depletedPowerDl_, depletedPower);
    }
    else if (direction_ == UL)
    {
        mac_->emit(cellBlocksUtilizationUl_, utilization);
        mac_->emit(lteAvgServedBlocksUl_, allocatedBlocks);
        mac_->emit(depletedPowerUl_, depletedPower);
    }
    else
    {
        throw cRuntimeError("LteSchedulerEnb::resourceBlockStatistics(): Unrecognized direction %d", direction_);
    }
}
ActiveSet LteSchedulerEnb::readActiveConnections()
{
    ActiveSet active = scheduler_->readActiveSet();
    ActiveSet::iterator it = active.begin();
    ActiveSet::iterator et = active.end();
    MacCid cid;
    for (; it != et; ++it)
    {
        cid = *it;
    }
    return scheduler_->readActiveSet();
}

void LteSchedulerEnb::removeActiveConnections(MacNodeId nodeId)
{
    ActiveSet active = scheduler_->readActiveSet();
    ActiveSet::iterator it = active.begin();
    ActiveSet::iterator et = active.end();
    MacCid cid;
    for (; it != et; ++it)
    {
        cid = *it;
        if (MacCidToNodeId(cid) == nodeId)
        {
            scheduler_->removeActiveConnection(cid);
        }
    }
}
