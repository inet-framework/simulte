//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteHarqProcessRxD2D.h"
#include "LteMacBase.h"
#include "LteControlInfo.h"
#include "LteHarqFeedback_m.h"
#include "LteMacPdu.h"

LteHarqProcessRxD2D::LteHarqProcessRxD2D(unsigned char acid, LteMacBase *owner)
    : LteHarqProcessRx(acid, owner)
{
}

LteHarqProcessRxD2D::~LteHarqProcessRxD2D()
{
}
