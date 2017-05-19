//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_GTP_COMMON_H_
#define _LTE_GTP_COMMON_H_

#include <map>
#include <list>
#include "inet/networklayer/common/L3Address.h"

using namespace inet;

enum EpcNodeType
{
    ENB,
    PGW,
    SGW
};

#define LOCAL_ADDRESS_TEID -1

#define UNSPECIFIED_PORT 65536

#define UNSPECIFIED_TFT 65536

//===================== GTP-U tunnel management =====================
typedef int TunnelEndpointIdentifier;

struct ConnectionInfo
{
    ConnectionInfo(TunnelEndpointIdentifier id, L3Address hop) :
        teid(id), nextHop(hop)
    {
    }

    TunnelEndpointIdentifier teid;
    L3Address nextHop;
};

typedef std::map<TunnelEndpointIdentifier, ConnectionInfo> LabelTable;
//===================================================================

//=================== Traffic filters management ====================
// identifies a traffic flow template
typedef int TrafficFlowTemplateId;

struct TrafficFlowTemplate
{
    TrafficFlowTemplate(L3Address ad, unsigned int src, unsigned int dest) :
        addr(ad), srcPort(src), destPort(dest)
    {
    }
    L3Address addr;

    unsigned int srcPort;
    unsigned int destPort;

    TrafficFlowTemplateId tftId;

    bool operator==(const TrafficFlowTemplate & b)
    {
        if ((b.addr == addr) && (b.srcPort == srcPort) && (b.destPort == destPort))
            return true;
        else
            return false;
    }
};

// contains a list of traffic flow templates associated to a src/dest address. It is used By the Traffic Flow Filter
typedef std::list<TrafficFlowTemplate> TrafficFilterTemplateList;

typedef std::map<L3Address, TrafficFilterTemplateList> TrafficFilterTemplateTable;
//===================================================================

char * const * loadXmlTable(char const * attributes[], unsigned int numAttributes);

#endif
