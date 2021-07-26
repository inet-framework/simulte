//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LCGSCHEDULER_H_
#define _LTE_LCGSCHEDULER_H_

#include "common/LteCommon.h"
#include "stack/mac/layer/LteMacUe.h"

/// forward declarations
class LteSchedulerUeUl;
class LteMacPdu;
/**
 * @class LcgScheduler
 */
typedef std::map<MacCid, unsigned int> ScheduleList;

class SIMULTE_API LcgScheduler
{

  protected:
    /**
     * Score-based schedulers descriptor.
     */
    template<typename T, typename S>
    struct SortedDesc
    {
        /// Connection identifier.
        T x_;
        /// Score value.
        S score_;

        /// Comparison operator to enable sorting.
        bool operator<(const SortedDesc& y) const
            {
            return score_ < y.score_;
        }

    public:
        SortedDesc(const T x, const S score)
        {
            x_ = x;
            score_ = score;
        }
    };

    struct StatusElem
    {
        unsigned int occupancy_;
        unsigned int bucket_;
        unsigned int sentData_;
        unsigned int sentSdus_;
    };

    // last execution time
    omnetpp::simtime_t lastExecutionTime_;

    /// MAC module, used to get parameters from NED
    LteMacUe *mac_;

    /// Associated LteSchedulerUeUl (it is the one who creates the LteScheduler)
    LteSchedulerUeUl* ueScheduler_;

    // schedule List - returned by reference on scheduler invocation
    ScheduleList scheduleList_;

    // scheduled bytes list
    ScheduleList scheduledBytesList_;

    MacCid firstSduCid;

    /// Cid List
    typedef std::list<MacCid> CidList;

    // scheduling status map
    std::map<MacCid, StatusElem> statusMap_;

  public:

    /**
     * Default constructor.
     */
    LcgScheduler(LteMacUe * mac);

    /**
     * Destructor.
     */
    virtual ~LcgScheduler();

    /**
     * Initializes the LteScheduler.
     * @param ueScheduler UE scheduler
     */
    inline virtual void setUeUlScheduler(LteSchedulerUeUl* ueScheduler)
    {
        ueScheduler_ = ueScheduler;
    }

    /* Executes the LCG scheduling algorithm
     * @param availableBytes
     * @return # of scheduled sdus per cid
     */
    virtual ScheduleList& schedule(unsigned int availableBytes, Direction grantDir = UL);

    /* After the scheduling, returns the amount of bytes
     * scheduled for each connection
     */
    virtual ScheduleList& getScheduledBytesList();

    inline virtual bool isFirstSdu(const MacCid& cid){
        return firstSduCid == cid;
    }

    // *****************************************************************************************

//        /// performs request of grant to the eNbScheduler
//        virtual unsigned int grant(MacCid cid,unsigned int bytes, bool& terminate,bool& active,bool& eligible);
//
//        /// calls eNbScheduler rtxschedule()
//        virtual bool rtxschedule();
//
//        virtual void notify( MacCid activeCid ) {;}
//
//        virtual void remove( MacCid cid ) {;}
//
//        virtual void update() {;}
};
#endif

