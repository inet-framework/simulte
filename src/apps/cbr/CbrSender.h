//
//

#ifndef _CBRSENDER_H_
#define _CBRSENDER_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "common/LteCommon.h"
#include "apps/cbr/CbrPacket_m.h"

class SIMULTE_API CbrSender : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket;
    //has the sender been initialized?
    bool initialized_;

    omnetpp::cMessage* selfSource_;
    //sender
    int nframes_;
    int iDframe_;
    int nframesTmp_;
    int size_;
    omnetpp::simtime_t sampling_time;
    omnetpp::simtime_t startTime_;
    omnetpp::simtime_t finishTime_;

    static omnetpp::simsignal_t cbrGeneratedThroughtputSignal_;
    static omnetpp::simsignal_t cbrGeneratedBytesSignal_;
    static omnetpp::simsignal_t cbrSentPktSignal_;

    int txBytes_;
    // ----------------------------

    omnetpp::cMessage *selfSender_;
    omnetpp::cMessage *initTraffic_;

    omnetpp::simtime_t timestamp_;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    void initTraffic();
    void sendCbrPacket();

  public:
    ~CbrSender();
    CbrSender();

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;
    void handleMessage(omnetpp::cMessage *msg) override;
};

#endif

