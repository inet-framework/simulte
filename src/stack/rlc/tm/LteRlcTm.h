//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERLCTM_H_
#define _LTE_LTERLCTM_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "stack/rlc/LteRlcDefs.h"

/**
 * @class LteRlcTm
 * @brief TM Module
 *
 * This is the TM Module of RLC.
 * It implements the transparent mode (TM):
 *
 * - Transparent mode (TM):
 *   This mode is only used for control traffic, and
 *   the corresponding port is therefore accessed only from
 *   the RRC layer. Traffic on this port is simply forwarded
 *   to lower layer on ports BCCH/PCCH/CCCH
 *
 *   TM mode does not attach any header to the packet.
 *
 */
class LteRlcTm : public cSimpleModule
{
  public:
    LteRlcTm()
    {
    }
    virtual ~LteRlcTm()
    {
    }

  protected:
    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(cMessage *msg);

    virtual void initialize();
    virtual void finish()
    {
    }

  private:
    /**
     * handler for traffic coming
     * from the upper layer (RRC)
     *
     * handleUpperMessage() simply forwards packet to lower layers.
     * An empty header is added so that the encapsulation
     * level is the same for all packets transversing the stack
     *
     * @param pkt packet to process
     */
    void handleUpperMessage(cPacket *pkt);

    /**
     * handler for traffic coming from
     * lower layer
     *
     * handleLowerMessage() is the function that takes care
     * of TM traffic coming from lower layer.
     * After decapsulating the fictitious
     * header, packet is simply forwarded to the upper layer
     *
     * @param pkt packet to process
     */
    void handleLowerMessage(cPacket *pkt);

    /**
     * Data structures
     */

    cGate* up_[2];
    cGate* down_[2];
};

#endif
