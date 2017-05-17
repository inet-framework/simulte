//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


#ifndef _LTE_LTESCHEDULERENBDLREALISTIC_H_
#define _LTE_LTESCHEDULERENBDLREALISTIC_H_

#include "stack/mac/scheduler/LteSchedulerEnbDl.h"
#include "common/LteCommon.h"
#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/amc/UserTxParams.h"

class LteSchedulerEnbDlRealistic : public LteSchedulerEnbDl
{

public:
    LteSchedulerEnbDlRealistic();
    virtual ~LteSchedulerEnbDlRealistic();

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
};

#endif /* _LTE_LTESCHEDULERENBDLREALISTIC_H_ */
