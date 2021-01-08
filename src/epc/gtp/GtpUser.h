//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_GTP_USER_H_
#define _LTE_GTP_USER_H_

#include <omnetpp.h>
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/common/packet/Packet.h"
#include "epc/gtp/TftControlInfo_m.h"
#include "epc/gtp/GtpUserMsg_m.h"

#include <map>
#include "epc/gtp_common.h"

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
class SIMULTE_API GtpUser : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket_;
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

    // specifies the type of the node that contains this filter (it can be ENB or PGW)
    EpcNodeType ownerType_;

    // determine gtpuser runs on PGW oder eNB
    EpcNodeType selectOwnerType(const char * type);

    // detect LTE interface
    inet::InterfaceEntry* detectInterface();
    inet::InterfaceEntry* ie_;

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage);
    virtual void handleMessage(inet::cMessage *msg);

    // receive and IP Datagram from the traffic filter, encapsulates it in a GTP-U packet than forwards it to the proper next hop
    void handleFromTrafficFlowFilter(inet::Packet * datagram);

    // receive a GTP-U packet from UDP, reads the TEID and decides whether performing label switching or removal
    //void handleFromUdp(GtpUserMsg * gtpMsg);
    void handleFromUdp(inet::Packet * gtpMsg);
};

#endif
