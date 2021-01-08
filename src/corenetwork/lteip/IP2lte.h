//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __SIMULTE_IP2LTE_H_
#define __SIMULTE_IP2LTE_H_

#include <omnetpp.h>
#include <inet/networklayer/common/InterfaceEntry.h>

#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "stack/handoverManager/LteHandoverManager.h"
#include "corenetwork/binder/LteBinder.h"

class LteHandoverManager;

// a sort of five-tuple with only two elements (a two-tuple...), src and dst addresses
typedef std::pair<inet::Ipv4Address, inet::Ipv4Address> AddressPair;

/**
 *
 */
class SIMULTE_API IP2lte : public omnetpp::cSimpleModule
{
protected:
    omnetpp::cGate *stackGateOut_;       // gate connecting IP2lte module to LTE stack
    omnetpp::cGate *ipGateOut_;          // gate connecting IP2lte module to network layer

    LteNodeType nodeType_;      // node type: can be ENODEB, UE

    // datagram sequence numbers (one for each flow)
    // TODO move numbering to PDCP
    std::map<AddressPair, unsigned int> seqNums_;

    // obsolete with the above map
    unsigned int seqNum_;       // datagram sequence number (RLC fragmentation needs it)

    // reference to the binder
    LteBinder* binder_;

    // MAC node id of this node
    MacNodeId nodeId_;

    // corresponding entry for our interface
    inet::InterfaceEntry* interfaceEntry;

    /*
     * Handover support
     */

    // manager for the handover
    LteHandoverManager* hoManager_;
    // store the pair <ue,target_enb> for temporary forwarding of data during handover
    std::map<MacNodeId, MacNodeId> hoForwarding_;
    // store the UEs for temporary holding of data received over X2 during handover
    std::set<MacNodeId> hoHolding_;

    typedef std::list<inet::Packet*> IpDatagramQueue;
    std::map<MacNodeId, IpDatagramQueue> hoFromX2_;
    std::map<MacNodeId, IpDatagramQueue> hoFromIp_;

    bool ueHold_;
    IpDatagramQueue ueHoldFromIp_;
  protected:
    /**
     * Handle packets from transport layer and forward them to the stack
     */
    void fromIpUe(inet::Packet * datagram);

    /**
     * Manage packets received from Lte Stack
     * and forward them to transport layer.
     */
    virtual void prepareForIpv4(inet::Packet *datagram, const inet::Protocol *protocol = &inet::Protocol::ipv4);
    virtual void toIpUe(inet::Packet *datagram);
    virtual void fromIpEnb(inet::Packet * datagram);
    virtual void toIpEnb(inet::Packet * datagram);
    virtual void toStackEnb(inet::Packet* datagram);
    virtual void toStackUe(inet::Packet* datagram);

    /**
     * utility: set nodeType_ field
     *
     * @param s string containing the node type ("enodeb", "ue")
     */
    void setNodeType(std::string s);

    /**
     * utility: print LteStackControlInfo fields
     *
     * @param ci LteStackControlInfo object
     */
    void printControlInfo(inet::Packet* pkt);
    void registerInterface();
    void registerMulticastGroups();

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::INITSTAGE_LAST; }
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;
  public:

    /*
     * Handover management at the eNB side
     */
    void triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb);
    void triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb);
    void sendTunneledPacketOnHandover(inet::Packet* datagram, MacNodeId targetEnb);
    void receiveTunneledPacketOnHandover(inet::Packet* datagram, MacNodeId sourceEnb);
    void signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb);
    void signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb);

    /*
     * Handover management at the UE side
     */
    void triggerHandoverUe();
    void signalHandoverCompleteUe();

    virtual ~IP2lte();
};

#endif
