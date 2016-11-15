//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_TFTCONTROLINFO_H_
#define _LTE_TFTCONTROLINFO_H_

#include <omnetpp.h>

using namespace omnetpp;

class TftControlInfo : public cObject
{
    unsigned int tft_;

  public:
    TftControlInfo();
    virtual ~TftControlInfo();

    void setTft(unsigned int tft)
    {
        tft_ = tft;
    }
    unsigned int getTft()
    {
        return tft_;
    }
};

#endif

