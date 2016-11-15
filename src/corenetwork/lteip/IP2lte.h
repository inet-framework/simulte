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
#include "LteControlInfo.h"
#include "LteControlInfo.h"
#include "IPv4Datagram.h"
#include "LteHandoverManager.h"
#include "LteBinder.h"

class LteHandoverManager;

// a sort of five-tuple with only two elements (a two-tuple...), src and dst addresses
typedef std::pair<IPv4Address, IPv4Address> AddressPair;

/**
 *
 */
class IP2lte : public cSimpleModule
{
    cGate *stackGateOut_;       // gate connecting IP2lte module to LTE stack
    cGate *ipGateOut_;          // gate connecting IP2lte module to network layer
    LteNodeType nodeType_;      // node type: can be ENODEB, UE

    // datagram sequence numbers (one for each flow)
    // TODO move numbering to PDCP
    std::map<AddressPair, unsigned int> seqNums_;

    // obsolete with the above map
    unsigned int seqNum_;       // datagram sequence number (RLC fragmentation needs it)

    // reference to the binder
    LteBinder* binder_;

    /*
     * Handover support
     */

    // manager for the handover
    LteHandoverManager* hoManager_;
    // store the pair <ue,target_enb> for temporary forwarding of data during handover
    std::map<MacNodeId, MacNodeId> hoForwarding_;
    // store the UEs for temporary holding of data received over X2 during handover
    std::set<MacNodeId> hoHolding_;

    typedef std::list<IPv4Datagram*> IpDatagramQueue;
    std::map<MacNodeId, IpDatagramQueue> hoFromX2_;
    std::map<MacNodeId, IpDatagramQueue> hoFromIp_;

    /**
     * Handle packets from transport layer and forward them to the stack
     */
    void fromIpUe(IPv4Datagram * datagram);

    /**
     * Manage packets received from Lte Stack
     * and forward them to transport layer.
     */
    void toIpUe(IPv4Datagram *datagram);

    void fromIpEnb(IPv4Datagram * datagram);
    void toIpEnb(cMessage * msg);
    void toStackEnb(IPv4Datagram* datagram);

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
    void printControlInfo(FlowControlInfo* ci);
    void registerInterface();
    void registerMulticastGroups();
  protected:
    virtual void initialize(int stage);
    virtual int numInitStages() const { return INITSTAGE_LAST; }
    virtual void handleMessage(cMessage *msg);
  public:
    void triggerHandoverSource(MacNodeId ueId, MacNodeId targetEnb);
    void triggerHandoverTarget(MacNodeId ueId, MacNodeId sourceEnb);
    void sendTunneledPacketOnHandover(IPv4Datagram* datagram, MacNodeId targetEnb);
    void receiveTunneledPacketOnHandover(IPv4Datagram* datagram, MacNodeId sourceEnb);
    void signalHandoverCompleteSource(MacNodeId ueId, MacNodeId targetEnb);
    void signalHandoverCompleteTarget(MacNodeId ueId, MacNodeId sourceEnb);
    virtual ~IP2lte();

};

#endif
