//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERLCUMREALISTICD2D_H_
#define _LTE_LTERLCUMREALISTICD2D_H_

#include "stack/rlc/um/LteRlcUmRealistic.h"

/**
 * @class LteRlcUmRealisticD2D
 * @brief UM Module
 *
 * This is the UM Module of RLC (with support for D2D)
 *
 */
class LteRlcUmRealisticD2D : public LteRlcUmRealistic
{
  public:
    LteRlcUmRealisticD2D()
    {
    }
    virtual ~LteRlcUmRealisticD2D()
    {
    }

  protected:

    LteNodeType nodeType_;

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);

    /**
     * UM Mode
     *
     * handler for traffic coming from
     * lower layer (DTCH, MTCH, MCCH).
     *
     * handleLowerMessage() performs the following task:
     *
     * - Search (or add) the proper RXBuffer, depending
     *   on the packet CID
     * - Calls the RXBuffer, that from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    virtual void handleLowerMessage(cPacket *pkt);
};

#endif
