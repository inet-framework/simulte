//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


cplusplus {{
#include "stack/rlc/packet/LteRlcPdu.h"
#include "stack/rlc/LteRlcDefs.h"
}}

packet LteRlcPdu;
enum LteAmType;


// Rlc Am Control fields for AM control PDUs
//class LteRlcAmControlInfo
//{
//     @customize(true);
//     unsigned int firstSn;
//     unsigned int lastSn;
//     abstract bool bitmap[];
//}

packet LteRlcAmPdu extends LteRlcPdu
{
    @customize(true);
    unsigned short amType enum(LteAmType);
    unsigned short txNumber;
    int firstSn;
     int lastSn;
     abstract bool bitmap[];
}