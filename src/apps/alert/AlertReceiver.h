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

#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include "common/LteCommon.h"
#include "apps/alert/AlertPacket_m.h"

class SIMULTE_API AlertReceiver : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket;

    omnetpp::simsignal_t alertDelay_;
    omnetpp::simsignal_t alertRcvdMsg_;

    omnetpp::simtime_t delaySum;
    long nrReceived;

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;

    // utility: show current statistics above the icon
    virtual void refreshDisplay() const override;
};

#endif

