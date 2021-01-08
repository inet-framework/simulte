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
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"

/**
 * @class UnUuMux
 * @brief Muxer for relay
 *
 */
class SIMULTE_API UnUuMux : public omnetpp::cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(omnetpp::cMessage *msg);

  private:
    void handleUpperMessage(omnetpp::cMessage *msg);
    void handleLowerMessage(omnetpp::cMessage *msg);

    /**
     * Data structures
     */

    omnetpp::cGate* upUn_[2];
    omnetpp::cGate* upUu_[2];
    omnetpp::cGate* down_[2];
};

#endif
