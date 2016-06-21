//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_ALERTSENDER_H_
#define _LTE_ALERTSENDER_H_

#include <string.h>
#include <omnetpp.h>
#include "UDPSocket.h"
#include "IPvXAddressResolver.h"
#include "AlertPacket_m.h"

class AlertSender : public cSimpleModule
{
    UDPSocket socket;

    //sender
    int nextSno_;
    int size_;
    simtime_t period_;

    simsignal_t alertSentMsg_;
    // ----------------------------

    cMessage *selfSender_;

    int localPort_;
    int destPort_;
    IPvXAddress destAddress_;

    void sendAlertPacket();

  public:
    ~AlertSender();
    AlertSender();

  protected:

    virtual int numInitStages() const { return 4; }
    void initialize(int stage);
    void handleMessage(cMessage *msg);
};

#endif

