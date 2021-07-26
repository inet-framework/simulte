//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/scheduler/LteSchedulerUeUl.h"
#include "stack/mac/layer/LteMacUe.h"
#include "stack/mac/amc/UserTxParams.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "stack/mac/scheduler/LcgScheduler.h"

using namespace omnetpp;
LteSchedulerUeUl::LteSchedulerUeUl(LteMacUe * mac)
{
    mac_ = mac;
    lcgScheduler_ = new LcgScheduler(mac);
}

LteSchedulerUeUl::~LteSchedulerUeUl()
{
    delete lcgScheduler_;
}

LteMacScheduleList*
LteSchedulerUeUl::schedule()
{
    // 1) Environment Setup

    // clean up old scheduling decisions
    scheduleList_.clear();

    // get the grant
    const LteSchedulingGrant* grant = mac_->getSchedulingGrant();
    Direction dir = grant->getDirection();

    // get the nodeId of the mac owner node
    MacNodeId nodeId = mac_->getMacNodeId();

    EV << NOW << " LteSchedulerUeUl::schedule - Scheduling node " << nodeId << endl;

    //! MCW support in UL
    unsigned int codewords = grant->getCodewords();

    // TODO get the amount of granted data per codeword
    unsigned int availableBlocks = grant->getTotalGrantedBlocks();

    // TODO check if HARQ ACK messages should be subtracted from available bytes

    for (Codeword cw = 0; cw < codewords; ++cw)
    {
        unsigned int availableBytes = grant->getGrantedCwBytes(cw);

        EV << NOW << " LteSchedulerUeUl::schedule - Node " << mac_->getMacNodeId() << " available data from grant are "
           << " blocks " << availableBlocks << " [" << availableBytes << " - Bytes]  on codeword " << cw << endl;

        // per codeword LCP scheduler invocation

        // invoke the schedule() method of the attached LCP scheduler in order to schedule
        // the connections provided
        std::map<MacCid, unsigned int>& sdus = lcgScheduler_->schedule(availableBytes, dir);

        // get the amount of bytes scheduled for each connection
        std::map<MacCid, unsigned int>& bytes = lcgScheduler_->getScheduledBytesList();

        // TODO check if this jump is ok
        if (sdus.empty())
            continue;

        std::map<MacCid, unsigned int>::const_iterator it = sdus.begin(), et = sdus.end();
        for (; it != et; ++it)
        {
            // set schedule list entry
            std::pair<MacCid, Codeword> schedulePair(it->first, cw);
            scheduleList_[schedulePair] = it->second;
        }

        std::map<MacCid, unsigned int>::const_iterator bit = bytes.begin(), bet = bytes.end();
        for (; bit != bet; ++bit)
        {
            // set schedule list entry
            std::pair<MacCid, Codeword> schedulePair(bit->first, cw);
            scheduledBytesList_[schedulePair] = bit->second;
        }

        MacCid highestBackloggedFlow = 0;
        MacCid highestBackloggedPriority = 0;
        MacCid lowestBackloggedFlow = 0;
        MacCid lowestBackloggedPriority = 0;
        bool backlog = false;

        // get the highest backlogged flow id and priority
        backlog = mac_->getHighestBackloggedFlow(highestBackloggedFlow, highestBackloggedPriority);

        if (backlog) // at least one backlogged flow exists
        {
            // get the lowest backlogged flow id and priority
            mac_->getLowestBackloggedFlow(lowestBackloggedFlow, lowestBackloggedPriority);
        }

        // TODO make use of above values
    }
    return &scheduleList_;
}

LteMacScheduleList* LteSchedulerUeUl::getScheduledBytesList()
{
    return &scheduledBytesList_;
}

bool LteSchedulerUeUl::isFirstSdu(const MacCid& cid){
    return lcgScheduler_->isFirstSdu(cid);
}
