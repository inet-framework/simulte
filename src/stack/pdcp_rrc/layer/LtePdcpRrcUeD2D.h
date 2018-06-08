//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEPDCPRRCUED2D_H_
#define _LTE_LTEPDCPRRCUED2D_H_

#include <omnetpp.h>
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"

/**
 * @class LtePdcp
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of LTE Stack (with D2D support).
 *
 */
class LtePdcpRrcUeD2D : public LtePdcpRrcUe
{
    // initialization flag for each D2D peer
    // it is set to true when the first IP datagram for that peer reaches the PDCP layer
    std::map<L3Address, bool> d2dPeeringInit_;

  protected:

    virtual void handleMessage(cMessage *msg);

    void handleControlInfo(cPacket* upPkt, FlowControlInfo* lteInfo)
    {
        delete lteInfo;
    }

    virtual MacNodeId getDestId(FlowControlInfo* lteInfo);

    Direction getDirection(MacNodeId destId)
    {
        if (binder_->getD2DCapability(nodeId_, destId) && binder_->getD2DMode(nodeId_, destId) == DM)
            return D2D;
        return UL;
    }

    /**
     * handler for data port
     * @param pkt incoming packet
     */
    virtual void fromDataPort(cPacket *pkt);

    // handler for mode switch signal
    void pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode);

  public:

};

#endif
