//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEPF_H_
#define _LTE_LTEPF_H_

#include "stack/mac/scheduler/LteScheduler.h"

class LtePf : public LteScheduler
{
  protected:

    typedef std::map<MacCid, double> PfRate;
    typedef SortedDesc<MacCid, double> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

    //! Long-term rates, used by PF scheduling.
    PfRate pfRate_;

    //! Granted bytes
    std::map<MacCid, unsigned int> grantedBytes_;

    //! Smoothing factor for proportional fair scheduler.
    double pfAlpha_;

    //! Small number to slightly blur away scores.
    const double scoreEpsilon_;

  public:

    double & pfAlpha()
    {
        return pfAlpha_;
    }

    // Scheduling functions ********************************************************************

    //virtual void schedule ();

    virtual void prepareSchedule();

    virtual void commitSchedule();

    // *****************************************************************************************

    void notifyActiveConnection(MacCid cid);

    void removeActiveConnection(MacCid cid);

    void updateSchedulingInfo();

    LtePf(double pfAlpha) :
        scoreEpsilon_(0.000001)
    {
        pfAlpha_ = pfAlpha;
        pfRate_.clear();
    }
};

#endif // _LTE_LTEPF_H_
