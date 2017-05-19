//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_TRAFFICFLOWFILTER_H_
#define _LTE_TRAFFICFLOWFILTER_H_

#include <omnetpp.h>
//#include "trafficFlowTemplateMsg_m.h"
#include "epc/gtp/TftControlInfo.h"
#include "epc/gtp_common.h"

using namespace inet;

/**
 * Objective of the Traffic Flow Filter is mapping IP 4-Tuples to TFT identifiers. This commonly means identifying a bearer and
 * associating it to an ID that will be recognized by the first GTP-U entity
 *
 * The traffic filter uses a TrafficFilterTemplateTable that maps IP 4-Tuples to TFT identifiers.
 * The TrafficFilterTemplateTable has two levels:
 *  - at the first level there is up to one entry for each IPvXAddress. This may be a destination (on the P-GW side) or source (on the eNB side) address
 *  - at the second level each entry is a list of TrafficFlowTemplates structures, with a src/dest address (depending on the first level key,
 *    a dest and src port, and a tftId;
 *
 * When a packet comes to the traffic flow filter, an entry for the whole 4-tuple will be searched. In case of failure, the src and dest port will
 * be left unspecified and a new search will be performed. In case of another failure a last search with only the first key will be performed.
 * If no result is found even in this case, an error will be thrown.
 *
 * This table is specified via (part of) a XML configuration file. Note that the fields of the TrafficFlowTemplates (except for the tftId) may
 * be left unspecified
 *
 * Example format for traffic filter XML configuration
 </config>
 <filterTable>
 <filter
 destName   ="Host2"
 tftId      = "1"
 />
 <filter
 destAddr   = "10.1.1.1"
 tftId      = "2"
 />
 <filter
 destAddr   = "10.1.1.1"
 srcAddr    = "Host3"
 tftId      = "2"
 />
 </filterTable>
 </config>
 *
 * Each entry of the filter table is specified with a "filter" tag
 * For each filter entry the "tftId" and one between "destName" and "destAddr" ( or "srcName" and "srcAddr" for the eNB )
 * must be specified.
 * In case of both "destName" and "destAddr" values, the "destAddr" will be used
 *
 */
class TrafficFlowFilter : public cSimpleModule
{
    // specifies the type of the node that contains this filter (it can be ENB or PGW
    // the filterTable_ will be indexed differently depending on this parameter
    EpcNodeType ownerType_;

    // gate for connecting with the GTP-U module
    cGate * gtpUserGate_;

    TrafficFilterTemplateTable filterTable_;

    void loadFilterTable(const char * filterTableFile);

    EpcNodeType selectOwnerType(const char * type);
    protected:
    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage);

    // TrafficFlowFilter module may receive messages only from the input interface of its compound module
    virtual void handleMessage(cMessage *msg);

    // functions for managing filter tables
    TrafficFlowTemplateId findTrafficFlow(L3Address firstKey, TrafficFlowTemplate secondKey);
    bool addTrafficFlow(L3Address firstKey, TrafficFlowTemplate tft);
};

#endif
