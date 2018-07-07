//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//  @author Angelo Buono
//

#include "MEIceAlertApp.h"

Define_Module(MEIceAlertApp);

void MEIceAlertApp::initialize(int stage)
{
    EV << "MEIceAlertApp::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieving parameters
    size_ = par("packetSize");
    ueSimbolicAddress = (char*)par("ueSimbolicAddress").stringValue();
    meHostSimbolicAddress = (char*)par("meHostSimbolicAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(ueSimbolicAddress);

    //testing
    EV << "MEIceAlertApp::initialize - MEIceAlertApp Symbolic Address: " << meHostSimbolicAddress << endl;
    EV << "MEIceAlertApp::initialize - UEIceAlertApp Symbolic Address: " << ueSimbolicAddress <<  " [" << destAddress_.str() << "]" << endl;

}

void MEIceAlertApp::handleMessage(cMessage *msg)
{
    EV << "MEIceAlertApp::handleMessage - \n";

    IceAlertPacket* pkt = check_and_cast<IceAlertPacket*>(msg);
    if (pkt == 0)
        throw cRuntimeError("MEIceAlertApp::handleMessage - \tFATAL! Error when casting to IceAlertPacket");

    if(!strcmp(pkt->getType(), INFO_UEAPP))         handleInfoUEIceAlertApp(pkt);

    else if(!strcmp(pkt->getType(), INFO_MEAPP))    handleInfoMEIceAlertApp(pkt);
}

void MEIceAlertApp::finish(){

    EV << "MEIceAlertApp::finish - Sending " << STOP_MEAPP << " to the MEIceAlertService" << endl;

    if(gate("mePlatformOut")->isConnected()){

        IceAlertPacket* packet = new IceAlertPacket();
        packet->setType(STOP_MEAPP);

        send(packet, "mePlatformOut");
    }
}
void MEIceAlertApp::handleInfoUEIceAlertApp(IceAlertPacket* pkt){

    simtime_t delay = simTime()-pkt->getTimestamp();

    EV << "MEIceAlertApp::handleInfoUEIceAlertApp - Received: " << pkt->getType() << " type IceAlertPacket from " << pkt->getSourceAddress() << ": delay: "<< delay << endl;

    EV << "MEIceAlertApp::handleInfoUEIceAlertApp - Upstream " << pkt->getType() << " type IceAlertPacket to MEIceAlertService \n";
    send(pkt, "mePlatformOut");

    //testing
    EV << "MEIceAlertApp::handleInfoUEIceAlertApp position: [" << pkt->getPositionX() << " ; " << pkt->getPositionY() << " ; " << pkt->getPositionZ() << "]" << endl;
}

void MEIceAlertApp::handleInfoMEIceAlertApp(IceAlertPacket* pkt){

    EV << "MEIceAlertApp::handleInfoMEIceAlertApp - Finalize creation of " << pkt->getType() << " type IceAlertPacket" << endl;

    //attaching info to the ClusterizeConfigPacket created by the MEClusterizeService
    pkt->setTimestamp(simTime());
    pkt->setByteLength(size_);
    pkt->setSourceAddress(meHostSimbolicAddress);
    pkt->setDestinationAddress(ueSimbolicAddress);
    pkt->setMEModuleType("lte.apps.mec.iceAlert.MEIcerAlertApp");
    pkt->setMEModuleName("MEIceAlertApp");

    EV << "MEIceAlertApp::handleInfoMEIceAlertApp - Downstream " << pkt->getType() << " type IceAlertPacket to VirtualisationManager \n";
    send(pkt, "virtualisationInfrastructureOut");
}


