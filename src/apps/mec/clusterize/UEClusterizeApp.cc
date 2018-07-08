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
#include "inet/common/ModuleAccess.h"  // for multicast support

Define_Module(UEClusterizeApp);

UEClusterizeApp::UEClusterizeApp()
{
    //auto-scheduling
    selfStart_ = NULL;
    selfSender_ = NULL;
    selfStop_ = NULL;
    stopped = false;
    //sequence numbers
    nextSnoStart_ = 0;
    nextSnoInfo_ = 0;
    nextSnoStop_ = 0;
    //acceleration used (received from MEClusterizeService)
    requiredAcceleration = 0;
}

UEClusterizeApp::~UEClusterizeApp()
{
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfSender_);
    cancelAndDelete(selfStop_);
}

void UEClusterizeApp::initialize(int stage)
{
    EV << "UEClusterizeApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    //--------------------------------------
    //auto-scheduling
    selfSender_ = new cMessage("selfSender");
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");
    period_ = par("period");
    //--------------------------------------
    //packet info
    size_ = par("packetSize");
    //--------------------------------------
    //communication with ME Host
    localPort_ = par("localPort");
    destPort_ = par("destPort");
    mySymbolicAddress = (char*)getParentModule()->getFullName();
    meHostSymbolicAddress = (char*)par("meHostAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(meHostSymbolicAddress);
    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort_);
    //testing
    EV << "UEClusterizeApp::initialize - sourceAddress: " << mySymbolicAddress << " [" << inet::L3AddressResolver().resolve(mySymbolicAddress).str()  <<"]"<< endl;
    EV << "UEClusterizeApp::initialize - destAddress: " << meHostSymbolicAddress << " [" << destAddress_.str()  <<"]"<< endl;
    EV << "UEClusterizeApp::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
    //--------------------------------------
    //multicast communication with cars
    multicastPort_ = par("multicastPort").longValue();
    multicastGoupAddress = (char*) par("multicastGoupAddress").stringValue();
    multicastAddress_ = inet::L3AddressResolver().resolve(multicastGoupAddress);
    multicastSocket.setOutputGate(gate("udpOut"));
    multicastSocket.bind(multicastPort_);
    //testing
    EV << "UEClusterizeApp::initialize - multicastGoupAddress: " << multicastGoupAddress << endl;
    EV << "UEClusterizeApp::initialize - binding to multicast port:" << multicastPort_ << endl;
    //##################################################################################################################
    //multicast group subscription
    inet::IInterfaceTable *ift = inet::getModuleFromPar< inet::IInterfaceTable >(par("interfaceTableModule"), this);
    inet::InterfaceEntry *ie = ift->getInterfaceByName("wlan");
    if (!ie)
        throw cRuntimeError("UEClusterizeApp::initialize - Wrong multicastInterface setting: no interface named wlan");
    inet::MulticastGroupList mgl = ift->collectMulticastGroups();
    multicastSocket.joinLocalMulticastGroups(mgl);
    multicastSocket.setMulticastOutputInterface(ie->getInterfaceId());
    //##################################################################################################################
    //--------------------------------------
    //mobility informations
    car = this->getParentModule();
    cModule *temp = getParentModule()->getSubmodule("mobility");
    if(temp != NULL)
    {
        //veins mobility ?
        if(!strcmp(car->par("mobilityType").stringValue(),"VeinsInetMobility"))
        {
            veins_mobility = check_and_cast<Veins::VeinsInetMobility*>(temp);
            EV << "UEClusterizeApp::initialize - Mobility module found!" << endl;
            //veinsManager = Veins::VeinsInetManagerAccess().get();
            //traci = veinsManager->getCommandInterface();
        }
        else{
            veins_mobility = NULL;
            //inet mobility ?
            if(!strcmp(car->par("mobilityType").stringValue(),"LinearMobility"))
                linear_mobility = check_and_cast<inet::LinearMobility*>(temp);
            else
                linear_mobility = NULL;
        }
    }
    else
        EV << "UEClusterizeApp::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;
    //--------------------------------------
    //collecting multihop statistics
    lteNic = car->getSubmodule("lteNic");
    if(lteNic == NULL)
    {
        EV << "UEClusterizeApp::initialize - ERROR getting lteNic module!" << endl;
        throw cRuntimeError("UEClusterizeApp::initialize - \tFATAL! Error when getting lteNIC!");
    }
    LteMacBase* mac = check_and_cast<LteMacBase*>(lteNic->getSubmodule("mac"));
    macID = mac->getMacNodeId();
    stat_ = check_and_cast<MultihopD2DStatistics*>(getModuleByPath("d2dMultihopStatistics"));
    //--------------------------------------
    //collecting application statistics
    clusterizeInfoSentMsg_ = registerSignal("clusterizeInfoSentMsg");
    clusterizeConfigRcvdMsg_ = registerSignal("clusterizeConfigRcvdMsg");
    clusterizeConfigDelay_ = registerSignal("clusterizeConfigDelay");
    //--------------------------------------
    //starting UEClusterizeApp
    simtime_t startTime = par("startTime");
    scheduleAt(simTime() + startTime, selfStart_);
    EV << "UEClusterizeApp::initialize - \t starting sendClusterizeStartPacket() in " << startTime << " seconds " << endl;
}


void UEClusterizeApp::handleMessage(cMessage *msg)
{
    if(!stopped){
        EV << "UEClusterizeApp::handleMessage -  message received\n";
        /*
         * Sender Side
         */
        if (msg->isSelfMessage())
        {
            if (!strcmp(msg->getName(), "selfSender"))      sendClusterizeInfoPacket();
            else if(!strcmp(msg->getName(), "selfStart"))   sendClusterizeStartPacket();
            else if(!strcmp(msg->getName(), "selfStop"))    sendClusterizeStopPacket();
            else
                throw cRuntimeError("UEClusterizeApp::handleMessage - \tWARNING: Unrecognized self message");
        }
        /*
         * Receiver Side
         */
        else{

            MEAppPacket* mePkt = check_and_cast<MEAppPacket*>(msg);
            if (mePkt == 0)
                throw cRuntimeError("UEClusterizeApp::handleMessage - \tFATAL! Error when casting to MEAppPacket");

            if( !strcmp(mePkt->getType(), ACK_START_MEAPP) )    handleMEAppAckStart(mePkt);
            else if(!strcmp(mePkt->getType(), ACK_STOP_MEAPP))  handleMEAppAckStop(mePkt);
            else if(!strcmp(mePkt->getType(), INFO_MEAPP)){

                ClusterizeConfigPacket* cpkt = check_and_cast<ClusterizeConfigPacket*>(mePkt);
                handleClusterizeConfig(cpkt);
            }
            delete msg;
        }
    }
    else
        delete msg; //delete messages received when UEClusterizeApp is stopped and waiting for ACK_STOP_MEAPP message!
}

void UEClusterizeApp::finish(){

    //ensuring stop auto-scheduling
    if(selfStart_->isScheduled())   cancelEvent(selfStart_);
    if(selfSender_->isScheduled())  cancelEvent(selfSender_);
    if(selfStop_->isScheduled())    cancelEvent(selfStop_);
}

/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UEClusterizeApp::sendClusterizeStartPacket()
{
    EV << "UEClusterizeApp::sendClusterizeStartPacket - Sending message SeqNo[" << nextSnoStart_ << "]\n";
    //creating and sending START_MEAPP ClusterizePacket
    ClusterizePacket* packet = ClusterizePacketBuilder().buildClusterizePacket(START_MEAPP, nextSnoStart_, simTime(), size_, car->getId(), mySymbolicAddress, meHostSymbolicAddress);
    socket.sendTo(packet, destAddress_, destPort_);
    nextSnoStart_++;
    //re-scheduling until ACK_START_MEAPP is received
    scheduleAt(simTime() + period_, selfStart_);
}

void UEClusterizeApp::sendClusterizeInfoPacket()
{
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Sending message SeqNo[" << nextSnoInfo_ << "]\n";

    //position = m and speed = m/s..    --> set in the simulation "*.rou.xml" configuration files for SUMO
    //w.r.t. top-left edge in the Network has Coord [0 ; 0]
    if(veins_mobility == NULL){
        position = linear_mobility->getCurrentPosition();
        speed = linear_mobility->getCurrentSpeed();
        maxSpeed = linear_mobility->getMaxSpeed();
        angularSpeed = linear_mobility->getCurrentAngularSpeed();
        angularPosition = linear_mobility->getCurrentAngularPosition();
        angularPosition.alpha *= -1.0;
        EV << "UEClusterizeApp::sendClusterizeInfoPacket - No VeinsMobility: reverting the angular position!"<< endl;
    }
    else{
        position = veins_mobility->getCurrentPosition();
        speed = veins_mobility->getCurrentSpeed();
        maxSpeed = veins_mobility->getMaxSpeed();
        angularSpeed = veins_mobility->getCurrentAngularSpeed();
        angularPosition = veins_mobility->getCurrentAngularPosition();
        }

    //creating and sending INFO_UEAPP ClusterizeInfoPacket
    ClusterizeInfoPacket* packet = ClusterizePacketBuilder().buildClusterizeInfoPacket(nextSnoInfo_, simTime(), size_, car->getId(), mySymbolicAddress, meHostSymbolicAddress, position, speed, angularPosition, angularSpeed);
    packet->setAcceleration(requiredAcceleration);
    socket.sendTo(packet, destAddress_, destPort_);
    nextSnoInfo_++;

    //emit statistics
    emit(clusterizeInfoSentMsg_, (long)1);

    //re-scheduling
    if(selfSender_->isScheduled())
        cancelEvent(selfSender_);
    scheduleAt(simTime() + period_, selfSender_);

    //testing
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Info: carID " << car->getId() << " sourceAddr " << mySymbolicAddress << " destAddr " << meHostSymbolicAddress << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Position: [" << position.x << " ; " << position.y << " ; " << position.z << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Speed: [" << speed.x << " ; " << speed.y << " ; " << speed.z << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Max Speed: [" << maxSpeed << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - AngularPosition: [" << angularPosition.alpha << " ; " << angularPosition.beta << " ; " << angularPosition.gamma << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - AngularSpeed: [" << angularSpeed.alpha << " ; " << angularSpeed.beta << " ; " << angularSpeed.gamma << "]" << endl ;
}

void UEClusterizeApp::sendClusterizeStopPacket()
{
    EV << "UEClusterizeApp::sendClusterizeStopPacket - Sending message SeqNo[" << nextSnoStop_ << "]\n";

    //creating and sending STOP_MEAPP ClusterizePacket
    ClusterizePacket* packet = ClusterizePacketBuilder().buildClusterizePacket(STOP_MEAPP, nextSnoStop_, simTime(), size_, car->getId(), mySymbolicAddress, meHostSymbolicAddress);
    socket.sendTo(packet, destAddress_, destPort_);
    nextSnoStop_++;

    //updating runtime color of the car icon background
    car->getDisplayString().setTagArg("i",1, "white");

    //stop to accelerate
    linear_mobility->setAcceleration(0);

    //stop the UEClusterizeApp auto-scheduling for sending INFO_UEAPP ClusterizeInfoPacket
    if(selfSender_->isScheduled())
        cancelEvent(selfSender_);

    //re-scheduling until ACK_STOP_MEAPP ClusterizePacket is received
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + period_, selfStop_);
}

/*
 * -----------------------------------------------Receiver Side------------------------------------------
 */
void UEClusterizeApp::handleMEAppAckStart(MEAppPacket* pkt){

    EV << "UEClusterizeApp::handleMEAppAckStart - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]" << endl;
    //stop auto-scheduling for sending START_MEAPP ClusterizePacket
    cancelEvent(selfStart_);

    //starting sending INFO_UEAPP ClusterizeInfoPacket
    if(!selfSender_->isScheduled())
    {
        double st = ceil(simTime().dbl());  //imposing synchronization (all INFO_UEAPP messages are sent at x_minutes:(y*period)_seconds:000_milliseconds)
        scheduleAt(st, selfSender_);
        EV << "UEClusterizeApp::handleMEAppAckStart - \t starting sendClusterizeStartPacket() at " << st << endl;
    }

    //starting sending STOP_MEAPP ClusterizePacket
    if(!selfStop_->isScheduled())
    {
        simtime_t  stopTime = par("stopTime");
        EV << "UEClusterizeApp::handleMEAppAckStart - \t starting sendClusterizeStopPacket() in " << stopTime << " seconds " << endl;
        scheduleAt(simTime() + stopTime, selfStop_);
    }

    //#############################################################################################
    //retrieving TraCICommandInterface::Vehicle to manipulate veins car mobility
    if(veins_mobility != NULL)
    {
        EV << "UEClusterizeApp::handleMEAppAckStart - Retrieving VehicleInterface (TRACI)" << endl;
        getVehicleInterface();
    }
    //#############################################################################################
}

void UEClusterizeApp::handleMEAppAckStop(MEAppPacket* pkt){

    EV << "UEClusterizeApp::handleMEAppAckStop - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]" << endl;
    //stop the auto-scheduling for sending STOP_MEAPP ClusterizePacket
    stopped = true;
    cancelEvent(selfStop_);

    //updating runtime color of the car icon background
    car->getDisplayString().setTagArg("i",1, "white");
}

void UEClusterizeApp::handleClusterizeConfig(ClusterizeConfigPacket* pkt){

    EV << "UEClusterizeApp::handleClusterizeConfig - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]" << endl;
    EV << "UEClusterizeApp::handleClusterizeConfig - Cluster "<< pkt->getClusterID() << " (" << pkt->getClusterColor() <<"): " << pkt->getClusterString() << endl;

    //handling the INFO_MEAPP ClusterizeConfigPacket propagation
    txMode = (char*)pkt->getTxMode();
    if( strcmp(pkt->getSourceAddress(), meHostSymbolicAddress) == 0 )
        handleClusterizeConfigFromMEHost(pkt);
    else
        handleClusterizeConfigFromUE(pkt);

    //retrieving the acceleration received with the INFO_MEAPP ClusterizeConfigPacket
    double acceleration = updateAcceleration(pkt);

    //setting the acceleration (modifying the mobility)
    if(veins_mobility != NULL)
    {
                                                                //TODO ENABLE VEINS MOBILITY
        //traciVehicle->slowDown(speed, timeInterval);
    }
    else if(linear_mobility != NULL)
    {
                                                                //TODO ADJUST (for now I added setAcceleration in INET.LinearMobility)
        linear_mobility->setAcceleration(acceleration);
    }

    // emit statistics
    simtime_t delay = simTime() - pkt->getTimestamp();
    emit(clusterizeConfigRcvdMsg_, (long)1);
    emit(clusterizeConfigDelay_, delay);
    stat_->recordReception(macID, pkt->getEventID(), delay, pkt->getHops());
}


void UEClusterizeApp::handleClusterizeConfigFromMEHost(ClusterizeConfigPacket *pkt){

    EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost: txMode " << txMode <<endl;

    //updating runtime color of the car icon background
    car->getDisplayString().setTagArg("i",1, pkt->getClusterColor());

    //V2V_UNICAST: propagating the INFO_MEAPP ClusterizeConfigPacket to the follower
    if(!strcmp(txMode, V2V_UNICAST_TX_MODE)){

        carFollowerSymbolicAddress = (char*) pkt->getClusterFollower();

        if(strcmp(carFollowerSymbolicAddress, "") != 0)
        {
            //check if car has left the network!
            if(getSimulation()->getModuleByPath(carFollowerSymbolicAddress) == NULL)
            {
                EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - \tERROR! Cannot propagate " << INFO_MEAPP << ": " << carFollowerSymbolicAddress << " left the Network!" << endl;
                return;
            }

            carFollowerAddress_ = inet::L3AddressResolver().resolve(carFollowerSymbolicAddress);

            //configuring and sending the INFO_MEAPP ClusterizeConfigPacket on the sidelink (D2D LTE NIC)
            if(lteNic->hasPar("d2dPeerAddresses"))
            {
                lteNic->par("d2dPeerAddresses") = carFollowerSymbolicAddress;
                ClusterizeConfigPacket* packet = new ClusterizeConfigPacket(*pkt);
                packet->setSourceAddress(mySymbolicAddress);
                socket.sendTo(packet, carFollowerAddress_ , destPort_);

                EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - propagating " << INFO_MEAPP <<" ClusterizeConfigPacket to: " << carFollowerSymbolicAddress << endl;
            }
            else
                EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - \tERROR configuring lteNic.d2dPeerAddresses!" << endl;
        }
    }
    //V2V_MUTICAST: propagating the INFO_MEAPP ClusterizeConfigPacket to the multicast group
    else if(!strcmp(txMode, V2V_MULTICAST_TX_MODE) && pkt->getClusterListArraySize() > 1)
    {
        //sending the INFO_MEAPP ClusterizeConfigPacket to the multicast address
        ClusterizeConfigPacket* packet = new ClusterizeConfigPacket(*pkt);
        packet->setSourceAddress(mySymbolicAddress);
        packet->setHops(packet->getHops()+1);
        multicastSocket.sendTo(packet, multicastAddress_, multicastPort_);

        EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - propagating " << INFO_MEAPP <<" ClusterizeConfigPacket to multicast: " << multicastGoupAddress << " : " << multicastPort_<< endl;
    }
}


void UEClusterizeApp::handleClusterizeConfigFromUE(ClusterizeConfigPacket *pkt){

    EV << "UEClusterizeApp::handleClusterizeConfigFromUE: txMode " << pkt->getTxMode() <<endl;

    //retrieve from INFO_MEAPP ClusterizeConfigPacket the follower and following cars
    std::string follower = getFollower(pkt);
    std::string following = getFollowing(pkt);

    if(!strcmp(following.c_str(), "") && !strcmp(follower.c_str(), ""))
    {
        EV << "UEClusterizeApp::handleClusterizeConfigFromUE - received ClusterizeConfig for another Cluster ... discarding!" << endl;
        return;
    }
    else
    {
        //updating the follower and following fields in the INFO_MEAPP ClusterizeConfigPacket to propagate
        pkt->setClusterFollower(follower.c_str());
        pkt->setClusterFollowing(following.c_str());

        //updating runtime color of the car icon background
        car->getDisplayString().setTagArg("i",1, pkt->getClusterColor());

        //V2V_UNICAST: propagating the INFO_MEAPP ClusterizeConfigPacket to the follower
        if(!strcmp(txMode, V2V_UNICAST_TX_MODE)){

            if(strcmp(follower.c_str(), "") != 0)
            {
                carFollowerSymbolicAddress = (char*) follower.c_str();

                //check if car has left the network!
                if(getSimulation()->getModuleByPath(carFollowerSymbolicAddress) == NULL)
                {
                    EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - \tERROR! Cannot propagate " << INFO_MEAPP << ": " << carFollowerSymbolicAddress << " left the Network!" << endl;
                    return;
                }

                carFollowerAddress_ = inet::L3AddressResolver().resolve(carFollowerSymbolicAddress);

                //configuring and sending the INFO_MEAPP ClusterizeConfigPacket on the sidelink (D2D LTE NIC)
                if(lteNic->hasPar("d2dPeerAddresses"))
                {
                    lteNic->par("d2dPeerAddresses") = carFollowerSymbolicAddress;
                    ClusterizeConfigPacket* packet = new ClusterizeConfigPacket(*pkt);
                    packet->setSourceAddress(mySymbolicAddress);
                    packet->setHops(packet->getHops()+1);
                    socket.sendTo(packet, carFollowerAddress_ , destPort_);

                    EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - propagating " << INFO_MEAPP <<" ClusterizeConfigPacket to: " << carFollowerSymbolicAddress << endl;
                }
                else
                    EV << "UEClusterizeApp::handleClusterizeConfigFromUE - \tERROR configuring lteNic.d2dPeerAddresses!" << endl;
            }
            else
                EV << "UEClusterizeApp::handleClusterizeConfigFromUE - No Follower!" << endl;
        }
    }
}

/*
 * -------------------------------------------------------------------------------------------------------------------------------
 * UTILITIES
 */
std::string UEClusterizeApp::getFollower(ClusterizeConfigPacket* pkt){

    unsigned int i=0;
    for(; i < pkt->getClusterListArraySize(); i++){
       if(strcmp(pkt->getClusterList(i), mySymbolicAddress) == 0){
           break;
       }
    }

    if(i+1 < pkt->getClusterListArraySize())
       return pkt->getClusterList(i+1);

    return "";
}

std::string UEClusterizeApp::getFollowing(ClusterizeConfigPacket* pkt){

    unsigned int i=0;
    for(; i < pkt->getClusterListArraySize(); i++){
       if(strcmp(pkt->getClusterList(i), mySymbolicAddress) == 0){
           break;
       }
    }

    if(i-1 >= 0 && i < pkt->getClusterListArraySize())
       return pkt->getClusterList(i-1);

    return "";
}

double UEClusterizeApp::updateAcceleration(ClusterizeConfigPacket *pkt){

    for(unsigned int i=0; i < pkt->getAccelerationsArraySize(); i++){
       if(strcmp(pkt->getClusterList(i), mySymbolicAddress) == 0)
       {
           requiredAcceleration = pkt->getAccelerations(i);
           EV << "UEClusterizeApp::updateAcceleration - new acceleration: " << requiredAcceleration << endl;
       }
    }
    return requiredAcceleration;
}

void UEClusterizeApp::getVehicleInterface(){
/*
    std::map<std::string, cModule*> cars = veinsManager->getManagedHosts();
    std::map<std::string, cModule*>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++)
        if(it->second == car)
            carVeinsID = it->first;

    EV << "UEClusterizeApp::getVehicleInterface - carVeinsID: " << carVeinsID << endl;

    traciVehicle = new Veins::TraCICommandInterface::Vehicle(traci->vehicle(carVeinsID));
*/

}
