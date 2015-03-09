//
//                           SimuLTE
// Copyright (C) 2015 Antonio Virdis, Giovanni Nardini, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


#include "X2AppClient.h"
#include "LteBinder.h"

#include "IPvXAddressResolver.h"
#include "SCTPAssociation.h"
#include "SCTPCommand_m.h"

Define_Module(X2AppClient);

void X2AppClient::initialize(int stage)
{
    SCTPClient::initialize(stage);
    if (stage==0)
    {
        x2ManagerOut_ = gate("x2ManagerOut");
    }
    else if (stage==4)
    {
        // TODO set the connect address
        // Automatic configuration not yet supported. Use the .ini file to set IP addresses

//    // automatic configuration for X2 mesh topology
//    IPv4NetworkConfigurator* configurator = check_and_cast<IPv4NetworkConfigurator*>(getModuleByPath("configurator"));
//    cModule* peer = simulation.getModule(getBinder()->getOmnetId(destId));
//
//    IPvXAddress addr = configurator->addressOf(peer, getParentModule()->getParentModule());
//
//    par("connectAddress").setStringValue(addr.str());

        // get the connectAddress and the corresponding X2 id
        IPvXAddress addr = IPvXAddressResolver().resolve(par("connectAddress").stringValue());
        X2NodeId peerId = getBinder()->getX2NodeId(addr.get4());

        // set the connect port
        int connectPort = getBinder()->getX2Port(peerId);
        par("connectPort").setLongValue(connectPort);
    }
}

void X2AppClient::socketEstablished(int32, void *, uint64 buffer )
{
    EV << "X2AppClient: connected\n";
    setStatusString("connected");
}

void X2AppClient::socketDataArrived(int32, void *, cPacket *msg, bool)
{
    packetsRcvd++;

    EV << "X2AppClient::socketDataArrived - Client received packet Nr " << packetsRcvd << " from SCTP\n";
    emit(rcvdPkSignal, msg);
    bytesRcvd += msg->getByteLength();

    SCTPSimpleMessage *smsg = check_and_cast<SCTPSimpleMessage*>(msg);
    if (smsg->getEncaps())
    {
        EV << "X2AppClient::socketDataArrived - Forwarding packet to the X2 manager" << endl;

        // extract encapsulated packet
        cMessage* encapMsg = smsg->decapsulate();

        // forward to x2manager
        send(encapMsg, x2ManagerOut_);
    }
    else
    {
        EV << "X2AppClient::socketDataArrived - No encapsulated message. Discard." << endl;

        // TODO: throw exception?

        delete msg;
    }
}


