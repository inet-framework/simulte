//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERLCUMD2D_H_
#define _LTE_LTERLCUMD2D_H_

#include "stack/rlc/um/LteRlcUm.h"

/**
 * @class LteRlcUmD2D
 * @brief UM Module
 *
 * This is the UM Module of RLC (with support for D2D)
 *
 */
class LteRlcUmD2D : public LteRlcUm
{
  public:
    LteRlcUmD2D()
    {
    }
    virtual ~LteRlcUmD2D()
    {
    }
    virtual void resumeDownstreamInPackets(MacNodeId peerId);
    virtual bool isEmptyingTxBuffer(MacNodeId peerId);

  protected:

    LteNodeType nodeType_;

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage);

    /**
     * getTxBuffer() is used by the sender to gather the TXBuffer
     * for that CID. If TXBuffer was already present, a reference
     * is returned, otherwise a new TXBuffer is created,
     * added to the tx_buffers map and a reference is returned aswell.
     *
     * @param lteInfo flow-related info
     * @return pointer to the TXBuffer for the CID of the flow
     *
     */
    virtual UmTxEntity* getTxBuffer(FlowControlInfo* lteInfo);

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

  private:

    std::map<MacNodeId, std::set<UmTxEntity*> > perPeerTxEntities_;
};

#endif
