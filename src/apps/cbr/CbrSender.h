//
//

#ifndef _CBRSENDER_H_
#define _CBRSENDER_H_

#include <string.h>
#include <omnetpp.h>

#include "inet/common/INETDefs.h"
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/cbr/CbrPacket_m.h"

using namespace inet;

class CbrSender : public cSimpleModule
{
    UDPSocket socket;
    //has the sender been initialized?
    bool initialized_;

    cMessage* selfSource_;
    //sender
    int iDtalk_;
    int nframes_;
    int iDframe_;
    int nframesTmp_;
    int size_;
    simtime_t sampling_time;
    simtime_t startTime_;
    simtime_t finishTime_;

    bool silences_;

    static simsignal_t cbrGeneratedThroughtputSignal_;
    static simsignal_t cbrGeneratedBytesSignal_;
    static simsignal_t cbrSentPktSignal_;

    int txBytes_;
    // ----------------------------

    cMessage *selfSender_;
    cMessage *initTraffic_;

    simtime_t timestamp_;
    int localPort_;
    int destPort_;
    L3Address destAddress_;

    void initTraffic();
    void sendCbrPacket();

  public:
    ~CbrSender();
    CbrSender();

  protected:

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;
    void handleMessage(cMessage *msg) override;
};

#endif

