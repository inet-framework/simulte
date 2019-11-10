//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/compManager/X2CompMsg.h"
#include "x2/packet/LteX2MsgSerializer.h"
#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"

using namespace omnetpp;
using namespace inet;


Register_Class(X2CompMsg);
Register_Serializer(X2CompMsg, LteX2MsgSerializer);

X2CompMsg::X2CompMsg() :
        LteX2Message()
    {
        type_ = X2_COMP_MSG;
    }


X2CompMsg::X2CompMsg(const X2CompMsg& other) : LteX2Message() { operator=(other); }

X2CompMsg& X2CompMsg::operator=(const X2CompMsg& other)
{
    if (&other == this)
        return *this;
    LteX2Message::operator=(other);
    return *this;
}

X2CompMsg* X2CompMsg::dup() const { return new X2CompMsg(*this); }

X2CompMsg::~X2CompMsg() { }
