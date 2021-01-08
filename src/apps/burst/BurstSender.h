//
//

#ifndef _BURSTSENDER_H_
#define _BURSTSENDER_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "common/LteCommon.h"
#include "apps/burst/BurstPacket_m.h"

using namespace inet;

class SIMULTE_API BurstSender : public cSimpleModule
{
    UdpSocket socket;
    //has the sender been initialized?
    bool initialized_;

    // timers
    cMessage* selfBurst_;
    cMessage* selfPacket_;

    //sender
    int idBurst_;
    int idFrame_;

    int burstSize_;
    int size_;
    simtime_t startTime_;
    simtime_t interBurstTime_;
    simtime_t intraBurstTime_;

    simsignal_t burstSentPkt_;
    // ----------------------------

    cMessage *selfSender_;
    cMessage *initTraffic_;

    simtime_t timestamp_;
    int localPort_;
    int destPort_;
    L3Address destAddress_;

    void initTraffic();
    void sendBurst();
    void sendPacket();

  public:
    ~BurstSender();
    BurstSender();

  protected:

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
};

#endif

