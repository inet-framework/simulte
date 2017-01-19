//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEHARQBUFFERRXD2D_H_
#define _LTE_LTEHARQBUFFERRXD2D_H_

#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/harq_d2d/LteHarqProcessRxD2D.h"

class LteHarqProcessRxD2D;


/**
 * H-ARQ RX buffer for D2D: messages coming from phy are stored in H-ARQ RX buffer.
 * When a new pdu is inserted, it is in EVALUATING state meaning that the hardware is
 * checking its correctness.
 * A feedback is sent after the pdu has been evaluated (HARQ_FB_EVALUATION_INTERVAL), and
 * in case of ACK the pdu moves to CORRECT state, else it is dropped from the process.
 * The operations of checking if a pdu is ready for feedback and if it is in correct state are
 * done in the extractCorrectPdu mehtod which must be called at every tti (it must be part
 * of the mac main loop).
 */
class LteHarqBufferRxD2D : public LteHarqBufferRx
{
  protected:

    // D2D Statistics
    simsignal_t macDelayD2D_;
    simsignal_t macCellThroughputD2D_;
    simsignal_t macThroughputD2D_;

    /**
     * Checks for all processes if the pdu has been evaluated and sends
     * feedback if affirmative.
     */
    virtual void sendFeedback();

  public:
    LteHarqBufferRxD2D(unsigned int num, LteMacBase *owner, MacNodeId nodeId, bool isMulticast=false);

    /*
     * Insertion of a new pdu coming from phy layer into
     * RX H-ARQ buffer.
     *
     * @param pdu to be inserted
     */
    virtual void insertPdu(Codeword cw, LteMacPdu *pdu);

    /**
     * Sends feedback for all processes which are older than
     * HARQ_FB_EVALUATION_INTERVAL, then extract the pdu in correct state (if any)
     *
     * @return uncorrupted pdus or empty list if none
     */
    virtual std::list<LteMacPdu*> extractCorrectPdus();

    virtual ~LteHarqBufferRxD2D();
};

#endif
