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

#include "UEIceAlertApp.h"

Define_Module(UEIceAlertApp);

UEIceAlertApp::UEIceAlertApp(){
    selfStart_ = NULL;
    selfSender_ = NULL;
    selfStop_ = NULL;
}

UEIceAlertApp::~UEIceAlertApp(){
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfSender_);
    cancelAndDelete(selfStop_);
}

void UEIceAlertApp::initialize(int stage)
{
    EV << "UEIceAlertApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieve parameters
    size_ = par("packetSize");
    period_ = par("period");
    localPort_ = par("localPort");
    destPort_ = par("destPort");
    sourceSimbolicAddress = (char*)getParentModule()->getFullName();
    destSimbolicAddress = (char*)par("destAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(destSimbolicAddress);

    requiredRam = par("requiredRam");
    requiredDisk = par("requiredDisk");
    requiredCpu = par("requiredCpu").doubleValue();

    //bindign socket UDP
    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort_);

    //retrieving car cModule
    ue = this->getParentModule();

    //retrieving mobility module
    cModule *temp = getParentModule()->getSubmodule("mobility");
    if(temp != NULL){
        mobility = check_and_cast<inet::IMobility*>(temp);
    }
    else {
        EV << "UEIceAlertApp::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;
        throw cRuntimeError("UEIceAlertApp::initialize - \tWARNING: Mobility module NOT FOUND!");
    }

    //initializing the autoscheduling messages
    selfSender_ = new cMessage("selfSender");
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");

    //starting UEIceAlertApp
    simtime_t startTime = par("startTime");
    EV << "UEIceAlertApp::initialize - starting sendStartMEIceAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UEIceAlertApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;
    EV << "UEIceAlertApp::initialize - destAddress: " << destSimbolicAddress << " [" << destAddress_.str()  <<"]"<< endl;
    EV << "UEIceAlertApp::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
}

void UEIceAlertApp::handleMessage(cMessage *msg)
{
    EV << "UEIceAlertApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))      sendInfoUEInceAlertApp();

        else if(!strcmp(msg->getName(), "selfStart"))   sendStartMEIceAlertApp();

        else if(!strcmp(msg->getName(), "selfStop"))    sendStopMEIceAlertApp();

        else    throw cRuntimeError("UEIceAlertApp::handleMessage - \tWARNING: Unrecognized self message");
    }
    // Receiver Side
    else{

        MEAppPacket* mePkt = check_and_cast<MEAppPacket*>(msg);
        if (mePkt == 0) throw cRuntimeError("UEIceAlertApp::handleMessage - \tFATAL! Error when casting to MEAppPacket");

        if( !strcmp(mePkt->getType(), ACK_START_MEAPP) )    handleAckStartMEIceAlertApp(mePkt);

        else if(!strcmp(mePkt->getType(), ACK_STOP_MEAPP))  handleAckStopMEIceAlertApp(mePkt);

        else if(!strcmp(mePkt->getType(), INFO_MEAPP))      handleInfoMEIcerAlertApp(mePkt);

        delete msg;
    }
}

void UEIceAlertApp::finish(){

    // ensuring there is no selfStop_ scheduled!
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
}
/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UEIceAlertApp::sendStartMEIceAlertApp(){
    EV << "UEIceAlertApp::sendStartMEIceAlertApp - Sending " << START_MEAPP << " type IceAlertPacket\n";

    IceAlertPacket* packet = new IceAlertPacket();

    //instantiation requirements and info
    packet->setType(START_MEAPP);
    packet->setMEModuleType("lte.apps.mec.iceAlert.MEIceAlertApp");
    packet->setMEModuleName("MEIceAlertApp");
    packet->setRequiredService("MEIceAlertService");
    packet->setRequiredRam(requiredRam);
    packet->setRequiredDisk(requiredDisk);
    packet->setRequiredCpu(requiredCpu);

    //connection info
    packet->setSourceAddress(sourceSimbolicAddress);
    packet->setDestinationAddress(destSimbolicAddress);

    //identification info
    packet->setUeAppID(getId());

    //other info
    packet->setUeOmnetID(ue->getId());
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);

    socket.sendTo(packet, destAddress_, destPort_);

    //rescheduling
    scheduleAt(simTime() + period_, selfStart_);
}
void UEIceAlertApp::sendInfoUEInceAlertApp(){
    EV << "UEIceAlertApp::sendInfoUEInceAlertApp - Sending " << INFO_UEAPP <<" type IceAlertPacket\n";

    position = mobility->getCurrentPosition();

    IceAlertPacket* packet = new IceAlertPacket();

    //forwarding requirements and info
    packet->setType(INFO_UEAPP);
    packet->setMEModuleName("MEIceAlertApp");
    packet->setRequiredService("MEIceAlertService");

    //connection info
    packet->setSourceAddress(sourceSimbolicAddress);
    packet->setDestinationAddress(destSimbolicAddress);

    //identification info
    packet->setUeAppID(getId());

    //other info
    packet->setUeOmnetID(ue->getId());
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);

    //INFO_UEAPP info
    packet->setPositionX(position.x);
    packet->setPositionY(position.y);
    packet->setPositionZ(position.z);

    socket.sendTo(packet, destAddress_, destPort_);

    //rescheduling
    scheduleAt(simTime() + period_, selfSender_);
}
void UEIceAlertApp::sendStopMEIceAlertApp(){

    EV << "UEIceAlertApp::sendStopMEIceAlertApp - Sending " << STOP_MEAPP <<" type IceAlertPacket\n";

    IceAlertPacket* packet = new IceAlertPacket();

    //termination requirements and info
    packet->setType(STOP_MEAPP);
    packet->setMEModuleType("lte.apps.mec.iceAlert.MEIcerAlertApp");
    packet->setMEModuleName("MEIceAlertApp");
    packet->setRequiredService("MEIceAlertService");
    packet->setRequiredRam(requiredRam);
    packet->setRequiredDisk(requiredDisk);
    packet->setRequiredCpu(requiredCpu);

    //connection info
    packet->setSourceAddress(sourceSimbolicAddress);
    packet->setDestinationAddress(destSimbolicAddress);

    //identification info
    packet->setUeAppID(getId());

    //other info
    packet->setUeOmnetID(ue->getId());
    packet->setTimestamp(simTime());
    packet->setByteLength(size_);

    socket.sendTo(packet, destAddress_, destPort_);

    // stopping the info updating
    if(selfSender_->isScheduled())
        cancelEvent(selfSender_);

    //rescheduling
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + period_, selfStop_);
}

/*
 * ---------------------------------------------Receiver Side------------------------------------------
 */
void UEIceAlertApp::handleAckStartMEIceAlertApp(MEAppPacket* pkt){

    EV << "UEIceAlertApp::handleAckStartMEIceAlertApp - Received " << pkt->getType() << " type IceAlertPacket from: "<< pkt->getSourceAddress() << endl;

    cancelEvent(selfStart_);

    //scheduling sendInfoUEInceAlertApp()
    if(!selfSender_->isScheduled()){

        simtime_t startTime = par("startTime");
        scheduleAt( simTime() + period_, selfSender_);
        EV << "UEIceAlertApp::handleAckStartMEIceAlertApp - Starting traffic in " << period_ << " seconds " << endl;
    }

    //scheduling sendStopMEIceAlertApp()
    if(!selfStop_->isScheduled()){
        simtime_t  stopTime = par("stopTime");
        scheduleAt(simTime() + stopTime, selfStop_);
        EV << "UEIceAlertApp::handleAckStartMEIceAlertApp - Starting sendClusterizeStopPacket() in " << stopTime << " seconds " << endl;
    }
}
void UEIceAlertApp::handleInfoMEIcerAlertApp(MEAppPacket* pkt){

    EV << "UEIceAlertApp::handleInfoMEIcerAlertApp - Received " << pkt->getType() << " type IceAlertPacket from: "<< pkt->getSourceAddress() << endl;

    IceAlertPacket* packet = check_and_cast<IceAlertPacket*>(pkt);

    //updating runtime color of the car icon background
    if(packet->getDanger()){

        EV << "UEIceAlertApp::handleInfoMEIcerAlertApp - Ice Alert Detected: DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "red");
    }
    else{

        EV << "UEIceAlertApp::handleInfoMEIcerAlertApp - Ice Alert Detected: NO DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "green");
    }
}
void UEIceAlertApp::handleAckStopMEIceAlertApp(MEAppPacket* pkt){

    EV << "UEIceAlertApp::handleAckStopMEIceAlertApp - Received " << pkt->getType() << " type IceAlertPacket from: "<< pkt->getSourceAddress() << endl;

    //updating runtime color of the car icon background
    ue->getDisplayString().setTagArg("i",1, "white");

    cancelEvent(selfStop_);
}
