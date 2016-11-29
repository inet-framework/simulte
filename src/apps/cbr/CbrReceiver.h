#ifndef _CBRRECEIVER_H_
#define _CBRRECEIVER_H_

#include <string.h>
#include <omnetpp.h>

#include "inet/common/INETDefs.h"
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "CbrPacket_m.h"

using namespace inet;

class CbrReceiver : public cSimpleModule
{
    UDPSocket socket;

    ~CbrReceiver();

    int numReceived_;
    int totFrames_;

    int recvBytes_;


    bool mInit_;

    static simsignal_t cbrFrameLossSignal_;
    static simsignal_t cbrFrameDelaySignal_;
    static simsignal_t cbrJitterSignal_;
    static simsignal_t cbrReceivedThroughtput_;
//    static simsignal_t cbrReceivedThroughtput_rate_;

  protected:

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    void initialize(int stage);
    void handleMessage(cMessage *msg);
    virtual void finish() override;
};

#endif

