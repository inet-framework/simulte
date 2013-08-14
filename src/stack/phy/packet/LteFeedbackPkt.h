//
//                           SimuLTE
// Copyright (C) 2012 Antonio Virdis, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEFEEDBACKPKT_H_
#define _LTE_LTEFEEDBACKPKT_H_

#include "LteFeedbackPkt_m.h"
#include "LteFeedback.h"

class LteFeedbackPkt : public LteFeedbackPkt_Base
{
  protected:
    // vector of vector with RU and TxMode as indexes
    LteFeedbackDoubleVector lteFeedbackDoubleVectorDl_;
    // vector of vector with RU and TxMode as indexes
    LteFeedbackDoubleVector lteFeedbackDoubleVectorUl_;
    //MacNodeId of the source
    MacNodeId sourceNodeId_;
    public:
    LteFeedbackPkt(const char *name = NULL, int kind = 0) :
        LteFeedbackPkt_Base(name, kind)
    {
    }
    LteFeedbackPkt(const LteFeedbackPkt& other) :
        LteFeedbackPkt_Base(other.getName())
    {
        operator=(other);
    }
    LteFeedbackPkt& operator=(const LteFeedbackPkt& other)
    {
        LteFeedbackPkt_Base::operator=(other);
        return *this;
    }
    virtual LteFeedbackPkt *dup() const
    {
        return new LteFeedbackPkt(*this);
    }
    // ADD CODE HERE to redefine and implement pure virtual functions from LteFeedbackPkt_Base
    LteFeedbackDoubleVector getLteFeedbackDoubleVectorDl();
    void setLteFeedbackDoubleVectorDl(LteFeedbackDoubleVector lteFeedbackDoubleVector_);
    LteFeedbackDoubleVector getLteFeedbackDoubleVectorUl();
    void setLteFeedbackDoubleVectorUl(LteFeedbackDoubleVector lteFeedbackDoubleVector_);
    void setSourceNodeId(MacNodeId id);
    MacNodeId getSourceNodeId();
};

#endif
