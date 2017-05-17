//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEMACUEREALISTIC_H_
#define _LTE_LTEMACUEREALISTIC_H_

#include "stack/mac/layer/LteMacUe.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"

class LteSchedulingGrant;
class LteSchedulerUeUl;
class LteBinder;

class LteMacUeRealistic : public LteMacUe
{
  protected:

    /// List of scheduled connection for this UE
    LteMacScheduleList* scheduleList_;

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
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    virtual bool macSduRequest();

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

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     */
    virtual bool bufferizePacket(cPacket* pkt);

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer
     */
    virtual void handleUpperMessage(cPacket* pkt);

    /**
     * Main loop
     */
    virtual void handleSelfMessage();

    /**
     * Flush Tx H-ARQ buffers for the user
     */
    virtual void flushHarqBuffers();

  public:
    LteMacUeRealistic();
    virtual ~LteMacUeRealistic();
};

#endif
