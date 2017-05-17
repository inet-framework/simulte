//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_GTP_USER_X2_H_
#define _LTE_GTP_USER_X2_H_

#include <omnetpp.h>
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "epc/gtp/GtpUserMsg_m.h"
#include "corenetwork/binder/LteBinder.h"
#include "x2/packet/LteX2Message.h"
#include <map>
#include "epc/gtp_common.h"

/**
 * GtpUserX2 is used for building data tunnels between GTP peers over X2, for handover procedure.
 * GtpUserX2 can receive two kind of packets:
 * a) LteX2Message from the X2 Manager. Those packets encapsulate an IP datagram
 * b) GtpUserX2Msg from UDP-IP layers.
 *
 */
class GtpUserX2 : public cSimpleModule
{
    UDPSocket socket_;
    int localPort_;

    // reference to the LTE Binder module
    LteBinder* binder_;

    // the GTP protocol Port
    unsigned int tunnelPeerPort_;

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);

    // receive an X2 Message from the X2 Manager, encapsulates it in a GTP-U packet than forwards it to the proper next hop
    void handleFromStack(LteX2Message * x2Msg);

    // receive a GTP-U packet from UDP, detunnel it and send it to the X2 Manager
    void handleFromUdp(GtpUserMsg * gtpMsg);
};

#endif
