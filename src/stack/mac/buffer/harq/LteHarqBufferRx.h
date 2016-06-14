//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEHARQBUFFERRX_H_
#define _LTE_LTEHARQBUFFERRX_H_

#include "LteMacBase.h"
#include "LteHarqProcessRx.h"

class LteMacBase;
class LteHarqProcessRx;


/**
 * H-ARQ RX buffer: messages coming from phy are stored in H-ARQ RX buffer.
 * When a new pdu is inserted, it is in EVALUATING state meaning that the hardware is
 * checking its correctness.
 * A feedback is sent after the pdu has been evaluated (HARQ_FB_EVALUATION_INTERVAL), and
 * in case of ACK the pdu moves to CORRECT state, else it is dropped from the process.
 * The operations of checking if a pdu is ready for feedback and if it is in correct state are
 * done in the extractCorrectPdu mehtod which must be called at every tti (it must be part
 * of the mac main loop).
 */
class LteHarqBufferRx
{
  protected:
    /// mac module reference
    LteMacBase *macOwner_;

    /// number of contained H-ARQ processes
    unsigned int numHarqProcesses_;

    MacNodeId nodeId_; // UE nodeId for which this buffer has been created

    /// processes vector
    std::vector<LteHarqProcessRx *> processes_;

    //Statistics
    simsignal_t macDelay_;
    simsignal_t macCellThroughput_;
    simsignal_t macThroughput_;

    // reference to the eNB module
    cModule* nodeB_;

    TaggedSample *tSample_;
    TaggedSample *tSampleCell_;

  public:
    LteHarqBufferRx() {}
    LteHarqBufferRx(unsigned int num, LteMacBase *owner, MacNodeId nodeId);

    /**
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

    /**
     * Purges PDUs in corrupted state (if any)
     *
     * @return number of purged corrupted PDUs or zero if none
     */
    unsigned int purgeCorruptedPdus();

    /*
     * Returns pointer to <acid> process.
     */
    LteHarqProcessRx* getProcess(unsigned char acid)
    {
        return processes_.at(acid);
    }

    //* Returns the number of contained H-ARQ processes
    unsigned int getProcesses()
    {
        return numHarqProcesses_;
    }

    // @return whole buffer status {RXHARQ_PDU_EMPTY, RXHARQ_PDU_EVALUATING, RXHARQ_PDU_CORRECT, RXHARQ_PDU_CORRUPTED }
    RxBufferStatus getBufferStatus();

    /**
     * Returns a pair with h-arq process id and a list of its empty {RXHARQ_PDU_EMPTY} units to be used for reception of new H-arq sub-bursts.
     *
     * @return  a list of acid and their units  to be used for reception
     */
    UnitList firstAvailable();

    virtual ~LteHarqBufferRx();

  protected:
    /**
     * Checks for all processes if the pdu has been evaluated and sends
     * feedback if affirmative.
     */
    virtual void sendFeedback();
};

#endif
