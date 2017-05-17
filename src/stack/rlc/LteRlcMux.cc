//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/rlc/LteRlcMux.h"

Define_Module(LteRlcMux);

/*
 * Upper Layer handler
 */

void LteRlcMux::rlc2mac(cPacket *pkt)
{
    EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port RLC_to_MAC\n";

    // Send message
    send(pkt,macSap_[OUT]);
}

    /*
     * Lower layer handler
     */

void LteRlcMux::mac2rlc(cPacket *pkt)
{
    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());
    switch (lteInfo->getRlcType())
    {
        case TM:
        EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port TM_Sap$o\n";
        send(pkt,tmSap_[OUT]);
        break;
        case UM:
        EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port UM_Sap$o\n";
        send(pkt,umSap_[OUT]);
        break;
        case AM:
        EV << "LteRlcMux : Sending packet " << pkt->getName() << " to port AM_Sap$o\n";
        send(pkt,amSap_[OUT]);
        break;
        default:
        throw cRuntimeError("LteRlcMux: wrong traffic type %d", lteInfo->getRlcType());
    }
}
            /*
             * Main functions
             */

void LteRlcMux::initialize()
{
    macSap_[IN] = gate("MAC_to_RLC");
    macSap_[OUT] = gate("RLC_to_MAC");
    tmSap_[IN] = gate("TM_Sap$i");
    tmSap_[OUT] = gate("TM_Sap$o");
    umSap_[IN] = gate("UM_Sap$i");
    umSap_[OUT] = gate("UM_Sap$o");
    amSap_[IN] = gate("AM_Sap$i");
    amSap_[OUT] = gate("AM_Sap$o");
}

void LteRlcMux::handleMessage(cMessage* msg)
{
    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LteRlcMux : Received packet " << pkt->getName() <<
    " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();
    if (incoming == macSap_[IN])
    {
        mac2rlc(pkt);
    }
    else
    {
        rlc2mac(pkt);
    }
    return;
}

void LteRlcMux::finish()
{
    // TODO make-finish
}
