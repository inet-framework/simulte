//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/d2dModeSelection/d2dModeSelectionBestCqi/D2DModeSelectionBestCqi.h"

Define_Module(D2DModeSelectionBestCqi);

void D2DModeSelectionBestCqi::initialize(int stage)
{
    D2DModeSelectionBase::initialize(stage);
}

void D2DModeSelectionBestCqi::doModeSelection()
{
    EV << NOW << " D2DModeSelectionBestCqi::doModeSelection - Running Mode Selection algorithm..." << endl;

    switchList_.clear();
    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> >::iterator it = peeringModeMap_->begin();
    for (; it != peeringModeMap_->end(); ++it)
    {
        MacNodeId srcId = it->first;

        // consider only UEs within this cell
        if (binder_->getNextHop(srcId) != mac_->getMacCellId())
            continue;

        std::map<MacNodeId, LteD2DMode>::iterator jt = it->second.begin();
        for (; jt != it->second.end(); ++jt)
        {
            MacNodeId dstId = jt->first;   // since the D2D CQI is the same for all D2D connections,
                                            // the mode will be the same for all destinations

            // consider only UEs within this cell
            if (binder_->getNextHop(dstId) != mac_->getMacCellId())
                continue;

            // skip UEs that are performing handover
            if (binder_->hasUeHandoverTriggered(dstId) || binder_->hasUeHandoverTriggered(srcId))
                continue;

            LteD2DMode oldMode = jt->second;

            // Compute the achievable bits on a single RB for UL direction
            // Note that this operation takes into account the CQI returned by the AMC Pilot (by default, it
            // is the minimum CQI over all RBs)
            unsigned int bitsUl = mac_->getAmc()->computeBitsOnNRbs(srcId, 0, 0, 1, UL);
            unsigned int bitsD2D = mac_->getAmc()->computeBitsOnNRbs(srcId, 0, 0, 1, D2D);

            EV << NOW << " D2DModeSelectionBestCqi::doModeSelection - bitsUl[" << bitsUl << "] bitsD2D[" << bitsD2D << "]" << endl;

            // compare the bits in the two modes and select the best one
            LteD2DMode newMode = (bitsUl > bitsD2D) ? IM : DM;

            if (newMode != oldMode)
            {
                // add this flow to the list of flows to be switched
                FlowId p(srcId, dstId);
                FlowModeInfo info;
                info.flow = p;
                info.oldMode = oldMode;
                info.newMode = newMode;
                switchList_.push_back(info);

                // update peering map
                jt->second = newMode;

                EV << NOW << " D2DModeSelectionBestCqi::doModeSelection - Flow: " << srcId << " --> " << dstId << " [" << d2dModeToA(newMode) << "]" << endl;
            }
        }
    }
}

void D2DModeSelectionBestCqi::doModeSwitchAtHandover(MacNodeId nodeId, bool handoverCompleted)
{
    // with this MS algorithm, connections of nodeId will return to DM after handover only
    // if the algorithm triggers the switch at the next period. Thus, it is not necessary to
    // force the switch here.
    if (handoverCompleted)
        return;

    D2DModeSelectionBase::doModeSwitchAtHandover(nodeId, handoverCompleted);
}
