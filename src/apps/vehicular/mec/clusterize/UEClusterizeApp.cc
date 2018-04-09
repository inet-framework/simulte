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
#include "UEClusterizeApp.h"

Define_Module(UEClusterizeApp);

UEClusterizeApp::UEClusterizeApp()
{
    selfStart_ = NULL;
    selfSender_ = NULL;
    selfStop_ = NULL;
    nextSnoStart_ = 0;
    nextSnoInfo_ = 0;
    nextSnoStop_ = 0;
}

UEClusterizeApp::~UEClusterizeApp()
{

    cancelAndDelete(selfStart_);
    cancelAndDelete(selfSender_);
}


void UEClusterizeApp::initialize(int stage)
{
    EV << "UEClusterizeApp::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    size_ = par("packetSize");
    period_ = par("period");

    localPort_ = par("localPort");
    destPort_ = par("destPort");

    sourceSimbolicAddress = (char*)getParentModule()->getFullName();
    EV << "UEClusterizeApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;

    destSimbolicAddress = (char*)par("destAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(destSimbolicAddress);
    EV << "UEClusterizeApp::initialize - destAddress: " << destSimbolicAddress << " [" << destAddress_.str()  <<"]"<< endl;

    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort_);
    EV << "UEClusterizeApp::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;


    selfSender_ = new cMessage("selfSender");
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");

    v2vApp = getParentModule()->getModuleByPath(par("v2vAppPath"));
    if(v2vApp == NULL){
        EV << "UEClusterizeApp::initialize - ERROR getting v2vApp module!" << endl;
        throw cRuntimeError("UEClusterizeApp::initialize - \tFATAL! Error when getting v2vApp!");
    }

    v2vAppName = par("v2vAppName");


    //getting mobility module
    cModule *temp = getParentModule()->getSubmodule("mobility");
    if(temp != NULL){
        mobility = check_and_cast<inet::IMobility*>(temp);
       EV << "UEClusterizeApp::initialize - Mobility module found!" << endl;
    }
    else
        EV << "UEClusterizeApp::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;


    //statics signals
    clusterizeInfoSentMsg_ = registerSignal("clusterizeInfoSentMsg");
    clusterizeConfigRcvdMsg_ = registerSignal("clusterizeConfigRcvdMsg");

    simtime_t startTime = par("startTime");
    EV << "\t starting sendClusterizeStartPacket() in " << startTime << " seconds " << endl;
    // starting sendClusterizeStartPacket() to initialize the ME App on the ME Host
    scheduleAt(simTime() + startTime, selfStart_);

    simtime_t  stopTime = par("stopTime");
    EV << "\t starting sendClusterizeStopPacket() in " << stopTime << " seconds " << endl;
    // starting sendClusterizeSopPacket() to terminate the ME App on the ME Host
    scheduleAt(simTime() + stopTime, selfStop_);
}


void UEClusterizeApp::handleMessage(cMessage *msg)
{
    /*
     * Sender Side
     */
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendClusterizeInfoPacket();
        else if(!strcmp(msg->getName(), "selfStart"))
            sendClusterizeStartPacket();
        else if(!strcmp(msg->getName(), "selfStop"))
            sendClusterizeStopPacket();
        else
            throw cRuntimeError("UEClusterizeApp::handleMessage - \tWARNING: Unrecognized self message");
    }
    /*
     * Receiver Side
     */
    else{

        ClusterizePacket* pkt = check_and_cast<ClusterizePacket*>(msg);
        if (pkt == 0)
            throw cRuntimeError("UEClusterizeApp::handleMessage - \tFATAL! Error when casting to ClusterizePacket");

        //ACK_START_CLUSTERIZE_PACKET
        if(!strcmp(pkt->getType(), ACK_START_CLUSTERIZE)){

                handleClusterizeAckStart(pkt);
        }
        //ACK_STOP_CLUSTERIZE_PACKET
        else if(!strcmp(pkt->getType(), ACK_STOP_CLUSTERIZE) ){

                    handleClusterizeAckStop(pkt);
            }
        //CONFIG_CLUSTERIZE_PACKET
        else if(!strcmp(pkt->getType(), CONFIG_CLUSTERIZE)){

            ClusterizeConfigPacket* cpkt = check_and_cast<ClusterizeConfigPacket*>(msg);
            handleClusterizeConfig(cpkt);
        }

        delete msg;
    }
}

void UEClusterizeApp::finish(){

    // ensuring there is no StopClusterize scheduled!
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);

    // when leaving the network --> terminate the correspondent MEClusterizeApp!
    sendClusterizeStopPacket();
}

/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UEClusterizeApp::sendClusterizeStartPacket()
{
    EV << "UEClusterizeApp::sendClusterizeStartPacket - Sending message SeqNo[" << nextSnoStart_ << "]\n";

    ClusterizePacket* packet = new ClusterizePacket(START_CLUSTERIZE);
    packet->setSno(nextSnoStart_);
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);

    packet->setType(START_CLUSTERIZE);

    packet->setV2vAppName(v2vAppName);
    packet->setSourceAddress(sourceSimbolicAddress);
    packet->setDestinationAddress(destSimbolicAddress);

    socket.sendTo(packet, destAddress_, destPort_);
    nextSnoStart_++;

    // trying to instantiate the MECLusterizeApp until receiving the ack
    scheduleAt(simTime() + period_, selfStart_);
}

void UEClusterizeApp::sendClusterizeInfoPacket()
{
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Sending message SeqNo[" << nextSnoInfo_ << "]\n";

    ClusterizeInfoPacket* packet = new ClusterizeInfoPacket(INFO_CLUSTERIZE);
    packet->setSno(nextSnoInfo_);
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);

    packet->setType(INFO_CLUSTERIZE);

    packet->setV2vAppName(v2vAppName);
    packet->setSourceAddress(sourceSimbolicAddress);
    packet->setDestinationAddress(destSimbolicAddress);

    //position = m and speed = m/s..    --> setted in the simulation "*.rou.xml" config files for SUMO
    //w.r.t. top-left edge in the Network with Coord [0 ; 0]
    if(mobility != NULL){
        position = mobility->getCurrentPosition();
        speed = mobility->getCurrentSpeed();
        angularPosition = mobility->getCurrentAngularPosition();
        angularSpeed = mobility->getCurrentAngularSpeed();
    }

    packet->setPositionX(position.x);
    packet->setPositionY(position.y);
    packet->setPositionZ(position.z);
    packet->setSpeedX(speed.x);
    packet->setSpeedY(speed.y);
    packet->setSpeedZ(speed.z);
    packet->setAngularPositionA(angularPosition.alpha);
    packet->setAngularPositionB(angularPosition.beta);
    packet->setAngularPositionC(angularPosition.gamma);
    packet->setAngularSpeedA(angularSpeed.alpha);
    packet->setAngularSpeedB(angularSpeed.beta);
    packet->setAngularSpeedC(angularSpeed.gamma);

    //testing
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Position: [" << position.x << " ; " << position.y << " ; " << position.z << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Speed: [" << speed.x << " ; " << speed.y << " ; " << speed.z << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - AngularPosition: [" << angularPosition.alpha << " ; " << angularPosition.beta << " ; " << angularPosition.gamma << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - AngularSpeed: [" << angularSpeed.alpha << " ; " << angularSpeed.beta << " ; " << angularSpeed.gamma << "]" << endl ;

    socket.sendTo(packet, destAddress_, destPort_);
    nextSnoInfo_++;

    emit(clusterizeInfoSentMsg_, (long)1);

    scheduleAt(simTime() + period_, selfSender_);
}

void UEClusterizeApp::sendClusterizeStopPacket()
{
    EV << "UEClusterizeApp::sendClusterizeStopPacket - Sending message SeqNo[" << nextSnoStop_ << "]\n";

    ClusterizePacket* packet = new ClusterizePacket(STOP_CLUSTERIZE);
    packet->setSno(nextSnoStop_);
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);

    packet->setType(STOP_CLUSTERIZE);

    packet->setV2vAppName(v2vAppName);
    packet->setSourceAddress(sourceSimbolicAddress);
    packet->setDestinationAddress(destSimbolicAddress);

    socket.sendTo(packet, destAddress_, destPort_);
    nextSnoStop_++;

    // stopping the info updating because the MEClusterizeApp is terminated
    if(selfSender_->isScheduled())
        cancelEvent(selfSender_);

    // trying to terminate the MECLusterizeApp until receiving the ack
    scheduleAt(simTime() + period_, selfStop_);
}

/*
 * -----------------------------------------------Receiver Side------------------------------------------
 */
void UEClusterizeApp::handleClusterizeAckStart(ClusterizePacket* pkt){

    EV << "UEClusterizeApp::handleClusterizeAckStart - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]" << endl;

    cancelEvent(selfStart_);

    simtime_t startTime = par("startTime");

    // starting sending local-info updates
    if(!selfSender_->isScheduled()){
        //check to avoid multiple scheduling by receiving duplicated acks!
        scheduleAt(simTime() + startTime, selfSender_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void UEClusterizeApp::handleClusterizeAckStop(ClusterizePacket* pkt){

    EV << "UEClusterizeApp::handleClusterizeAckStop - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]" << endl;

    cancelEvent(selfStop_);
    //this->callFinish();
    //this->deleteModule();

    //Stop the v2vApp
    if(v2vApp != NULL){
        v2vApp->callFinish();
        //v2vApp->deleteModule();
    }
}

void UEClusterizeApp::handleClusterizeConfig(ClusterizeConfigPacket* pkt){

    EV << "UEClusterizeApp::handleClusterizeConfig - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]" << endl;

    //configure the Cluster parameters for V2vAlertSender IUDPApp
    if(v2vApp != NULL && !strcmp(v2vAppName, "V2vAlertChainApp")){

        EV << "UEClusterizeApp::handleClusterizeConfig - Configuring V2vAlertSender!" << endl;
        (check_and_cast<V2vAlertChainApp*>(v2vApp))->setClusterConfiguration(pkt);
    }

    // emit statistics
    emit(clusterizeConfigRcvdMsg_, (long)1);
}
