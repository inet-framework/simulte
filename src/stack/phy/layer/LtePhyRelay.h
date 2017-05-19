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

class LtePhyRelay : public LtePhyBase
{
  private:
    /** Broadcast messages interval (equal to updatePos interval for mobility) */
    double bdcUpdateInterval_;

    /** Self message to trigger broadcast message sending for handover purposes */
    cMessage *bdcStarter_;

    /** Master MacNodeId */
    MacNodeId masterId_;

  protected:
    virtual void initialize(int stage);

    void handleSelfMessage(cMessage *msg);
    void handleAirFrame(cMessage* msg);
    public:
    virtual ~LtePhyRelay();
};

#endif  /* _LTE_AIRPHYRELAY_H_ */
