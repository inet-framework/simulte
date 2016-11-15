//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteSchedulerUeUl.h"
#include "LteMacUe.h"
#include "UserTxParams.h"
#include "LteSchedulingGrant.h"
#include "LteMacPdu.h"
#include "LcgScheduler.h"
#include "LcgSchedulerRealistic.h"

LteSchedulerUeUl::LteSchedulerUeUl(LteMacUe * mac)
{
    mac_ = mac;

    // check if MAC is "experimental"
    std::string macType = mac_->getParentModule()->par("LteMacType").stdstringValue();
    if (macType.compare("LteMacUeRealistic") == 0 || macType.compare("LteMacUeRealisticD2D") == 0)
        lcgScheduler_ = new LcgSchedulerRealistic(mac);
    else
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

    // retrieve Transmission parameters
//        const UserTxParams* txPar = grant->getUserTxParams();

//! MCW support in UL
    unsigned int codewords = grant->getCodewords();

    // TODO get the amount of granted data per codeword
    //unsigned int availableBytes = grant->getGrantedBytes();

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
