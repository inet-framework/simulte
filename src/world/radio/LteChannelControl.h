//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef LTECHANNELCONTROL_H
#define LTECHANNELCONTROL_H

#include "world/radio/ChannelControl.h"

/**
 * Monitors which radios are "in range"
 *
 * @ingroup LteChannelControl
 * @see ChannelAccess
 */
class LteChannelControl : public ChannelControl
{
  protected:

    /** Calculate interference distance*/
    virtual double calcInterfDist();

    /** Reads init parameters and calculates a maximal interference distance*/
    virtual void initialize();

  public:
    LteChannelControl();
    virtual ~LteChannelControl();

    /** Called from ChannelAccess, to transmit a frame to all the radios in range, on the frame's channel */
    virtual void sendToChannel(RadioRef srcRadio, AirFrame *airFrame);
};

#endif
