//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

cplusplus {{
    #include "common/LteCommon.h"

    /**
     * Timer type
     */
    enum TTimerType
    {
       TTSIMPLE, TTMULTI
    };
}};

enum TTimerType;

packet TTimerMsg
{
    unsigned int timerId;
    unsigned short type enum(TTimerType);
}
