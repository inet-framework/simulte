//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEMAXCICOMP_H_
#define _LTE_LTEMAXCICOMP_H_

#include "stack/mac/scheduler/LteScheduler.h"
#include "common/LteCommon.h"

class LteMaxCiComp : public virtual LteScheduler
{
  protected:

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

    // get the bandLimit for the given user
    void getBandLimit(std::vector<BandLimit>* bandLimit, MacNodeId ueId);

  public:

    virtual void prepareSchedule();

    virtual void commitSchedule();

    // *****************************************************************************************

    void notifyActiveConnection(MacCid cid);

    void removeActiveConnection(MacCid cid);

    void updateSchedulingInfo();
};

#endif // _LTE_LTEMAXCICOMP_H_
