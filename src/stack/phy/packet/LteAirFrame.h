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

#ifndef _LTE_LTEAIRFRAME_H_
#define _LTE_LTEAIRFRAME_H_

#include "LteCommon.h"
#include "LteAirFrame_m.h"

class LteAirFrame : public LteAirFrame_Base
{
  protected:
    RemoteUnitPhyDataVector remoteUnitPhyDataVector;
    public:
    LteAirFrame(const char *name = NULL, int kind = 0) :
        LteAirFrame_Base(name, kind)
    {
    }
    LteAirFrame(const LteAirFrame& other) :
        LteAirFrame_Base(other)
    {
        operator=(other);
    }
    LteAirFrame& operator=(const LteAirFrame& other)
    {
        LteAirFrame_Base::operator=(other);
        this->remoteUnitPhyDataVector = other.remoteUnitPhyDataVector;
        return *this;
    }
    virtual LteAirFrame *dup() const
    {
        return new LteAirFrame(*this);
    }
    // ADD CODE HERE to redefine and implement pure virtual functions from LteAirFrame_Base
    void addRemoteUnitPhyDataVector(RemoteUnitPhyData data);
    RemoteUnitPhyDataVector getRemoteUnitPhyDataVector();
};

Register_Class(LteAirFrame);
//TODO: this should go into a .cc file

#endif
