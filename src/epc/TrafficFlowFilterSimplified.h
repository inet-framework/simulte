//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_TRAFFICFLOWFILTERSIMPLIFIED_H_
#define _LTE_TRAFFICFLOWFILTERSIMPLIFIED_H_

#include <omnetpp.h>
#include "epc/gtp/TftControlInfo.h"
#include "epc/gtp_common.h"
#include "corenetwork/binder/LteBinder.h"

/**
 * Objective of the Traffic Flow Filter is mapping IP 4-Tuples to TFT identifiers. This commonly means identifying a bearer and
 * associating it to an ID that will be recognized by the first GTP-U entity
 *
 * This simplified traffic filter queries the LTE Binder to find the destination of the packet.
 * It resides at both the eNodeB and the PGW. At the PGW, when a packet comes to the traffic flow filter, the latter finds the
 * destination endpoint (the eNodeB serving the destination UE) of the GTP tunnel that needs to be established. At the eNodeB,
 * the destination endpoint is always the PGW. However, if the fastForwarding flag is enabled and the destination of the packet
 * is within the same cell, the packet is just relayed to the Radio interface.
 */
class TrafficFlowFilterSimplified : public cSimpleModule
{
    // specifies the type of the node that contains this filter (it can be ENB or PGW
    // the filterTable_ will be indexed differently depending on this parameter
    EpcNodeType ownerType_;

    // reference to the LTE Binder module
    LteBinder* binder_;

    // gate for connecting with the GTP-U module
    cGate * gtpUserGate_;

    // if this flag is set, each packet received from the radio network, having the same radio network as destination
    // must be re-sent down without going through the Internet
    bool fastForwarding_;

    TrafficFilterTemplateTable filterTable_;

    EpcNodeType selectOwnerType(const char * type);
  protected:
    virtual int numInitStages() const { return INITSTAGE_LAST+1; }
    virtual void initialize(int stage);

    // TrafficFlowFilter module may receive messages only from the input interface of its compound module
    virtual void handleMessage(cMessage *msg);

    // functions for managing filter tables
    TrafficFlowTemplateId findTrafficFlow(L3Address srcAddress, L3Address destAddress);
};

#endif
