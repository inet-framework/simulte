//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_AIRPHYRELAY_H_
#define _LTE_AIRPHYRELAY_H_

#include "stack/phy/layer/LtePhyBase.h"

class SIMULTE_API LtePhyRelay : public LtePhyBase
{
  private:
    /** Broadcast messages interval (equal to updatePos interval for mobility) */
    double bdcUpdateInterval_;

    /** Self message to trigger broadcast message sending for handover purposes */
    omnetpp::cMessage *bdcStarter_;

    /** Master MacNodeId */
    MacNodeId masterId_;

  protected:
    virtual void initialize(int stage) override;

    void handleSelfMessage(omnetpp::cMessage *msg) override;
    void handleAirFrame(omnetpp::cMessage* msg) override;
    public:
    virtual ~LtePhyRelay();
};

#endif  /* _LTE_AIRPHYRELAY_H_ */
