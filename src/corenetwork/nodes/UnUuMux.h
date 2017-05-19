//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_UNUUMUX_H_
#define _LTE_UNUUMUX_H_

#include <omnetpp.h>
#include "common/LteControlInfo.h"

/**
 * @class UnUuMux
 * @brief Muxer for relay
 *
 */
class UnUuMux : public cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

  private:
    void handleUpperMessage(cMessage *msg);
    void handleLowerMessage(cMessage *msg);

    /**
     * Data structures
     */

    cGate* upUn_[2];
    cGate* upUu_[2];
    cGate* down_[2];
};

#endif
