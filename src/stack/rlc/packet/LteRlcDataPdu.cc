//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/packet/LteRlcDataPdu.h"

void LteRlcDataPdu::setPduSequenceNumber(unsigned int sno)
{
    pduSequenceNumber_ = sno;
}

unsigned int LteRlcDataPdu::getPduSequenceNumber()
{
    return pduSequenceNumber_;
}

void LteRlcDataPdu::setFramingInfo(FramingInfo fi)
{
    fi_ = fi;
}

FramingInfo LteRlcDataPdu::getFramingInfo()
{
    return fi_;
}
