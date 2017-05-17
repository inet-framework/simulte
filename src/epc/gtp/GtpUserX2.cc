//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "epc/gtp/GtpUserX2.h"
#include "inet/networklayer/common/L3Address.h"
#include <iostream>

Define_Module(GtpUserX2);

void GtpUserX2::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    // get reference to the binder
    binder_ = getBinder();

    socket_.setOutputGate(gate("udpOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");
}

void GtpUserX2::handleMessage(cMessage *msg)
{
    if (strcmp(msg->getArrivalGate()->getFullName(), "lteStackIn") == 0)
    {
        EV << "GtpUserX2::handleMessage - message from X2 Manager" << endl;

        // obtain the encapsulated IPv4 datagram
        LteX2Message* x2Msg = check_and_cast<LteX2Message*>(msg);
        handleFromStack(x2Msg);
    }
    else if(strcmp(msg->getArrivalGate()->getFullName(),"udpIn")==0)
    {
        EV << "GtpUserX2::handleMessage - message from udp layer" << endl;

        GtpUserMsg * gtpMsg = check_and_cast<GtpUserMsg *>(msg);
        handleFromUdp(gtpMsg);
    }
}

void GtpUserX2::handleFromStack(LteX2Message* x2Msg)
{
    // extract destination from the message
    X2NodeId destId = x2Msg->getDestinationId();
    X2NodeId srcId = x2Msg->getSourceId();
    EV << "GtpUserX2::handleFromStack - Received a LteX2Message with destId[" << destId << "]" << endl;

    // create a new GtpUserMessage
    GtpUserMsg * gtpMsg = new GtpUserMsg();
    gtpMsg->setName("GtpUserMessage");

    // encapsulate the datagram within the GtpUserX2Message
    gtpMsg->encapsulate(x2Msg);

    // get the IP address of the destination X2 interface from the Binder
    L3Address peerAddress = binder_->getX2PeerAddress(srcId, destId);
    socket_.sendTo(gtpMsg, peerAddress, tunnelPeerPort_);
}

void GtpUserX2::handleFromUdp(GtpUserMsg * gtpMsg)
{
    EV << "GtpUserX2::handleFromUdp - Decapsulating and sending to local connection." << endl;

    // obtain the original X2 message and send it to the X2 Manager
    LteX2Message * x2Msg = check_and_cast<LteX2Message*>(gtpMsg->decapsulate());
    delete(gtpMsg);

    // send message to the X2 Manager
    send(x2Msg,"lteStackOut");
}
