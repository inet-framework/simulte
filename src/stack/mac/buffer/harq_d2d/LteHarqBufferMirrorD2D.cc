//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/buffer/harq_d2d/LteHarqBufferMirrorD2D.h"

LteHarqBufferMirrorD2D::LteHarqBufferMirrorD2D(unsigned int numProc, unsigned char maxHarqRtx)
{
    numProc_ = numProc;
    maxHarqRtx_ = maxHarqRtx;
    processes_.resize(numProc_, NULL);
    for (unsigned int i = 0; i < numProc_; i++)
        processes_[i] = new LteHarqProcessMirrorD2D(MAX_CODEWORDS, maxHarqRtx_);
}

void LteHarqBufferMirrorD2D::receiveHarqFeedback(LteHarqFeedbackMirror *fbpkt)
{
    EV << "LteHarqBufferMirrorD2D::receiveHarqFeedback - start" << endl;

    bool result = fbpkt->getResult();
    HarqAcknowledgment harqResult = result ? HARQACK : HARQNACK;
    Codeword cw = fbpkt->getCw();
    unsigned char acid = fbpkt->getAcid();
    unsigned int pduLength = fbpkt->getPduLength();

    processes_[acid]->storeFeedback(harqResult, pduLength, cw);

    // debug output
    const char *ack = result ? "ACK" : "NACK";
    EV << "H-ARQ MIRROR: feedback received for process " << (int)acid << " codeword " << (int)cw << " result is " << ack << endl;
    delete fbpkt;
}

void LteHarqBufferMirrorD2D::markSelectedAsWaiting()
{
    for (unsigned int i = 0; i < numProc_; i++)
    {
        std::vector<TxHarqPduStatus> status = processes_[i]->getProcessStatus();
        for (unsigned int cw = 0; cw < status.size(); cw++)
        {
            if (status[cw] == TXHARQ_PDU_SELECTED)
                processes_[i]->markWaiting(cw);
        }
    }
}

LteHarqBufferMirrorD2D::~LteHarqBufferMirrorD2D()
{
}
