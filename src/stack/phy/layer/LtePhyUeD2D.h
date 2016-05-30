//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_AIRPHYUED2D_H_
#define _LTE_AIRPHYUED2D_H_

#include "LtePhyUe.h"

class LtePhyUeD2D : public LtePhyUe
{
  protected:

    // D2D Tx Power
    double d2dTxPower_;

    virtual void initialize(int stage);
    virtual void handleAirFrame(cMessage* msg);

  public:
    LtePhyUeD2D();
    virtual ~LtePhyUeD2D();

    virtual void sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req);
    virtual double getTxPwr(Direction dir = UNKNOWN_DIRECTION)
    {
        if (dir == D2D)
            return d2dTxPower_;
        return txPower_;
    }
};

#endif  /* _LTE_AIRPHYUED2D_H_ */
