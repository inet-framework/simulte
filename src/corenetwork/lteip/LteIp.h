//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEIP_H_
#define _LTE_LTEIP_H_

#include <omnetpp.h>
#include <stdlib.h>

#include "inet/queueing/base/PacketQueueBase.h"
#include "inet/networklayer/common/IpProtocolId_m.h"
#include "inet/transportlayer/udp/Udp.h"
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/packet/Message.h"
#include "inet/common/IProtocolRegistrationListener.h"

using namespace inet;

/**
 * @class LteIp
 * @brief Implements a simplified version of the IP protocol.
 *
 * This class implements a simplified version of the IP protocol,
 * to be used within the LTE model.
 * The behaviour of this IP module, as regards the data flow,
 * depends on the node type:
 * - INTERNET : Transport Layer <--> LteIp Peer
 * - ENODEB   : LteIp Peer <--> LTE Stack
 * - UE       : Transport Layer <--> LTE Stack
 *
 * This class differs from the INET IP module, but preserves
 * the same interface towards INET transport protocols.
 * When a transport packet is received from the transport layer,
 * it is encapsulated in an IP Datagram. Along with the datagram
 * there is a a control info (LteStackControlInfo), containing
 * the four tuple, a sequence number and the header size (IP+Transport).
 *
 */

class SIMULTE_API LteIp : public cSimpleModule, public IProtocolRegistrationListener
{
public:
    struct SocketDescriptor
    {
        int socketId = -1;
        int protocolId = -1;
        inet::Ipv4Address localAddress;
        inet::Ipv4Address remoteAddress;

        SocketDescriptor(int socketId, int protocolId, Ipv4Address localAddress)
                : socketId(socketId), protocolId(protocolId), localAddress(localAddress) { }
    };
  protected:
    cGate *stackGateOut_;           /// gate connecting LteIp module to LTE stack
    cGate *peerGateOut_;            /// gate connecting LteIp module to another LteIp
    int defaultTimeToLive_;         /// default ttl value
    LteNodeType nodeType_;     /// node type: can be INTERNET, ENODEB, UE

    // working vars
    long curFragmentId_;      /// counter, used to assign unique fragmentIds to datagrams (actually unused)
    unsigned int seqNum_;     /// datagram sequence number (RLC fragmentation needs it)
    // inet::ProtocolMapping mapping_; /// where to send transport packets after decapsulation
    std::set<const inet::Protocol *> upperProtocols;    // where to send packets after decapsulation
    std::map<int, SocketDescriptor *> socketIdToSocketDescriptor;

    // statistics
    int numForwarded_;  /// number of forwarded packets
    int numDropped_;    /// number of dropped packets

  public:

    /**
     * LteIp constructor.
     */
    LteIp()
    {
    }

    virtual ~LteIp()
    {
        for (auto it : socketIdToSocketDescriptor)
            delete it.second;
    }

  protected:

    virtual void handleRegisterService(const inet::Protocol& protocol, cGate *out, ServicePrimitive servicePrimitive) override;
    virtual void handleRegisterProtocol(const inet::Protocol& protocol, cGate *in, ServicePrimitive servicePrimitive) override;

    void decapsulate(inet::Packet *packet);

    void handleRequest(inet::Request *request);

    /**
     * utility: show current statistics above the icon
     */
    virtual void updateDisplayString();

    /**
     * Initialization
     */
    virtual void initialize(int stage) override;

    /**
     * Processing of IP datagrams.
     * Called when a datagram reaches the front of the queue.
     *
     * @param msg packet received
     */
    virtual void handleMessage(cMessage *msg) override;

  private:

    /**
     * Handle packets from transport layer and forward them
     * to the specified output gate, after encapsulation in
     * IP Datagram.
     * This method adds to the datagram the LteStackControlInfo.
     *
     * @param transportPacket transport packet received from transport layer
     * @param outputgate output gate where the datagram will be sent
     */
    void fromTransport(inet::Packet * transportPacket, omnetpp::cGate *outputgate);

    /**
     * Manage packets received from Lte Stack or LteIp peer
     * and forward them to transport layer.
     *
     * @param msg  IP Datagram received from Lte Stack or LteIp peer
     */
    void toTransport(inet::Packet * msg);

    /**
     * utility: set nodeType_ field
     *
     * @param s string containing the node type ("internet", "enodeb", "ue")
     */
    void setNodeType(std::string s);

    /**
     * utility: print LteStackControlInfo fields
     *
     * @param ci LteStackControlInfo object
     */
    void printControlInfo(inet::Packet* ci);
};

#endif

