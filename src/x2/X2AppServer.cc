//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "x2/X2AppServer.h"

#include <inet/transportlayer/contract/sctp/SctpCommand_m.h>

Define_Module(X2AppServer);

using namespace omnetpp;
using namespace inet;

void X2AppServer::initialize(int stage)
{
    SctpServer::initialize(stage);
    if (stage==inet::INITSTAGE_LOCAL)
    {
        x2ManagerIn_ = gate("x2ManagerIn");

        X2NodeId id = getAncestorPar("macCellId");

        // register listening port to the binder. It will be used by
        // the client side as connectPort
        int localPort = par("localPort");
        getBinder()->registerX2Port(id, localPort);
    }
}

void X2AppServer::generateAndSend(cPacket* pkt)
{
    /**
     * TODO: this has to be reworked to make use of the new packet api
     * look here: inet version 4.0.0
     * src/inet/applications/netperfmeter/NetPerfMeter.cc
     * line 956
     */
    Packet* cmsg = new Packet("CMSG");
    SctpSimpleMessage* msg = new SctpSimpleMessage("Server");
    int numBytes = pkt->getByteLength();
    msg->setDataArraySize(numBytes);
    for (int i=0; i<numBytes; i++)
        msg->setData(i, 's');

    msg->setDataLen(numBytes);

    // encapsulate packet
    msg->setEncaps(true);
    msg->encapsulate(pkt);

    msg->setBitLength(numBytes * 8);
    cmsg->encapsulate(msg);
    auto command = cmsg->addTagIfAbsent<SctpSendReq>();
    //SctpSendInfo *cmd = new SctpSendInfo("Send1");
    //command->setAssocId(assocId);
    //
    // import from inet
    //command->setSocketId(ConnectionID);                                 

    // done
    //command->setSid(streamID);                                          
    //command->setSendUnordered( (sendUnordered == true) ?                
    //                           COMPLETE_MESG_UNORDERED : COMPLETE_MESG_ORDERED );
    //command->setLast(true);                                             

    /*
     * TODO: Ignored. Needed
     */
    //command->setPrimary(PrimaryPath.isUnspecified());                   
    //command->setRemoteAddr(PrimaryPath);                                
    //command->setPrValue(1);                                             
    //command->setPrMethod( (sendUnreliable == true) ? 2 : 0 );   // PR-Sctp policy: RTX



    //command->setSendUnordered(ordered ? COMPLETE_MESG_ORDERED : COMPLETE_MESG_UNORDERED);
    lastStream = (lastStream+1)%outboundStreams;
    command->setSid(lastStream);
    command->setPrValue(par("prValue"));
    command->setPrMethod((int32)par("prMethod"));
    if (queueSize>0 && numRequestsToSend > 0 && count < queueSize*2)
        command->setLast(false);
    else
        command->setLast(true);
    cmsg->setKind(SCTP_C_SEND);
    cmsg->setControlInfo(command);
    packetsSent++;
    bytesSent += msg->getBitLength()/8;

    sendOrSchedule(cmsg);
}

void X2AppServer::handleMessage(cMessage *msg)
{
    cPacket* pkt = check_and_cast<cPacket*>(msg);
    cGate* incoming = pkt->getArrivalGate();
    if (incoming == x2ManagerIn_)
    {
        EV << "X2AppServer::handleMessage - Received message from x2 manager" << endl;
        EV << "X2AppServer::handleMessage - Forwarding to X2 interface" << endl;

        // generate a Sctp packet and sent to lower layer
        generateAndSend(pkt);
    }
    else
    {
        SctpServer::handleMessage(msg);
    }
}

void X2AppServer::handleTimer(cMessage *msg)
{
    SctpServer::handleTimer(msg);
}
