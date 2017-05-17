//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEMACENBREALISTIC_H_
#define _LTE_LTEMACENBREALISTIC_H_

#include "stack/mac/layer/LteMacEnb.h"

class LteMacEnbRealistic : public LteMacEnb
{
  protected:

    /// List of scheduled users - Downlink
    LteMacScheduleList* scheduleListDl_;

    /**
     * Reads MAC parameters for eNb and performs initialization.
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
    virtual void macSduRequest();

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List (stored after scheduling).
     * It sends them to H-ARQ
     */
    virtual void macPduMake(MacCid cid);


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
     * Flush Tx H-ARQ buffers for all users
     */
    virtual void flushHarqBuffers();

  public:

    LteMacEnbRealistic();
    virtual ~LteMacEnbRealistic();

    virtual bool isD2DCapable()
    {
        return false;
    }

};

#endif
