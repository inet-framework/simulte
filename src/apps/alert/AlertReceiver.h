//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_AlertReceiver_H_
#define _LTE_AlertReceiver_H_

#include <string.h>
#include <omnetpp.h>

#include "IPvXAddressResolver.h"
#include "UDPSocket.h"
#include "AlertPacket_m.h"

class AlertReceiver : public cSimpleModule
{
    UDPSocket socket;

    simsignal_t alertDelay_;
    simsignal_t alertRcvdMsg_;

  protected:

    virtual int numInitStages() const { return 4; }
    void initialize(int stage);
    void handleMessage(cMessage *msg);
};

#endif

