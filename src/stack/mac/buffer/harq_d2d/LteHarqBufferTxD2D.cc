//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteHarqBufferTxD2D.h"

LteHarqBufferTxD2D::LteHarqBufferTxD2D(unsigned int numProc, LteMacBase *owner, LteMacBase *dstMac)
{
    numProc_ = numProc;
    macOwner_ = owner;
    nodeId_ = dstMac->getMacNodeId();
    selectedAcid_ = HARQ_NONE;
    processes_ = new std::vector<LteHarqProcessTx *>(numProc);
    numEmptyProc_ = numProc;
    for (unsigned int i = 0; i < numProc_; i++)
    {
        (*processes_)[i] = new LteHarqProcessTxD2D(i, MAX_CODEWORDS, numProc_, macOwner_, dstMac);
    }
}

LteHarqBufferTxD2D::~LteHarqBufferTxD2D()
{
}
