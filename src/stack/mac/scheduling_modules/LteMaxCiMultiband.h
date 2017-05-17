/*
 * LteMaxCiMultiband.h
 *
 *  Created on: Apr 16, 2014
 *      Author: antonio
 */

#ifndef LTEMAXCIMULTIBAND_H_
#define LTEMAXCIMULTIBAND_H_

#include "stack/mac/scheduler/LteScheduler.h"

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

class LteMaxCiMultiband : public virtual LteScheduler
{


public:
    LteMaxCiMultiband(){ }
    virtual ~LteMaxCiMultiband() {};

    virtual void prepareSchedule();

    virtual void commitSchedule();



    // *****************************************************************************************


    void notifyActiveConnection(MacCid cid);

    void removeActiveConnection(MacCid cid);

    void updateSchedulingInfo();
};


#endif /* LTEMAXCIMULTIBAND_H_ */
