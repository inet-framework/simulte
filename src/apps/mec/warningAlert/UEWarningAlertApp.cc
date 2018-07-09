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

#include "UEWarningAlertApp.h"

Define_Module(UEWarningAlertApp);

UEWarningAlertApp::UEWarningAlertApp(){
    selfStart_ = NULL;
    selfSender_ = NULL;
    selfStop_ = NULL;
}

UEWarningAlertApp::~UEWarningAlertApp(){
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfSender_);
    cancelAndDelete(selfStop_);
}

void UEWarningAlertApp::initialize(int stage)
{
    EV << "UEWarningAlertApp::initialize - stage " << stage << endl;
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

    //binding socket UDP
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
        EV << "UEWarningAlertApp::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;
        throw cRuntimeError("UEWarningAlertApp::initialize - \tWARNING: Mobility module NOT FOUND!");
    }

    //initializing the auto-scheduling messages
    selfSender_ = new cMessage("selfSender");
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");

    //starting UEWarningAlertApp
    simtime_t startTime = par("startTime");
    EV << "UEWarningAlertApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);

    //testing
    EV << "UEWarningAlertApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;
    EV << "UEWarningAlertApp::initialize - destAddress: " << destSimbolicAddress << " [" << destAddress_.str()  <<"]"<< endl;
    EV << "UEWarningAlertApp::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
}

void UEWarningAlertApp::handleMessage(cMessage *msg)
{
    EV << "UEWarningAlertApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))      sendInfoUEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfStart"))   sendStartMEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfStop"))    sendStopMEWarningAlertApp();

        else    throw cRuntimeError("UEWarningAlertApp::handleMessage - \tWARNING: Unrecognized self message");
    }
    // Receiver Side
    else{

        MEAppPacket* mePkt = check_and_cast<MEAppPacket*>(msg);
        if (mePkt == 0) throw cRuntimeError("UEWarningAlertApp::handleMessage - \tFATAL! Error when casting to MEAppPacket");

        if( !strcmp(mePkt->getType(), ACK_START_MEAPP) )    handleAckStartMEWarningAlertApp(mePkt);

        else if(!strcmp(mePkt->getType(), ACK_STOP_MEAPP))  handleAckStopMEWarningAlertApp(mePkt);

        else if(!strcmp(mePkt->getType(), INFO_MEAPP))      handleInfoMEWarningAlertApp(mePkt);

        delete msg;
    }
}

void UEWarningAlertApp::finish()
{
    // ensuring there is no selfStop_ scheduled!
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
}
/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UEWarningAlertApp::sendStartMEWarningAlertApp()
{
    EV << "UEWarningAlertApp::sendStartMEWarningAlertApp - Sending " << START_MEAPP << " type WarningAlertPacket\n";

    WarningAlertPacket* packet = new WarningAlertPacket();

    //instantiation requirements and info
    packet->setType(START_MEAPP);
    packet->setMEModuleType("lte.apps.mec.warningAlert.MEWarningAlertApp");
    packet->setMEModuleName("MEWarningAlertApp");
    packet->setRequiredService("MEWarningAlertService");
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
void UEWarningAlertApp::sendInfoUEWarningAlertApp()
{
    EV << "UEWarningAlertApp::sendInfoUEInceAlertApp - Sending " << INFO_UEAPP <<" type WarningAlertPacket\n";

    position = mobility->getCurrentPosition();

    WarningAlertPacket* packet = new WarningAlertPacket();

    //forwarding requirements and info
    packet->setType(INFO_UEAPP);
    packet->setMEModuleName("MEWarningAlertApp");
    packet->setRequiredService("MEWarningAlertService");

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
void UEWarningAlertApp::sendStopMEWarningAlertApp()
{
    EV << "UEWarningAlertApp::sendStopMEWarningAlertApp - Sending " << STOP_MEAPP <<" type WarningAlertPacket\n";

    WarningAlertPacket* packet = new WarningAlertPacket();

    //termination requirements and info
    packet->setType(STOP_MEAPP);
    packet->setMEModuleType("lte.apps.mec.warningAlert.MEWarningrAlertApp");
    packet->setMEModuleName("MEWarningAlertApp");
    packet->setRequiredService("MEWarningAlertService");
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
void UEWarningAlertApp::handleAckStartMEWarningAlertApp(MEAppPacket* pkt)
{
    EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket from: "<< pkt->getSourceAddress() << endl;

    cancelEvent(selfStart_);

    //scheduling sendInfoUEInceAlertApp()
    if(!selfSender_->isScheduled()){

        simtime_t startTime = par("startTime");
        scheduleAt( simTime() + period_, selfSender_);
        EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - Starting traffic in " << period_ << " seconds " << endl;
    }

    //scheduling sendStopMEWarningAlertApp()
    if(!selfStop_->isScheduled()){
        simtime_t  stopTime = par("stopTime");
        scheduleAt(simTime() + stopTime, selfStop_);
        EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - Starting sendStopMEWarningAlertApp() in " << stopTime << " seconds " << endl;
    }
}
void UEWarningAlertApp::handleInfoMEWarningAlertApp(MEAppPacket* pkt)
{
    EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Received " << pkt->getType() << " type WarningAlertPacket from: "<< pkt->getSourceAddress() << endl;

    WarningAlertPacket* packet = check_and_cast<WarningAlertPacket*>(pkt);

    //updating runtime color of the car icon background
    if(packet->getDanger()){

        EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "red");
    }
    else{

        EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: NO DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "green");
    }
}
void UEWarningAlertApp::handleAckStopMEWarningAlertApp(MEAppPacket* pkt)
{
    EV << "UEWarningAlertApp::handleAckStopMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket from: "<< pkt->getSourceAddress() << endl;

    //updating runtime color of the car icon background
    ue->getDisplayString().setTagArg("i",1, "white");

    cancelEvent(selfStop_);
}
