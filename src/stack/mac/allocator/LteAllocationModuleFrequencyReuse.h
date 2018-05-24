//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEALLOCATIONMODULEFREQUENCYREUSE_H_
#define _LTE_LTEALLOCATIONMODULEFREQUENCYREUSE_H_

#include "common/LteCommon.h"
#include "stack/mac/allocator/LteAllocationModule.h"

class LteAllocationModuleFrequencyReuse : public LteAllocationModule
{
    public:
    /// Default constructor.
    LteAllocationModuleFrequencyReuse(LteMacEnb *mac, const Direction direction);
    // Store the Allocation based on passed paremeter
    virtual void storeAllocation( std::vector<std::vector<AllocatedRbsPerBandMapA> > allocatedRbsPerBand,std::set<Band>* untouchableBands = NULL);
    // Get the bands already allocated by RAC and RTX ( Debug purpose)
    virtual std::set<Band> getAllocatorOccupiedBands();
};

#endif
