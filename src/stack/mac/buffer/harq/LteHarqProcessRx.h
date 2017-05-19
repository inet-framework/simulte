//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEHARQPROCESSRX_H_
#define _LTE_LTEHARQPROCESSRX_H_

#include <omnetpp.h>

#include "common/LteCommon.h"

typedef std::pair<unsigned char, RxHarqPduStatus> RxUnitStatus;
typedef std::vector<std::vector<RxUnitStatus> > RxBufferStatus;

class LteMacBase;
class LteMacPdu;
class LteHarqFeedback;

/**
 * H-ARQ RX processes contain pdus received from phy layer for which
 * H-ARQ feedback must be sent.
 * These pdus must be evaluated using H-ARQ correction functions.
 * H-ARQ RX process state machine has three states:
 * RXHARQ_PDU_EMPTY
 * RXHARQ_PDU_EVALUATING
 * RXHARQ_PDU_CORRECT
 */
// TODO: controllare numero di processi UL/DL, UE, ENB
// TODO: statistiche per i drop (aggiungere membro?)
class LteHarqProcessRx
{
  protected:
    /// contained pdus
    std::vector<LteMacPdu *> pdu_;

    /// current status for each codeword
    std::vector<RxHarqPduStatus> status_;

    /// reception time timestamp
    std::vector<simtime_t> rxTime_;

    // reception status of buffered pdus
    std::vector<bool> result_;

    /// H-ARQ process identifier
    unsigned char acid_;

    /// mac module to manage errors (endSimulation)
    LteMacBase *macOwner_;

    /// Number of (re)transmissions for current pdu (N.B.: values are 1,2,3,4)
    unsigned char transmissions_;

    unsigned char maxHarqRtx_;

  public:

    /**
     * Constructor.
     *
     * @param acid process identifier
     * @param
     */
    LteHarqProcessRx(unsigned char acid, LteMacBase *owner);

    /**
     * Inserts a pdu into the process and evaluates it (corrupted or correct).
     *
     * @param pdu pdu to be inserted
     */
    virtual void insertPdu(Codeword cw, LteMacPdu *pdu);

    /**
     * Tells if contained pdus have been evaluated and feedback responses can be
     * sent.
     *
     * A pdu is evaluated if 4ms has been elapsed since when it was put into the process.
     *
     * @return true if the pdu can be acked/nacked, false if the evaluation is still in process.
     */
    virtual bool isEvaluated(Codeword cw);

    /**
     * Creates a feedback message based on the evaluation result for this pdu.
     *
     * @return feedback message to be sent.
     */
    virtual LteHarqFeedback *createFeedback(Codeword cw);

    /**
     * Tells if a pdu is in correct state (not corrupted, exctractable).
     *
     * @return true if correct, no otherwise
     */
    virtual bool isCorrect(Codeword cw);

    /**
     * Returns process/codeword PDU size (bytes)  [only for buffered status] .
     *
     * @return size if PDU is present, 0 otherwise
     */
    virtual int64_t getByteLength(Codeword cw);

    /**
     * @return the codeword status
     */
    virtual RxHarqPduStatus getUnitStatus(Codeword cw)
    {
        return status_.at(cw);
    }

    /**
     * @return whole buffer status
     */

    virtual std::vector<RxUnitStatus> getProcessStatus();
    /**
     * Extracts a pdu that can be passed to mac layer and reset process status.
     *
     * @return pdu ready for upper layer
     */
    virtual LteMacPdu *extractPdu(Codeword cw);

    /**
     * Purges a corrupted PDU that has been received on codeword <cw>
     */
    void purgeCorruptedPdu(Codeword cw);

    /**
     * Resets the status of a codeword containing a corrupted PDU
     */
    virtual void resetCodeword(Codeword cw);

    /**
     * @return number of codewords available for this process (set to MAX_CODEWORDS by default)
     */
    virtual unsigned int getNumHarqUnits()
    {
        return MAX_CODEWORDS;
    }

    virtual ~LteHarqProcessRx();

  protected:
};

#endif
