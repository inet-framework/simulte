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


#include "V2vClusterSpeedControlApp.h"

Define_Module(V2vClusterSpeedControlApp);

V2vClusterSpeedControlApp::V2vClusterSpeedControlApp()
{
}

V2vClusterSpeedControlApp::~V2vClusterSpeedControlApp()
{
}


void V2vClusterSpeedControlApp::initialize(int stage)
{
    EV << "V2vClusterSpeedControlApp::initialize - stage " << stage << endl;

    V2vClusterBaseApp::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    car = this->getParentModule();

    //getting mobility module
    cModule *temp = getParentModule()->getSubmodule("mobility");
    if(temp != NULL){
        mobility = check_and_cast<Veins::VeinsInetMobility*>(temp);
       EV << "V2vClusterSpeedControlApp::initialize - Mobility module found!" << endl;
       veinsManager = Veins::VeinsInetManagerAccess().get();
       traci = veinsManager->getCommandInterface();
    }
    else
        EV << "V2vClusterSpeedControlApp::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;


    v2vClusterSpeedControlSentMsg_ = registerSignal("v2vClusterSpeedControlSentMsg");
    v2vClusterSpeedControlDelay_ = registerSignal("v2vClusterSpeedControlDelay");
    v2vClusterSpeedControlRcvdMsg_ = registerSignal("v2vClusterSpeedControlRcvdMsg");
}

void V2vClusterSpeedControlApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendV2vSpeedControlPacket(NULL);
        else
            throw cRuntimeError("V2vClusterSpeedControlApp::handleMessage - Unrecognized self message");
    }
    else{

        V2vSpeedControlPacket* pkt = check_and_cast<V2vSpeedControlPacket*>(msg);

        if (pkt == 0)
            throw cRuntimeError("V2vClusterSpeedControlApp::handleMessage - FATAL! Error when casting to V2vClusterSpeedPacket");
        else
            handleV2vSpeedControlPacket(pkt);
    }
}

void V2vClusterSpeedControlApp::sendV2vSpeedControlPacket(V2vSpeedControlPacket* rcvdPkt)
{
    //if vehicle is null:
    //call getVehicleInterface()
                                                                                                                    //TODO
                                                                                                            //See V2vClusterAlertApp.....       -->     build a virtual V2vApp class!

    if(!selfSender_->isScheduled())
        scheduleAt(simTime() + period_, selfSender_);
}

void V2vClusterSpeedControlApp::handleV2vSpeedControlPacket(V2vSpeedControlPacket* pkt){

    // emit statistics
    simtime_t delay = simTime() - pkt->getTimestamp();
    emit(v2vClusterSpeedControlDelay_, delay);
    emit(v2vClusterSpeedControlRcvdMsg_, (long)1);

    EV << "V2vClusterSpeedControlApp::handleV2vSpeedControlPacket - Received: " << pkt->getName() << " from " << pkt->getForwarderAddress() << " started by " << pkt->getSourceAddress() << endl;
    EV << "V2vClusterSpeedControlApp::handleV2vSpeedControlPacket - Packet received: SeqNo[" << pkt->getSno() << "] Delay[" << delay << "] (from starting Car)" << endl;

    // forwarding the alertPacket behind!
    sendV2vSpeedControlPacket(pkt);
}


void V2vClusterSpeedControlApp::getVehicleInterface(){

    std::map<std::string, cModule*> cars = veinsManager->getManagedHosts();
    std::map<std::string, cModule*>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++)
        if(it->second == car)
            carID = it->first;

    EV << "V2vClusterSpeedControlApp::getVehicleInterface - carID: " << carID << endl;

    traciVehicle = new Veins::TraCICommandInterface::Vehicle(traci->vehicle(carID));

    //  speed mode:
    //  bit0: Regard safe speed
    //  bit1: Regard maximum acceleration
    //  bit2: Regard maximum deceleration
    //  bit3: Regard right of way at intersections
    //  bit4: Brake hard to avoid passing a red light
    traciVehicle->setSpeedMode(0x00);
}
