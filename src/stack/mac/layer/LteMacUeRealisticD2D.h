//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEMACUEREALISTICD2D_H_
#define _LTE_LTEMACUEREALISTICD2D_H_

#include "stack/mac/layer/LteMacUeRealistic.h"
#include "stack/mac/layer/LteMacEnbRealisticD2D.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferTxD2D.h"

class LteSchedulingGrant;
class LteSchedulerUeUl;
class LteBinder;

class LteMacUeRealisticD2D : public LteMacUeRealistic
{
    // reference to the eNB
    LteMacEnbRealisticD2D* enb_;

    // RAC Handling variables
    bool racD2DMulticastRequested_;
    // Multicast D2D BSR handling
    bool bsrD2DMulticastTriggered_;

    // if true, use the preconfigured TX params for transmission, else use that signaled by the eNB
    bool usePreconfiguredTxParams_;
    UserTxParams* preconfiguredTxParams_;
    UserTxParams* getPreconfiguredTxParams();  // build and return new user tx params

  protected:

    /**
     * Reads MAC parameters for ue and performs initialization.
     */
    virtual void initialize(int stage);

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(cMessage *msg);

    /**
     * Main loop
     */
    virtual void handleSelfMessage();

    virtual void macHandleGrant(cPacket* pkt);

    /*
     * Checks RAC status
     */
    virtual void checkRAC();

    /*
     * Receives and handles RAC responses
     */
    virtual void macHandleRac(cPacket* pkt);

    void macHandleD2DModeSwitch(cPacket* pkt);

    virtual LteMacPdu* makeBsr(int size);

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    virtual void macPduMake();

  public:
    LteMacUeRealisticD2D();
    virtual ~LteMacUeRealisticD2D();

    virtual bool isD2DCapable()
    {
        return true;
    }

    virtual void triggerBsr(MacCid cid)
    {
        if (connDesc_[cid].getDirection() == D2D_MULTI)
            bsrD2DMulticastTriggered_ = true;
        else
            bsrTriggered_ = true;
    }
    virtual void doHandover(MacNodeId targetEnb);
};

#endif
