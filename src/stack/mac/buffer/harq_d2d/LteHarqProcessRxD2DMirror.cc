//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//
#include "stack/mac/buffer/harq_d2d/LteHarqProcessRxD2DMirror.h"

LteHarqProcessRxD2DMirror::LteHarqProcessRxD2DMirror(unsigned char acid,unsigned char maxharq){
    pdu_length_.resize(MAX_CODEWORDS,0);
    status_.resize(MAX_CODEWORDS, RXHARQ_PDU_EMPTY);
    acid_ = acid;
    transmissions_ = 0;
    maxHarqRtx_ = maxharq;
};

std::vector<RxUnitStatus> LteHarqProcessRxD2DMirror::getProcessStatus()
{
    std::vector<RxUnitStatus> ret(MAX_CODEWORDS);

    for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
    {
        ret[j].first = j;
        ret[j].second = getStatus(j);
    }
    return ret;
}
