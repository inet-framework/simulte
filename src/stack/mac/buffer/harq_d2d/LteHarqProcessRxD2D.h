//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEHARQPROCESSRXD2D_H_
#define _LTE_LTEHARQPROCESSRXD2D_H_

#include "stack/mac/buffer/harq/LteHarqProcessRx.h"

/**
 * H-ARQ RX processes contain pdus received from phy layer for which
 * H-ARQ feedback must be sent.
 * These pdus must be evaluated using H-ARQ correction functions.
 * H-ARQ RX process state machine has three states:
 * RXHARQ_PDU_EMPTY
 * RXHARQ_PDU_EVALUATING
 * RXHARQ_PDU_CORRECT
 */
class LteHarqProcessRxD2D : public LteHarqProcessRx
{
  public:

    /**
     * Constructor.
     *
     * @param acid process identifier
     * @param
     */
    LteHarqProcessRxD2D(unsigned char acid, LteMacBase *owner);

    /**
     * Creates a feedback message based on the evaluation result for this pdu.
     *
     * @return feedback message to be sent.
     */
    virtual LteHarqFeedback *createFeedback(Codeword cw);

    virtual ~LteHarqProcessRxD2D();
};

#endif
