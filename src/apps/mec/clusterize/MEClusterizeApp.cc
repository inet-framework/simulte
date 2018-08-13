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

#include <cmath>
#include "MEClusterizeApp.h"

Define_Module(MEClusterizeApp);

MEClusterizeApp::MEClusterizeApp()
{
    nextSnoConfig_= 0;
}

MEClusterizeApp::~MEClusterizeApp()
{
}

void MEClusterizeApp::initialize(int stage)
{
    EV << "MEClusterizeApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    //--------------------------------------
    //packet informations
    size_ = par("packetSize");
    //--------------------------------------
    //communication with ME Host
    ueSimbolicAddress = (char*)par("ueSimbolicAddress").stringValue();
    meHostSimbolicAddress = (char*)par("meHostSimbolicAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(ueSimbolicAddress);
    //--------------------------------------
    //statistics
    clusterizeConfigSentMsg_ = registerSignal("clusterizeConfigSentMsg");
    clusterizeInfoRcvdMsg_ = registerSignal("clusterizeInfoRcvdMsg");
    clusterizeInfoDelay_ = registerSignal("clusterizeInfoDelay");

    //testing
    EV << "MEClusterizeApp::initialize - MEClusterizeApp SymbolicAddress: " << meHostSimbolicAddress << endl;
    EV << "MEClusterizeApp::initialize - UEClusterizeApp SymbolicAddress: " << ueSimbolicAddress << " [" << destAddress_.str() << "]" << endl;
}

void MEClusterizeApp::handleMessage(cMessage *msg)
{
    EV << "MEClusterizeApp::handleMessage -  message received\n";

    //handling the triggering to emit statistics for txMode V2V_UNICAST or V2V_MULTICAST (from MEClusterizeService)
    if( strcmp(msg->getName(), "triggerClusterizeConfigStatistics") == 0){

        emit(clusterizeConfigSentMsg_, (long)1);    //even if the message is sent only to the Leader and then propagated on the sidelink
        delete msg;
        return;
    }

    ClusterizePacket* pkt = check_and_cast<ClusterizePacket*>(msg);
    if (pkt == 0)
        throw cRuntimeError("UEClusterizeApp::handleMessage - \tFATAL! Error when casting to ClusterizePacket");

    if(!strcmp(pkt->getType(), INFO_UEAPP))
    {
        ClusterizeInfoPacket* ipkt = check_and_cast<ClusterizeInfoPacket*>(msg);
        handleClusterizeInfo(ipkt);
    }
    else if(!strcmp(pkt->getType(), INFO_MEAPP))
    {
        ClusterizeConfigPacket* cpkt = check_and_cast<ClusterizeConfigPacket*>(msg);
        handleClusterizeConfig(cpkt);
    }
}

void MEClusterizeApp::finish(){

    EV << "MEClusterizeApp::finish - Sending ClusterizeStop to the MEClusterizeService" << endl;

    //forwarding to the MEClusterizeService
    if(gate("mePlatformOut")->isConnected())
    {
        ClusterizePacket* packet = ClusterizePacketBuilder().buildClusterizePacket(STOP_MEAPP, 0, simTime(), size_, 0, meHostSimbolicAddress, ueSimbolicAddress);
        send(packet, "mePlatformOut");
    }
}

void MEClusterizeApp::handleClusterizeConfig(ClusterizeConfigPacket* packet){

    EV << "MEClusterizeApp::handleClusterizeConfig - finalize " << INFO_MEAPP << " ClusterizeConfigPacket creation" << endl;

    //attaching informations to the INFO_MEAPP packet created by the MEClusterizeService
    packet->setSno(nextSnoConfig_);
    packet->setTimestamp(simTime());
    //dynamically setting packet size:
    //1 array of double (8byte) + 1 array of string (4byte) + 30 fixed bytes
    size_ = 30 + packet->getClusterListArraySize()*12;
    packet->setByteLength(size_);
    packet->setSourceAddress(meHostSimbolicAddress);
    packet->setDestinationAddress(ueSimbolicAddress);
    packet->setMEModuleName(ME_CLUSTERIZE_APP_MODULE_NAME);
    packet->setMEModuleType(ME_CLUSTERIZE_APP_MODULE_TYPE);
    //forwarding to the VirtualistionManager
    send(packet, "virtualisationInfrastructureOut");
    nextSnoConfig_++;

    EV << "MEClusterizeApp::handleClusterizeConfig - Sending " << INFO_MEAPP << " ClusterizeConfigPacket SeqNo[" << nextSnoConfig_ << "] size: "<< size_ <<"\n";
}

void MEClusterizeApp::handleClusterizeInfo(ClusterizeInfoPacket *pkt){

    simtime_t delay = simTime()-pkt->getTimestamp();

    EV << "MEClusterizeApp::handleClusterizeInfo - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "] delay: "<< delay << endl;

    //forwarding to the MEClusterizeService
    send(pkt, "mePlatformOut");

    //emit statistics
    emit(clusterizeInfoRcvdMsg_, (long)1);
    emit(clusterizeInfoDelay_, delay);

    //testing
    EV << "MEClusterizeApp::handleClusterizeInfo position: [" << pkt->getPositionX() << " ; " << pkt->getPositionY() << " ; " << pkt->getPositionZ() << "]" << endl;
    EV << "MEClusterizeApp::handleClusterizeInfo speed: [" << pkt->getSpeedX() << " ; " << pkt->getSpeedY() << " ; " << pkt->getSpeedZ() << "]" << endl;
    EV << "MEClusterizeApp::handleClusterizeInfo AngularPosition: [" << pkt->getAngularPositionA() << " ; " << pkt->getAngularPositionB() << " ; " << pkt->getAngularPositionC() << "]" << endl;
    EV << "MEClusterizeApp::handleClusterizeInfo AngularSpeed: [" << pkt->getAngularSpeedA() << " ; " << pkt->getAngularSpeedB() << " ; " << pkt->getAngularSpeedC() << "]" << endl;
}
