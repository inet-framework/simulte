//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEHARQPROCESSRXD2DMIRROR_H_
#define _LTE_LTEHARQPROCESSRXD2DMIRROR_H_

#include <omnetpp.h>
#include "common/LteCommon.h"

/**
 * H-ARQ RX processes contain pdus received from phy layer for which
 * H-ARQ feedback must be sent.
 * These pdus must be evaluated using H-ARQ correction functions.
 * H-ARQ RX process state machine has four states:
 * RXHARQ_PDU_EMPTY
 * RXHARQ_PDU_EVALUATING
 * RXHARQ_PDU_CORRECT
 * RXHARQ_PDU_CORRUPTED
 */

typedef std::pair<unsigned char, RxHarqPduStatus> RxUnitStatus;

class LteHarqProcessRxD2DMirror
{
  public:
    /// bytelength of contained pdus
    std::vector<int64_t> pdu_length_;

    /// H-ARQ process identifier
    unsigned char acid_;

    /// Number of (re)transmissions for current pdu (N.B.: values are 1,2,3,4)
    unsigned char transmissions_;

    unsigned char maxHarqRtx_;

    /// current status for each codeword
    std::vector<RxHarqPduStatus> status_;
    //LteHarqProcessRxMirror();
    LteHarqProcessRxD2DMirror(unsigned char acid,unsigned char maxharq);

    //virtual ~LteHarqProcessRxMirror();

    RxHarqPduStatus getStatus(Codeword cw) { return status_[cw]; }

    std::vector<RxUnitStatus> getProcessStatus();
};

#endif
