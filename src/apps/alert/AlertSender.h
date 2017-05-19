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
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "apps/alert/AlertPacket_m.h"

class AlertSender : public cSimpleModule
{
    inet::UDPSocket socket;

    //sender
    int nextSno_;
    int size_;
    simtime_t period_;

    simsignal_t alertSentMsg_;
    // ----------------------------

    cMessage *selfSender_;

    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    void sendAlertPacket();

  public:
    ~AlertSender();
    AlertSender();

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void initialize(int stage);
    void handleMessage(cMessage *msg);
};

#endif

