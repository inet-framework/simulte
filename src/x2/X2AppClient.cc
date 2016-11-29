//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


#include "x2/X2AppClient.h"
#include "corenetwork/binder/LteBinder.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/sctp/SCTPAssociation.h"
#include "inet/transportlayer/contract/sctp/SCTPCommand_m.h"

Define_Module(X2AppClient);

void X2AppClient::initialize(int stage)
{
    SCTPClient::initialize(stage);
    if (stage==inet::INITSTAGE_LOCAL)
    {
        x2ManagerOut_ = gate("x2ManagerOut");
    }
    else if (stage==inet::INITSTAGE_APPLICATION_LAYER)
    {
        // TODO set the connect address
        // Automatic configuration not yet supported. Use the .ini file to set IP addresses

        // get the connectAddress and the corresponding X2 id
        L3Address addr = L3AddressResolver().resolve(par("connectAddress").stringValue());
        X2NodeId peerId = getBinder()->getX2NodeId(addr.toIPv4());

        X2NodeId nodeId = check_and_cast<LteMacEnb*>(getParentModule()->getParentModule()->getSubmodule("lteNic")->getSubmodule("mac"))->getMacCellId();
        getBinder()->setX2PeerAddress(nodeId, peerId, addr);

        // set the connect port
        int connectPort = getBinder()->getX2Port(peerId);
        par("connectPort").setLongValue(connectPort);
    }
}

void X2AppClient::socketEstablished(int32_t, void *, unsigned long int buffer )
{
    EV << "X2AppClient: connected\n";
}

void X2AppClient::socketDataArrived(int32_t, void *, cPacket *msg, bool)
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

        delete smsg;
    }
    else
    {
        EV << "X2AppClient::socketDataArrived - No encapsulated message. Discard." << endl;

        // TODO: throw exception?

        delete msg;
    }
}


