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

    size_ = par("packetSize");

    sourceSimbolicAddress = (char*)par("sourceAddress").stringValue();

    destSimbolicAddress = (char*)par("destAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(destSimbolicAddress);

    //testing
    EV << "MEClusterizeApp::initialize - MEAppSimbolicAddress: " << sourceSimbolicAddress << endl;
    EV << "MEClusterizeApp::initialize - UEAppSimbolicAddress " << destSimbolicAddress << " found!" << " [" << destAddress_.str() << "]" << endl;

    // par is configured by UdpManager
    v2vAppName = par("v2vAppName");

    clusterizeConfigSentMsg_ = registerSignal("clusterizeConfigSentMsg");
    clusterizeInfoRcvdMsg_ = registerSignal("clusterizeInfoRcvdMsg");
    clusterizeInfoDelay_ = registerSignal("clusterizeInfoDelay");
}

void MEClusterizeApp::handleMessage(cMessage *msg)
{
    // handle trigger emit statistic (from MEClusterizeService)
    if( strcmp(msg->getName(), "triggerClusterizeConfigStatistics") == 0){

        emit(clusterizeConfigSentMsg_, (long)1);
        delete msg;
        return;
    }

    ClusterizePacket* pkt = check_and_cast<ClusterizePacket*>(msg);
    if (pkt == 0)
        throw cRuntimeError("UEClusterizeApp::handleMessage - \tFATAL! Error when casting to ClusterizePacket");

    // handling INFO_CLUSTERIZE
    if(!strcmp(pkt->getType(), INFO_CLUSTERIZE)){

        ClusterizeInfoPacket* ipkt = check_and_cast<ClusterizeInfoPacket*>(msg);
        handleClusterizeInfo(ipkt);
    }
    // handling CONFIG_CLUSTERIZE
    else if(!strcmp(pkt->getType(), CONFIG_CLUSTERIZE)){

        ClusterizeConfigPacket* cpkt = check_and_cast<ClusterizeConfigPacket*>(msg);
        handleClusterizeConfig(cpkt);
    }
}

void MEClusterizeApp::finish(){

    EV << "MEClusterizeApp::finish - Sending ClusterizeStop to the MEClusterizeService" << endl;

    //
    // informing the MEClusterizeService to cancel data about this MEClusterizeApp instance
    //
    if(gate("mePlatformOut")->isConnected()){
        ClusterizePacket* packet = new ClusterizePacket(STOP_CLUSTERIZE);
        packet->setTimestamp(simTime());
        packet->setByteLength(size_);

        packet->setType(STOP_CLUSTERIZE);

        packet->setSourceAddress(sourceSimbolicAddress);
        packet->setDestinationAddress(destSimbolicAddress);

        send(packet, "mePlatformOut");
    }
}

void MEClusterizeApp::handleClusterizeConfig(ClusterizeConfigPacket* packet){

    EV << "MEClusterizeApp::handleClusterizeConfig - finalize ClusterizeConfigPacket creation" << endl;

    //attaching info to the ClusterizeConfigPacket created by the MEClusterizeService
    packet->setSno(nextSnoConfig_);
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);
    packet->setSourceAddress(sourceSimbolicAddress);
    packet->setDestinationAddress(destSimbolicAddress);

    EV << "MEClusterizeApp::handleClusterizeConfig - Sending ClusterizeConfigPacket SeqNo[" << nextSnoConfig_ << "]\n";

    send(packet, "virtualisationInfrastructureOut");
    nextSnoConfig_++;
}

void MEClusterizeApp::handleClusterizeInfo(ClusterizeInfoPacket *pkt){

    simtime_t delay = simTime()-pkt->getTimestamp();
    EV << "MEClusterizeApp::handleClusterizeInfo - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "] delay: "<< delay << endl;
    //simply forward to the MEClusterizeService
    send(pkt, "mePlatformOut");

    emit(clusterizeInfoRcvdMsg_, (long)1);
    emit(clusterizeInfoDelay_, delay);

    //testing
    EV << "MEClusterizeApp::handleClusterizeInfo position: [" << pkt->getPositionX() << " ; " << pkt->getPositionY() << " ; " << pkt->getPositionZ() << "]" << endl;
    EV << "MEClusterizeApp::handleClusterizeInfo speed: [" << pkt->getSpeedX() << " ; " << pkt->getSpeedY() << " ; " << pkt->getSpeedZ() << "]" << endl;
    EV << "MEClusterizeApp::handleClusterizeInfo AngularPosition: [" << pkt->getAngularPositionA() << " ; " << pkt->getAngularPositionB() << " ; " << pkt->getAngularPositionC() << "]" << endl;
    EV << "MEClusterizeApp::handleClusterizeInfo AngularSpeed: [" << pkt->getAngularSpeedA() << " ; " << pkt->getAngularSpeedB() << " ; " << pkt->getAngularSpeedC() << "]" << endl;
}
