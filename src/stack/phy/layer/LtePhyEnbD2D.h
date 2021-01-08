//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_AIRPHYENBD2D_H_
#define _LTE_AIRPHYENBD2D_H_

#include "stack/phy/layer/LtePhyEnb.h"

class SIMULTE_API LtePhyEnbD2D : public LtePhyEnb
{
    friend class DasFilter;

    bool enableD2DCqiReporting_;

  protected:

    virtual void initialize(int stage) override;
    virtual void requestFeedback(UserControlInfo* lteinfo, LteAirFrame* frame, inet::Packet * pkt) override;
    virtual void handleAirFrame(omnetpp::cMessage* msg) override;

  public:
    virtual ~LtePhyEnbD2D();

};

#endif  /* _LTE_AIRPHYENBD2D_H_ */
