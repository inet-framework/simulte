//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_MULTIHOPD2D_H_
#define _LTE_MULTIHOPD2D_H_

#include <string.h>
#include <omnetpp.h>
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "apps/d2dMultihop/MultihopD2DPacket_m.h"
#include "apps/d2dMultihop/statistics/MultihopD2DStatistics.h"
#include "apps/d2dMultihop/eventGenerator/EventGenerator.h"
#include "stack/mac/layer/LteMacBase.h"
#include "stack/phy/layer/LtePhyBase.h"

class EventGenerator;

class MultihopD2D : public cSimpleModule
{
    static uint16_t numMultihopD2DApps;  // counter of apps (used for assigning the ids)

protected:
    uint16_t senderAppId_;             // unique identifier of the application within the network
    uint16_t localMsgId_;              // least-significant bits for the identifier of the next message
    int msgSize_;

    double selfishProbability_;         // if = 0, the node is always collaborative
    int ttl_;                           // if < 0, do not use hops to limit the flooding
    double maxBroadcastRadius_;         // if < 0, do not use radius to limit the flooding
    simtime_t maxTransmissionDelay_;    // if > 0, when a new message has to be transmitted, choose an offset between 0 and maxTransmissionDelay_

    /*
     * Trickle parameters
     */
    bool trickleEnabled_;
    unsigned int k_;
    simtime_t I_;
    std::map<unsigned int, MultihopD2DPacket*> last_;
    std::map<unsigned int, unsigned int> counter_;
    /***************************************************/

    std::map<uint32_t,bool> relayedMsgMap_;  // indicates if a received message has been relayed before

    int localPort_;
    int destPort_;
    L3Address destAddress_;
    UDPSocket socket;

    cMessage *selfSender_;

    EventGenerator* eventGen_;          // reference to the eventGenerator
    LtePhyBase* ltePhy_;                // reference to the LtePhy
    MacNodeId lteNodeId_;               // LTE Node Id
    MacNodeId lteCellId_;               // LTE Cell Id

    // local statistics
    simsignal_t d2dMultihopGeneratedMsg_;
    simsignal_t d2dMultihopSentMsg_;
    simsignal_t d2dMultihopRcvdMsg_;
    simsignal_t d2dMultihopRcvdDupMsg_;
    simsignal_t d2dMultihopTrickleSuppressedMsg_;

    // reference to the statistics manager
    MultihopD2DStatistics* stat_;

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    void markAsReceived(uint32_t msgId);      // store the msg id in the set of received messages
    bool isAlreadyReceived(uint32_t msgId);   // returns true if the given msg has already been received
    void markAsRelayed(uint32_t msgId);       // set the corresponding entry in the set as relayed
    bool isAlreadyRelayed(uint32_t msgId);   // returns true if the given msg has already been relayed
    bool isWithinBroadcastArea(Coord srcCoord, double maxRadius);

    virtual void sendPacket();
    virtual void handleRcvdPacket(cMessage* msg);
    virtual void handleTrickleTimer(cMessage* msg);
    virtual void relayPacket(cMessage* msg);

  public:
    MultihopD2D();
    ~MultihopD2D();

    virtual void handleEvent(unsigned int eventId);
};

#endif

