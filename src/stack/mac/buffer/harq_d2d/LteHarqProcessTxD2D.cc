//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/buffer/harq_d2d/LteHarqProcessTxD2D.h"

LteHarqProcessTxD2D::LteHarqProcessTxD2D(unsigned char acid, unsigned int numUnits, unsigned int numProcesses, LteMacBase *macOwner, LteMacBase *dstMac)
{
    macOwner_ = macOwner;
    acid_ = acid;
    numHarqUnits_ = numUnits;
    units_ = new UnitVector(numUnits);
    numProcesses_ = numProcesses;
    numEmptyUnits_ = numUnits; //++ @ insert, -- @ unit reset (ack or fourth nack)
    numSelected_ = 0; //++ @ markSelected and insert, -- @ extract/sendDown

    // H-ARQ unit istances
    for (unsigned int i = 0; i < numHarqUnits_; i++)
    {
        (*units_)[i] = new LteHarqUnitTxD2D(acid, i, macOwner_, dstMac);
    }
}

LteHarqProcessTxD2D::~LteHarqProcessTxD2D()
{
}

LteMacPdu *LteHarqProcessTxD2D::extractPdu(Codeword cw)
{
    if (numSelected_ == 0)
        throw cRuntimeError("H-ARQ TX process: cannot extract pdu: numSelected = 0 ");

    numSelected_--;
    LteMacPdu *pdu = (*units_)[cw]->extractPdu();
    if (check_and_cast<LteControlInfo*>(pdu->getControlInfo())->getDirection() == D2D_MULTI)
    {
        // if the pdu is for a multicast/broadcast connection, the selected unit has been emptied
        numEmptyUnits_++;
    }
    return pdu;
}
