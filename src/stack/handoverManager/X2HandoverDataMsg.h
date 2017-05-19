//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_X2HANDOVERDATAMSG_H_
#define _LTE_X2HANDOVERDATAMSG_H_

#include "x2/packet/LteX2Message.h"
#include "common/LteCommon.h"

/**
 * @class X2HandoverDataMsg
 *
 * Class derived from LteX2Message
 * It defines the message that encapsulata datagram to be exchanged between Handover managers
 */
class X2HandoverDataMsg : public LteX2Message
{

  public:

    X2HandoverDataMsg(const char* name = NULL, int kind = 0) :
        LteX2Message(name, kind)
    {
        type_ = X2_HANDOVER_DATA_MSG;
    }

    X2HandoverDataMsg(const X2HandoverDataMsg& other) : LteX2Message() { operator=(other); }

    X2HandoverDataMsg& operator=(const X2HandoverDataMsg& other)
    {
        if (&other == this)
            return *this;
        LteX2Message::operator=(other);
        return *this;
    }

    virtual X2HandoverDataMsg* dup() const { return new X2HandoverDataMsg(*this); }

    virtual ~X2HandoverDataMsg() { }
};

//Register_Class(X2HandoverDataMsg);

#endif

