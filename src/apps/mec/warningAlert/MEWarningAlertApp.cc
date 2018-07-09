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

#include "MEWarningAlertApp.h"

Define_Module(MEWarningAlertApp);

void MEWarningAlertApp::initialize(int stage)
{
    EV << "MEWarningAlertApp::initialize - stage " << stage << endl;

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
    EV << "MEWarningAlertApp::initialize - MEWarningAlertApp Symbolic Address: " << meHostSimbolicAddress << endl;
    EV << "MEWarningAlertApp::initialize - UEWarningAlertApp Symbolic Address: " << ueSimbolicAddress <<  " [" << destAddress_.str() << "]" << endl;

}

void MEWarningAlertApp::handleMessage(cMessage *msg)
{
    EV << "MEWarningAlertApp::handleMessage - \n";

    WarningAlertPacket* pkt = check_and_cast<WarningAlertPacket*>(msg);
    if (pkt == 0)
        throw cRuntimeError("MEWarningAlertApp::handleMessage - \tFATAL! Error when casting to WarningAlertPacket");

    if(!strcmp(pkt->getType(), INFO_UEAPP))         handleInfoUEWarningAlertApp(pkt);

    else if(!strcmp(pkt->getType(), INFO_MEAPP))    handleInfoMEWarningAlertApp(pkt);
}

void MEWarningAlertApp::finish(){

    EV << "MEWarningAlertApp::finish - Sending " << STOP_MEAPP << " to the MEIceAlertService" << endl;

    if(gate("mePlatformOut")->isConnected()){

        WarningAlertPacket* packet = new WarningAlertPacket();
        packet->setType(STOP_MEAPP);

        send(packet, "mePlatformOut");
    }
}
void MEWarningAlertApp::handleInfoUEWarningAlertApp(WarningAlertPacket* pkt){

    simtime_t delay = simTime()-pkt->getTimestamp();

    EV << "MEWarningAlertApp::handleInfoUEWarningAlertApp - Received: " << pkt->getType() << " type WarningAlertPacket from " << pkt->getSourceAddress() << ": delay: "<< delay << endl;

    EV << "MEWarningAlertApp::handleInfoUEWarningAlertApp - Upstream " << pkt->getType() << " type WarningAlertPacket to MEIceAlertService \n";
    send(pkt, "mePlatformOut");

    //testing
    EV << "MEWarningAlertApp::handleInfoUEWarningAlertApp position: [" << pkt->getPositionX() << " ; " << pkt->getPositionY() << " ; " << pkt->getPositionZ() << "]" << endl;
}

void MEWarningAlertApp::handleInfoMEWarningAlertApp(WarningAlertPacket* pkt){

    EV << "MEWarningAlertApp::handleInfoMEWarningAlertApp - Finalize creation of " << pkt->getType() << " type WarningAlertPacket" << endl;

    //attaching info to the ClusterizeConfigPacket created by the MEClusterizeService
    pkt->setTimestamp(simTime());
    pkt->setByteLength(size_);
    pkt->setSourceAddress(meHostSimbolicAddress);
    pkt->setDestinationAddress(ueSimbolicAddress);
    pkt->setMEModuleType("lte.apps.mec.iceAlert.MEIcerAlertApp");
    pkt->setMEModuleName("MEWarningAlertApp");

    EV << "MEWarningAlertApp::handleInfoMEWarningAlertApp - Downstream " << pkt->getType() << " type WarningAlertPacket to VirtualisationManager \n";
    send(pkt, "virtualisationInfrastructureOut");
}


