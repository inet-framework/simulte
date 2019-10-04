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

#include <stdlib.h>
#include <omnetpp.h>
#include <inet/networklayer/common/IpProtocolId_m.h>
#include <inet/transportlayer/tcp_common/TcpHeader.h>
#include <inet/transportlayer/udp/Udp.h>

#include "common/LteControlInfo.h"

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
class LteIp : public inet::OperationalBase
{
    protected:
        omnetpp::cGate *stackGateOut_;           /// gate connecting LteIp module to LTE stack
        omnetpp::cGate *peerGateOut_;            /// gate connecting LteIp module to another LteIp
        int defaultTimeToLive_;         /// default ttl value
        LteNodeType nodeType_;     /// node type: can be INTERNET, ENODEB, UE
    
        // working vars
        long curFragmentId_;      /// counter, used to assign unique fragmentIds to datagrams (actually unused)
        unsigned int seqNum_;     /// datagram sequence number (RLC fragmentation needs it)

        // attribute dropped in favor of the new inet packet api. Needs more testing
        //inet::ProtocolMapping mapping_; /// where to send transport packets after decapsulation
    
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

    /**
     * ILifecycle methods
     */
    virtual bool isInitializeStage(int stage) override {
        return stage == inet::INITSTAGE_NETWORK_LAYER;
    }
    virtual bool isModuleStartStage(int stage) override {
        return stage == inet::ModuleStartOperation::STAGE_NETWORK_LAYER;
    }
    virtual bool isModuleStopStage(int stage) override {
        return stage == inet::ModuleStopOperation::STAGE_NETWORK_LAYER;
    }
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;


    protected:

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
        virtual void endService(omnetpp::cPacket *msg);

        /**
         * @see OperationalBase::handleMessageWhenUp
         *
         * @param msg Message to be handled
         */
        virtual void handleMessageWhenUp(inet::cMessage *msg) override;


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
        void fromTransport(omnetpp::cPacket * transportPacket, omnetpp::cGate *outputgate);

        /**
         * Manage packets received from Lte Stack or LteIp peer
         * and forward them to transport layer.
         *
         * @param msg  IP Datagram received from Lte Stack or LteIp peer
         */
        void toTransport(omnetpp::cPacket * msg);

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
        void printControlInfo(FlowControlInfo* ci);
};

#endif

