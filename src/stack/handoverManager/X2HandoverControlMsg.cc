//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "X2HandoverControlMsg.h"
#include "x2/packet/LteX2MsgSerializer.h"
#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"

using namespace omnetpp;
using namespace inet;

Register_Serializer(X2HandoverControlMsg, LteX2MsgSerializer);

X2HandoverControlMsg::X2HandoverControlMsg() :
        LteX2Message() {
    type_ = X2_HANDOVER_CONTROL_MSG;
}

X2HandoverControlMsg::X2HandoverControlMsg(const X2HandoverControlMsg& other) :
        LteX2Message() {
    operator=(other);
}

X2HandoverControlMsg& X2HandoverControlMsg::operator=(
        const X2HandoverControlMsg& other) {
    if (&other == this)
        return *this;
    LteX2Message::operator=(other);
    return *this;
}

X2HandoverControlMsg* X2HandoverControlMsg::dup() const {
    return new X2HandoverControlMsg(*this);
}

X2HandoverControlMsg::~X2HandoverControlMsg() {
}
