//
//                           SimuLTE
// Copyright (C) 2015 Giovanni Nardini
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LCGSCHEDULEREXPERIMENTAL_H_
#define _LTE_LCGSCHEDULEREXPERIMENTAL_H_

#include "LcgScheduler.h"

/**
 * @class LcgSchedulerExperimental
 */
class LcgSchedulerExperimental : public LcgScheduler
{
  public:

    /**
     * Default constructor.
     */
    LcgSchedulerExperimental(LteMacUe * mac);

    /**
     * Destructor.
     */
    virtual ~LcgSchedulerExperimental();

    /* Executes the LCG scheduling algorithm
     * @param availableBytes
     * @return # of scheduled sdus per cid
     */
    virtual ScheduleList& schedule(unsigned int availableBytes);

};
#endif

