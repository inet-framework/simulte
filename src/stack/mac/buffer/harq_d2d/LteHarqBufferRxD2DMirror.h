//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEHARQBUFFERRXD2DMIRROR_H_
#define _LTE_LTEHARQBUFFERRXD2DMIRROR_H_

#include "stack/mac/buffer/harq_d2d/LteHarqProcessRxD2DMirror.h"

class LteHarqBufferRx;

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
class LteHarqBufferRxD2DMirror
{
  public:

    /// number of contained H-ARQ processes
    unsigned int numHarqProcesses_;

    MacNodeId nodeId_; // UE nodeId for which this buffer has been created
    MacNodeId peerId_; //UE ID of the owner peer

    /// processes vector
    std::vector<LteHarqProcessRxD2DMirror*> processes_;

    //Max number of harq retransmission
    unsigned char maxHarqRtx_;
    //LteHarqBufferRxMirror();
  public:
    LteHarqBufferRxD2DMirror(LteHarqBufferRx* harqBuffer,unsigned char maxHarqRtx,MacNodeId peer);
    ~LteHarqBufferRxD2DMirror();
    void checkCorrupted();
    LteHarqProcessRxD2DMirror* getProcess(int proc) { return processes_[proc]; }
    unsigned int getProcesses() { return numHarqProcesses_; }
};

#endif
