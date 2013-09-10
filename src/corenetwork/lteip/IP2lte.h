//
//                           SimuLTE
// Copyright (C) 2013 Antonio Virdis
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


/**
 *
 */
class IP2lte : public cSimpleModule
{
    cGate *stackGateOut_;       // gate connecting IP2lte module to LTE stack
    cGate *ipGateOut_;          // gate connecting IP2lte module to network layer
    LteNodeType nodeType_;      // node type: can be ENODEB, UE

    unsigned int seqNum_;       // datagram sequence number (RLC fragmentation needs it)

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
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

};

#endif
