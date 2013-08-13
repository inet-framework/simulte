//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef ___4GSIM_GTP_USER_H_
#define ___4GSIM_GTP_USER_H_

#include <omnetpp.h>
#include "UDPSocket.h"
#include "IPvXAddressResolver.h"
#include "IPv4Datagram.h"
#include "TftControlInfo.h"
#include "gtpUserMsg_m.h"

#include <map>
#include "gtp_common.h"

/**
 * GtpUser is used for building data tunnels between GTP peers.
 * GtpUser can receive two kind of packets:
 * a) IP datagram from a trafficFilter. Those packets are labeled with a tftId
 * b) gtpUserMsg from UDP-IP layers. Those messages contains a TEIDin.
 *
 * In case (a) the GtpUser encapsulates the IP datagram within a gtpUserMsg then accesses the tftTable
 *  (by using the tftId) and obtains the TEID and nextHopAddress that will be used for sending the
 *  encapsulated IP packet in the GTP tunnel towards the chosen GTP peer
 *
 * In case (b) the GTP user accesses the teidTable (by using the TEIDin) and obtains the TEIDout and nextHopAddress
 *  - if the TEIDout == 127 the packet will be decapsulated and forwarded to the local network (note that 127 equals to
 *      the value LOCAL_ADDRESS_TEID as defined in "gtp_common.h"
 *  - otherwise the GtpUserMsg will be sent in the GTP tunnel towards the chosen GTP peer
 *
 * The teidTable and tftTable are filled via XML configuration files. All fields are mandatory
 *
 * Example format for teidTable
 <config>
 <teidTable>
 <teid
 teidIn  ="0"
 teidOut ="127"
 nextHop ="0.0.0.0"
 />
 <teid
 teidIn  ="1"
 teidOut ="2"
 nextHop ="192.168.1.1"
 />
 </teidTable>
 </config>
 *
 *
 * Example format for tftTable
 <config>
 <tftTable>
 <tft
 tftId   ="1"
 teidOut ="1"
 nextHop ="192.168.4.2"
 />
 </tftTable>
 </config>
 *
 */
class GtpUser : public cSimpleModule
{
    UDPSocket socket_;
    int localPort_;

    /*
     * This table contains mapping between an incoming TEID and <nextTEID,nextHop>
     * The GTP-U entity reads the incoming TEID and performs a TEID switch/removal depending on the values contained in the teidTable:
     * - if nextTEID==LOCAL_ADDRESS_TEID decapsulate the packet and forward it towards its original destination
     * - if nextTEID>0 then update the TEID value of the incoming packet with nextTEID and then forward it to nextHop
     */
    LabelTable teidTable_;

    /*
     * This table contains mapping between TrafficFlowTemplate (TFT) identifiers and <nextTEID,nextHop>
     * TFT are set by the traffic filter in the P-GW and UE
     */
    LabelTable tftTable_;

    // the GTP protocol Port
    unsigned int tunnelPeerPort_;

    bool loadTeidTable(const char * teidTableFile);
    bool loadTftTable(const char * tftTableFile);

  protected:

    virtual int numInitStages() const { return 4; }
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);

    // receive and IP Datagram from the traffic filter, encapsulates it in a GTP-U packet than forwards it to the proper next hop
    void handleFromTrafficFlowFilter(IPv4Datagram * datagram);

    // receive a GTP-U packet from UDP, reads the TEID and decides whether performing label switching or removal
    void handleFromUdp(GtpUserMsg * gtpMsg);
};

#endif
