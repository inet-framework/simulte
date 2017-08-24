//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEALLOCATORBESTFIT_H_
#define _LTE_LTEALLOCATORBESTFIT_H_

#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/allocator/LteAllocatorUtils.h"
#include "stack/mac/allocator/LteAllocationModule.h"

struct Candidate {
    Band index;
    unsigned int len;
    bool greater;
};


class LteAllocatorBestFit : public virtual LteScheduler
{
  protected:

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

    /**
     * e.g. allocatedRbsBand_ [ <plane> ] [ <antenna> ] [ <band> ] give the the amount of blocks allocated for each UE
     */
    std::vector<std::vector<AllocatedRbsPerBandMapA> > allocatedRbsPerBand_;

    // For each UE, stores the amount of blocks allocated for each band
    AllocatedRbsPerUeMapA allocatedRbsUe_;

    /**
     * Amount of Blocks allocated during this TTI, for each plane and for each antenna
     *
     * e.g. allocatedRbsMatrix_[ <plane> ] [ <antenna> ]
     */
    std::vector<std::vector<unsigned int> > allocatedRbsMatrix_;

    // Map that specify ,for each NodeId,wich bands are free and wich are not
    std::map<MacNodeId,std::map<Band,bool> > perUEbandStatusMap_;

    typedef std::pair<AllocationUeType,std::set<MacNodeId> > AllocationType_Set;
    // Map that specify which bands can(non exclusive bands-D2D) or cannot(exlcusive bands-CELL) be shared
    std::map<Band,AllocationType_Set> bandStatusMap_;

    /**
     * Parameter that specify if the Allocator puts D2D and Infrastructure UEs on dedicated resources
     */
    bool dedicated_;
    /**
     * Enumerator specified for the return of mutualExclusiveAllocation() function.
     * @see mutualExclusiveAllocation()
     */

    // returns the next "hole" in the subframe where the UEs can be eventually allocated
    void checkHole(Candidate& candidate, Band holeIndex, unsigned int holeLen, unsigned int req);


  public:

    LteAllocatorBestFit();

    virtual void prepareSchedule();

    virtual void commitSchedule();

    // *****************************************************************************************

    void notifyActiveConnection(MacCid cid);

    void removeActiveConnection(MacCid cid);

    void updateSchedulingInfo();

    // Initialize the allocation structures of the Scheduler
    void initAndReset();

    // Set the specified bands to exclusive
    void setAllocationType(std::vector<Band> bandVect, AllocationUeType type,MacNodeId nodeId);
};

#endif // _LTE_LTEALLOCATORBESTFIT_H_
