//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"

/**
 * TODO:
 * - rimuovere i commenti dalle funzioni quando saranno implementate nel enb scheduler
 */

void LteScheduler::setEnbScheduler(LteSchedulerEnb* eNbScheduler)
{
    eNbScheduler_ = eNbScheduler;
    direction_ = eNbScheduler_->direction_;
    mac_ = eNbScheduler_->mac_;
    initializeGrants();
}


unsigned int LteScheduler::requestGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible , std::vector<BandLimit>* bandLim)
{
    return eNbScheduler_->scheduleGrant(cid, bytes, terminate, active, eligible ,bandLim);
}

bool LteScheduler::scheduleRetransmissions()
{
    return eNbScheduler_->rtxschedule();
}

void LteScheduler::scheduleRacRequests()
{
    //return (dynamic_cast<LteSchedulerEnbUl*>(eNbScheduler_))->serveRacs();
}

void LteScheduler::requestRacGrant(MacNodeId nodeId)
{
    //return (dynamic_cast<LteSchedulerEnbUl*>(eNbScheduler_))->racGrantEnb(nodeId);
}

void LteScheduler::schedule()
{
    prepareSchedule();
    commitSchedule();
}

void LteScheduler::initializeGrants()
{
    if (direction_ == DL)
    {
        grantTypeMap_[CONVERSATIONAL] = aToGrantType(mac_->par("grantTypeConversationalDl"));
        grantTypeMap_[STREAMING] = aToGrantType(mac_->par("grantTypeStreamingDl"));
        grantTypeMap_[INTERACTIVE] = aToGrantType(mac_->par("grantTypeInteractiveDl"));
        grantTypeMap_[BACKGROUND] = aToGrantType(mac_->par("grantTypeBackgroundDl"));

        grantSizeMap_[CONVERSATIONAL] = mac_->par("grantSizeConversationalDl");
        grantSizeMap_[STREAMING] = mac_->par("grantSizeStreamingDl");
        grantSizeMap_[INTERACTIVE] = mac_->par("grantSizeInteractiveDl");
        grantSizeMap_[BACKGROUND] = mac_->par("grantSizeBackgroundDl");
    }
    else if (direction_ == UL)
    {
        grantTypeMap_[CONVERSATIONAL] = aToGrantType(mac_->par("grantTypeConversationalUl"));
        grantTypeMap_[STREAMING] = aToGrantType(mac_->par("grantTypeStreamingUl"));
        grantTypeMap_[INTERACTIVE] = aToGrantType(mac_->par("grantTypeInteractiveUl"));
        grantTypeMap_[BACKGROUND] = aToGrantType(mac_->par("grantTypeBackgroundUl"));

        grantSizeMap_[CONVERSATIONAL] = mac_->par("grantSizeConversationalUl");
        grantSizeMap_[STREAMING] = mac_->par("grantSizeStreamingUl");
        grantSizeMap_[INTERACTIVE] = mac_->par("grantSizeInteractiveUl");
        grantSizeMap_[BACKGROUND] = mac_->par("grantSizeBackgroundUl");
    }
    else
    {
        throw cRuntimeError("Unknown direction %d", direction_);
    }
}
