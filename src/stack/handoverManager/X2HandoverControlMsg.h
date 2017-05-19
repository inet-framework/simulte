//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_X2HANDOVERCONTROLMSG_H_
#define _LTE_X2HANDOVERCONTROLMSG_H_

#include "x2/packet/LteX2Message.h"
#include "common/LteCommon.h"

/**
 * @class X2HandoverControlMsg
 *
 * Class derived from LteX2Message
 * It defines the message exchanged between Handover managers
 */
class X2HandoverControlMsg : public LteX2Message
{

  public:

    X2HandoverControlMsg(const char* name = NULL, int kind = 0) :
        LteX2Message(name, kind)
    {
        type_ = X2_HANDOVER_CONTROL_MSG;
    }

    X2HandoverControlMsg(const X2HandoverControlMsg& other) : LteX2Message() { operator=(other); }

    X2HandoverControlMsg& operator=(const X2HandoverControlMsg& other)
    {
        if (&other == this)
            return *this;
        LteX2Message::operator=(other);
        return *this;
    }

    virtual X2HandoverControlMsg* dup() const { return new X2HandoverControlMsg(*this); }

    virtual ~X2HandoverControlMsg() { }
};

//Register_Class(X2HandoverControlMsg);

#endif

