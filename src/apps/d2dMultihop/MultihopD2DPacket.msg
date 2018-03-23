//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

cplusplus {{
#include "inet/common/geometry/common/Coord.h"
}}

class noncobject inet::Coord;


//
// MultihopD2DPacket
//
// Extend this packet for creating your own multihop message
//
packet MultihopD2DPacket
{
    unsigned int srcId;
    uint32_t msgid;          // unique id of the message  < sender id  | local msg id >
    unsigned int size;
    
    inet::Coord srcCoord;
    double maxRadius;
    
    int ttl;                 // time-to-live (to avoid flooding)
    unsigned int hops;       // number of hops

    unsigned int lastHopSenderId;
    simtime_t timestamp;
}
