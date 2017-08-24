//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTESCHEDULERENB_H_
#define _LTE_LTESCHEDULERENB_H_

#include "common/LteCommon.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/allocator/LteAllocatorUtils.h"

/// forward declarations
class LteScheduler;
class LteAllocationModule;
class LteMacEnb;

/**
 * @class LteSchedulerEnb
 *
 */
class LteSchedulerEnb
{
    /******************
     * Friend classes
     ******************/

    // Lte Scheduler Modules access grants
    friend class LteScheduler;
    friend class LteDrr;
    friend class LtePf;
    friend class LteMaxCi;
    friend class LteMaxCiMultiband;
    friend class LteMaxCiOptMB;
    friend class LteMaxCiComp;
    friend class LteAllocatorBestFit;

  protected:

    /*******************
     * Data structures
     *******************/

    // booked requests list : Band, bytes, blocks
    struct Request
    {
        Band b_;
        unsigned int bytes_;
        unsigned int blocks_;

        Request(Band b, unsigned int bytes, unsigned int blocks)
        {
            b_ = b;
            bytes_ = bytes;
            blocks_ = blocks;
        }

        Request()
        {
            Request(0, 0, 0);
        }
    };

    // Owner MAC module (can be LteMacEnb on eNB or LteMacRelayEnb on Relays). Set via initialize().
    LteMacEnb *mac_;

    // Reference to the LTE Binder
    LteBinder *binder_;

    // System allocator, carries out the block-allocation functions.
    LteAllocationModule *allocator_;

    // Scheduling agent.
    LteScheduler *scheduler_;

    // Operational Direction. Set via initialize().
    Direction direction_;

    // Schedule list
    LteMacScheduleList scheduleList_;

    // Codeword list
    LteMacAllocatedCws allocatedCws_;

    // Pointer to downlink virtual buffers (that are in LteMacBase)
    LteMacBufferMap* vbuf_;

    // Pointer to uplink virtual buffers (that are in LteMacBase)
    LteMacBufferMap* bsrbuf_;

    // Pointer to Harq Tx Buffers (that are in LteMacBase)
    HarqTxBuffers* harqTxBuffers_;

    // Pointer to Harq Rx Buffers (that are in LteMacBase)
    HarqRxBuffers* harqRxBuffers_;

    /// Total available resource blocks (switch on direction)
    /// Initialized by LteMacEnb::handleSelfMessage() using resourceBlocks()
    unsigned int resourceBlocks_;

    /// Statistics
    simsignal_t cellBlocksUtilizationDl_;
    simsignal_t cellBlocksUtilizationUl_;
    simsignal_t lteAvgServedBlocksDl_;
    simsignal_t lteAvgServedBlocksUl_;
    simsignal_t depletedPowerDl_;
    simsignal_t depletedPowerUl_;
    simsignal_t prf_0, prf_1a, prf_2a, prf_3a, prf_1b, prf_2b, prf_3b,
        prf_1c, prf_2c, prf_3c, prf_4, prf_5, prf_6a, prf_7a, prf_8a, prf_6b, prf_7b,
        prf_8b, prf_6c, prf_7c, prf_8c, prf_9;
    simsignal_t rb_0, rb_1a, rb_2a, rb_3a, rb_1b, rb_2b, rb_3b,
        rb_1c, rb_2c, rb_3c, rb_4, rb_5, rb_6a, rb_7a, rb_8a, rb_6b, rb_7b,
        rb_8b, rb_6c, rb_7c, rb_8c, rb_9;

  public:

    /**
     * Default constructor.
     */
    LteSchedulerEnb();

    /**
     * Destructor.
     */
    virtual ~LteSchedulerEnb();

    /**
     * Set Direction and bind the internal pointers to the MAC objects.
     * @param dir link direction
     * @param mac pointer to MAC module
     */
    void initialize(Direction dir, LteMacEnb* mac);

    /**
     * Schedule data.
     * @param list
     */
    virtual LteMacScheduleList* schedule();

    /**
     * Update the status of the scheduler. Called by the MAC.
     * The function calls the LteScheduler update().
     */
    void update();

    /**
     * Adds an entry (if not already in) to scheduling list.
     * The function calls the LteScheduler notify().
     * @param cid connection identifier
     */
    void backlog(MacCid cid);

    /**
     * Get/Set current available Resource Blocks.
     */
    unsigned int& resourceBlocks()
    {
        return resourceBlocks_;
    }

    /**
     * Gets the amount of the system resource block
     */
    unsigned int getResourceBlocks()
    {
        return resourceBlocks_;
    }

    /**
     * Returns the number of blocks allocated by the UE on the given antenna
     * into the logical band provided.
     */
    unsigned int readPerUeAllocatedBlocks(const MacNodeId nodeId, const Remote antenna, const Band b);

    /*
     * Returns the amount of blocks allocated on a logical band
     */
    unsigned int readPerBandAllocatedBlocks(Plane plane, const Remote antenna, const Band band);

    /*
     * Returns the amount of blocks allocated on a logical band
     */
    unsigned int getInterferringBlocks(Plane plane, const Remote antenna, const Band band);

    /**
     * Returns the number of available blocks for the UE on the given antenna
     * in the logical band provided.
     */
    unsigned int readAvailableRbs(const MacNodeId id, const Remote antenna, const Band b);

    /**
     * Returns the number of available blocks.
     */
    unsigned int readTotalAvailableRbs();

    /**
     * Resource Block IDs computation function.
     */
    unsigned int readRbOccupation(const MacNodeId id, RbMap& rbMap);

    /**
     * Schedules retransmission for the Harq Process of the given UE on a set of logical bands.
     * Each band has also assigned a band limit amount of bytes: no more than the specified
     * amount will be served on the given band for the acid.
     *
     * @param nodeId The node ID
     * @param cw The codeword used to serve the acid process
     * @param bands A vector of logical bands
     * @param acid The ACID
     * @return The allocated bytes. 0 if retransmission was not possible
     */
    virtual unsigned int schedulePerAcidRtx(MacNodeId nodeId, Codeword cw, unsigned char acid,
        std::vector<BandLimit>* bandLim = NULL, Remote antenna = MACRO, bool limibBl = false) = 0;

    /**
     * Schedules capacity for a given connection without effectively perform the operation on the
     * real downlink/uplink buffer: instead, it performs the operation on a virtual buffer,
     * which is used during the finalize() operation in order to commit the decision performed
     * by the grant function.
     * Each band has also assigned a band limit amount of bytes: no more than the
     * specified amount will be served on the given band for the cid
     *
     * @param cid Identifier of the connection that is granted capacity
     * @param antenna the DAS remote antenna
     * @param bands The set of logical bands with their related limits
     * @param bytes Grant size, in bytes
     * @param terminate Set to true if scheduling has to stop now
     * @param active Set to false if the current queue becomes inactive
     * @param eligible Set to false if the current queue becomes ineligible
     * @param limitBl if true bandLim vector express the limit of allocation for each band in block
     * @return The number of bytes that have been actually granted.
     */
    virtual unsigned int scheduleGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible,
        std::vector<BandLimit>* bandLim = NULL, Remote antenna = MACRO, bool limitBl = false);

    /*
     * Getter for active connection set
     */
    ActiveSet readActiveConnections();

    void removeActiveConnections(MacNodeId nodeId);

  protected:

    /**
     * Checks Harq Descriptors and return the first free codeword.
     *
     * @param id
     * @param cw
     * @return
     */
    virtual bool checkEligibility(MacNodeId id, Codeword& cw) =0;

    /*
     * Schedule and related methods.
     *
     * The methods in the following section must be implemented in the UL and DL class of the
     * LteSchedulerEnb: these are common functions used inside the schedule method, which is
     * the main function of the class
     */

    /**
     * Updates current schedule list with HARQ retransmissions.
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxschedule() = 0;

    /*
     * OFDMA frame management
     */

    /**
     * Records assigned resource blocks statistics.
     */
    void resourceBlockStatistics(bool sleep = false);

    /**
     * Reset And Init the blocks-related structures allocation
     */
    void initAndResetAllocator();

    /**
     * Returns the available space for a given user, antenna, logical band and codeword, in bytes.
     *
     * @param id MAC node Id
     * @param antenna antenna
     * @param b band
     * @param cw codeword
     * @return available space in bytes
     */
    unsigned int availableBytes(const MacNodeId id, const Remote antenna, Band b, Codeword cw, Direction dir, int limit = -1);

    unsigned int allocatedCws(MacNodeId nodeId)
    {
        return allocatedCws_[nodeId];
    }

    // Get the bands already allocated
    std::set<Band> getOccupiedBands();

    void storeAllocationEnb(std::vector<std::vector<AllocatedRbsPerBandMapA> > allocatedRbsPerBand, std::set<Band>* untouchableBands = NULL);

    // store an element in the schedule list
    void storeScListId(std::pair<unsigned int, Codeword> scList,unsigned int num_blocks);

  private:

    /*****************
     * UTILITIES
     *****************/

    /**
     * Returns a particular LteScheduler subclass,
     * implementing the given discipline.
     * @param discipline scheduler discipline
     */
    LteScheduler* getScheduler(SchedDiscipline discipline);
};

#endif // _LTE_LTESCHEDULERENB_H_
