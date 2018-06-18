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
    selfStart_ = NULL;
    selfSender_ = NULL;
    selfStop_ = NULL;
    nextSnoStart_ = 0;
    nextSnoInfo_ = 0;
    nextSnoStop_ = 0;

    v2vAppName = "";
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


    // for multicast support
    multicastPort_ = par("multicastPort").longValue();
    multicastGoupAddress = (char*) par("multicastGoupAddress").stringValue();
    multicastAddress_ = inet::L3AddressResolver().resolve(multicastGoupAddress);
    EV << "UEClusterizeApp::initialize - multicastGoupAddress: " << multicastGoupAddress << endl;

    multicastSocket.setOutputGate(gate("udpOut"));
    multicastSocket.bind(multicastPort_);
    EV << "UEClusterizeApp::initialize - binding to multicast port:" << multicastPort_ << endl;

    inet::IInterfaceTable *ift = inet::getModuleFromPar< inet::IInterfaceTable >(par("interfaceTableModule"), this);
    inet::InterfaceEntry *ie = ift->getInterfaceByName("wlan");
    if (!ie)
        throw cRuntimeError("Wrong multicastInterface setting: no interface named wlan");
    inet::MulticastGroupList mgl = ift->collectMulticastGroups();
    multicastSocket.joinLocalMulticastGroups(mgl);
    multicastSocket.setMulticastOutputInterface(ie->getInterfaceId());
    // -------------------- //


    car = this->getParentModule();
        //getting mobility module
        cModule *temp = getParentModule()->getSubmodule("mobility");
        if(temp != NULL){
            mobility = check_and_cast<inet::IMobility*>(temp);

            if(!strcmp(car->par("mobilityType").stringValue(),"VeinsInetMobility")){
                veins_mobility = check_and_cast<Veins::VeinsInetMobility*>(temp);
                EV << "UEClusterizeApp::initialize - Mobility module found!" << endl;
            }
            else{
                // necessary for using inet mobility (in sendClusterizeInfo needs to reverse the angle!)
                veins_mobility = NULL;
            }
        }
        else
            EV << "UEClusterizeApp::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;

    lteNic = car->getSubmodule("lteNic");
        if(lteNic == NULL){
            EV << "V2vClusterBaseApp::initialize - ERROR getting lteNic module!" << endl;
            throw cRuntimeError("V2vClusterAlertApp::initialize - \tFATAL! Error when getting lteNIC!");
        }

    LteMacBase* mac = check_and_cast<LteMacBase*>(lteNic->getSubmodule("mac"));
    macID = mac->getMacNodeId();

    selfSender_ = new cMessage("selfSender");
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");

    v2vApp = getParentModule()->getModuleByPath(par("v2vAppPath"));
    if(v2vApp == NULL){
        EV << "UEClusterizeApp::initialize - ERROR getting v2vApp module!\nRunning without ..." << endl;
        //throw cRuntimeError("UEClusterizeApp::initialize - \tFATAL! Error when getting v2vApp!");
    }
    else{
        v2vClusterApp = check_and_cast<V2vClusterBaseApp*>(v2vApp);
        v2vAppName = (char*) v2vClusterApp->getClassName();
    }

    //statics signals
    clusterizeInfoSentMsg_ = registerSignal("clusterizeInfoSentMsg");
    clusterizeConfigRcvdMsg_ = registerSignal("clusterizeConfigRcvdMsg");
    clusterizeConfigDelay_ = registerSignal("clusterizeConfigDelay");


    // global statistics recorder
    stat_ = check_and_cast<MultihopD2DStatistics*>(getModuleByPath("d2dMultihopStatistics"));

    //START app
    simtime_t startTime = par("startTime");
    EV << "\t starting sendClusterizeStartPacket() in " << startTime << " seconds " << endl;
    // starting sendClusterizeStartPacket() to initialize the ME App on the ME Host
    scheduleAt(simTime() + startTime, selfStart_);
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

    ClusterizePacket* packet = ClusterizePacketBuilder().buildClusterizePacket(START_CLUSTERIZE, nextSnoStart_, simTime(), size_, car->getId(), v2vAppName, sourceSimbolicAddress, destSimbolicAddress);

    socket.sendTo(packet, destAddress_, destPort_);
    nextSnoStart_++;

    // trying to instantiate the MECLusterizeApp until receiving the ack
    scheduleAt(simTime() + period_, selfStart_);
}

void UEClusterizeApp::sendClusterizeInfoPacket()
{
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Sending message SeqNo[" << nextSnoInfo_ << "]\n";

    //position = m and speed = m/s..    --> setted in the simulation "*.rou.xml" config files for SUMO
    //w.r.t. top-left edge in the Network with Coord [0 ; 0]
    if(veins_mobility == NULL){
        position = mobility->getCurrentPosition();
        speed = mobility->getCurrentSpeed();
        maxSpeed = mobility->getMaxSpeed();
        angularSpeed = mobility->getCurrentAngularSpeed();
        angularPosition = mobility->getCurrentAngularPosition();
        angularPosition.alpha *= -1.0;
        EV << "UEClusterizeApp::sendClusterizeInfoPacket - No VeinsMobility: reverting the angular position!"<< endl;

        //Coord max = mobility->getConstraintAreaMax();
        //Coord min = mobility->getConstraintAreaMin();
        //EV << "UEClusterizeApp::sendClusterizeInfoPacket - Area: "<<max<<" - "<<min<< endl;
        //if(position.x<=min.x+10 || position.x >= max.x-10 || position.y <= min.y+10 || position.y >= max.y-10)
            //this->callFinish();
    }
    else{
        position = veins_mobility->getCurrentPosition();
        speed = veins_mobility->getCurrentSpeed();
        maxSpeed = veins_mobility->getMaxSpeed();
        angularSpeed = veins_mobility->getCurrentAngularSpeed();
        angularPosition = veins_mobility->getCurrentAngularPosition();
        }


    ClusterizeInfoPacket* packet = ClusterizePacketBuilder().buildClusterizeInfoPacket(nextSnoInfo_, simTime(), size_, car->getId(), v2vAppName, sourceSimbolicAddress, destSimbolicAddress, position, speed, angularPosition, angularSpeed);

    //testing
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Info: carID " << car->getId() << " sourceAddr " << sourceSimbolicAddress << " destAddr " << destSimbolicAddress << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Position: [" << position.x << " ; " << position.y << " ; " << position.z << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Speed: [" << speed.x << " ; " << speed.y << " ; " << speed.z << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - Max Speed: [" << maxSpeed << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - AngularPosition: [" << angularPosition.alpha << " ; " << angularPosition.beta << " ; " << angularPosition.gamma << "]" << endl ;
    EV << "UEClusterizeApp::sendClusterizeInfoPacket - AngularSpeed: [" << angularSpeed.alpha << " ; " << angularSpeed.beta << " ; " << angularSpeed.gamma << "]" << endl ;

    socket.sendTo(packet, destAddress_, destPort_);
    nextSnoInfo_++;

    emit(clusterizeInfoSentMsg_, (long)1);

    if(selfSender_->isScheduled())
        cancelEvent(selfSender_);

    scheduleAt(simTime() + period_, selfSender_);
}

void UEClusterizeApp::sendClusterizeStopPacket()
{
    EV << "UEClusterizeApp::sendClusterizeStopPacket - Sending message SeqNo[" << nextSnoStop_ << "]\n";

    ClusterizePacket* packet = ClusterizePacketBuilder().buildClusterizePacket(STOP_CLUSTERIZE, nextSnoStart_, simTime(), size_, car->getId(), v2vAppName, sourceSimbolicAddress, destSimbolicAddress);

    socket.sendTo(packet, destAddress_, destPort_);
    nextSnoStop_++;

    // stopping the info updating because the MEClusterizeApp is terminated
    if(selfSender_->isScheduled())
        cancelEvent(selfSender_);

    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
    // trying to terminate the MECLusterizeApp until receiving the ack
    scheduleAt(simTime() + period_, selfStop_);
}

/*
 * -----------------------------------------------Receiver Side------------------------------------------
 */
void UEClusterizeApp::handleClusterizeAckStart(ClusterizePacket* pkt){

    EV << "UEClusterizeApp::handleClusterizeAckStart - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]" << endl;

    cancelEvent(selfStart_);

    // starting sending local-info updates
    if(!selfSender_->isScheduled()){
        //check to avoid multiple scheduling by receiving duplicated acks!

        simtime_t startTime = par("startTime");
        scheduleAt(simTime() + period_, selfSender_);
        EV << "\t starting traffic in " << period_ << " seconds " << endl;
    }

    if(!selfSender_->isScheduled()){
        //STOP app
        simtime_t  stopTime = par("stopTime");
        EV << "\t starting sendClusterizeStopPacket() in " << stopTime << " seconds " << endl;
        // starting sendClusterizeSopPacket() to terminate the ME App on the ME Host
        scheduleAt(simTime() + stopTime, selfStop_);
    }
}

void UEClusterizeApp::handleClusterizeAckStop(ClusterizePacket* pkt){

    EV << "UEClusterizeApp::handleClusterizeAckStop - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]" << endl;

    cancelEvent(selfStop_);
    //this->callFinish();
    //this->deleteModule();

    //updating V2vApp Clustering inforamtion
    if(v2vApp != NULL){

        ClusterizeConfigPacket* pkt = new ClusterizeConfigPacket(CONFIG_CLUSTERIZE);
        pkt->setTimestamp(simTime());
        pkt->setType(CONFIG_CLUSTERIZE);
        pkt->setClusterFollower("");
        pkt->setClusterFollowing("");

        EV << "UEClusterizeApp::handleClusterizeAckStop - Stop Cluster Configuration " << v2vAppName << endl;
        v2vClusterApp->setClusterConfiguration(pkt);

        //updating runtime color of the car icon background
        car->getDisplayString().setTagArg("i",1,"white");
    }

    //Stop the v2vApp
    if(v2vApp != NULL){
        v2vApp->callFinish();
        //v2vApp->deleteModule();
    }
}

void UEClusterizeApp::handleClusterizeConfig(ClusterizeConfigPacket* pkt){

    EV << "UEClusterizeApp::handleClusterizeConfig - Packet received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]" << endl;
    EV << "UEClusterizeApp::handleClusterizeConfig - Cluster "<< pkt->getClusterID() << " (" << pkt->getClusterColor() <<"): " << pkt->getClusterString() << endl;

    // if message received from MEHost
    if( strcmp(pkt->getSourceAddress(), destSimbolicAddress) == 0 ){

        handleClusterizeConfigFromMEHost(pkt);
    }
    else{
        // message propagated by another car
        handleClusterizeConfigFromUE(pkt);
    }

    //check_and_cast<LinearMobility*>(mobility)->setSpeed(15);
    //check_and_cast<LinearMobility*>(mobility)->setAcceleration(0.2);

    simtime_t delay = simTime()-pkt->getTimestamp();
    // emit statistics
    emit(clusterizeConfigRcvdMsg_, (long)1);
    emit(clusterizeConfigDelay_, delay);

    // emit global-statistics
    stat_->recordReception(macID, pkt->getEventID(), delay, pkt->getHops());

    EV << "UEClusterizeApp::handleClusterizeConfig - \tClusterizeConfig Delay: " << delay << endl;
}


void UEClusterizeApp::handleClusterizeConfigFromMEHost(ClusterizeConfigPacket *pkt){

    EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost" <<endl;

    //configure the Cluster parameters for V2vClusterBaseApp
    if(v2vApp != NULL){

        EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - Configuring " << v2vAppName << endl;
        v2vClusterApp->setClusterConfiguration(pkt);
    }
    //updating runtime color of the car icon background
    car->getDisplayString().setTagArg("i",1, pkt->getClusterColor());

    // propagating in D2D UNICAST to the Follower
    if(pkt->getTxMode() == V2V_UNICAST_TX_MODE){

        if(strcmp(pkt->getClusterFollower(), "") != 0)
        {
            v2vFollowerSimbolicAddress = (char*) pkt->getClusterFollower();

            //check if car has left the network!
            if(getSimulation()->getModuleByPath(v2vFollowerSimbolicAddress) == NULL){
                EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - \tERROR! Cannot propagate Configuration: " << v2vFollowerSimbolicAddress << " left the Network!" << endl;
                return;
            }

            v2vFollowerAddress_ = inet::L3AddressResolver().resolve(v2vFollowerSimbolicAddress);

            //configuring the D2D LTE NIC
            if(lteNic->hasPar("d2dPeerAddresses"))
            {
                lteNic->par("d2dPeerAddresses") = v2vFollowerSimbolicAddress;
                EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - lteNic.d2dPeerAddresses: " << v2vFollowerSimbolicAddress << " configured!" << endl;

                ClusterizeConfigPacket* packet = new ClusterizeConfigPacket(*pkt);
                packet->setSourceAddress(sourceSimbolicAddress);

                //propagate configuration to the follower
                socket.sendTo(packet, v2vFollowerAddress_ , destPort_);
                EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - propagating ClusterizeConfig to: " << v2vFollowerSimbolicAddress << endl;
            }
            else
                EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - \tERROR configuring lteNic.d2dPeerAddresses!" << endl;
        }
    }
    // propagating in D2D MULTICAST
    else if(pkt->getTxMode() == V2V_MULTICAST_TX_MODE && pkt->getClusterListArraySize() > 1)
    {
        ClusterizeConfigPacket* packet = new ClusterizeConfigPacket(*pkt);
        packet->setSourceAddress(sourceSimbolicAddress);
        packet->setHops(packet->getHops()+1);

        packet->setName(v2vAppName);

        //propagate on the multicast address
        multicastSocket.sendTo(packet, multicastAddress_, multicastPort_);
        EV << "UEClusterizeApp::handleClusterizeConfigFromMEHost - propagating ClusterizeConfig to multicast: " << multicastGoupAddress << " : " << multicastPort_<< endl;
    }

}


void UEClusterizeApp::handleClusterizeConfigFromUE(ClusterizeConfigPacket *pkt){


    EV << "UEClusterizeApp::handleClusterizeConfigFromUE" <<endl;

    // Update Follower and Following ( retrieve from the Cluster Member Ordered List)
    std::string follower = getFollower(pkt);
    std::string following = getFollowing(pkt);

    if(following.compare("") == 0 && follower.compare("") == 0 )
    {
        EV << "UEClusterizeApp::handleClusterizeConfigFromUE - received ClusterizeConfig for another Cluster ... discarding!" << endl;
    }
    else
    {
        pkt->setClusterFollower(follower.c_str());
        pkt->setClusterFollowing(following.c_str());

        //configure the Cluster parameters for V2vClusterBaseApp (with the updated follower and following)
        if(v2vApp != NULL){

            EV << "UEClusterizeApp::handleClusterizeConfigFromUE - Configuring " << v2vAppName << endl;
            v2vClusterApp->setClusterConfiguration(pkt);
        }

        //updating runtime color of the car icon background
        car->getDisplayString().setTagArg("i",1, pkt->getClusterColor());

        // propagating in D2D UNICAST to the Follower
        if(pkt->getTxMode() == V2V_UNICAST_TX_MODE){

            if(strcmp(pkt->getClusterFollower(), "") != 0)
            {
                v2vFollowerSimbolicAddress = (char*) pkt->getClusterFollower();

                //check if car has left the network!
                if(getSimulation()->getModuleByPath(v2vFollowerSimbolicAddress) == NULL){
                    EV << "UEClusterizeApp::handleClusterizeConfigFromUE - \tERROR! Cannot propagate Configuration: " << v2vFollowerSimbolicAddress << " left the Network!" << endl;
                    return;
                }

                v2vFollowerAddress_ = inet::L3AddressResolver().resolve(v2vFollowerSimbolicAddress);                                            //CONTROLLARE SE CAR è USCITA!

                //configuring the D2D LTE NIC
                if(lteNic->hasPar("d2dPeerAddresses"))
                {
                    lteNic->par("d2dPeerAddresses") = v2vFollowerSimbolicAddress;
                    EV << "UEClusterizeApp::handleClusterizeConfigFromUE - lteNic.d2dPeerAddresses: " << v2vFollowerSimbolicAddress << " configured!" << endl;

                    ClusterizeConfigPacket* packet = new ClusterizeConfigPacket(*pkt);
                    packet->setSourceAddress(sourceSimbolicAddress);
                    packet->setHops(packet->getHops()+1);

                    //propagate configuration to the follower
                    socket.sendTo(packet, v2vFollowerAddress_ , destPort_);
                    EV << "UEClusterizeApp::handleClusterizeConfigFromUE - propagating ClusterizeConfig to: " << v2vFollowerSimbolicAddress << endl;
                }
                else
                    EV << "UEClusterizeApp::handleClusterizeConfigFromUE - \tERROR configuring lteNic.d2dPeerAddresses!" << endl;
            }
        }
    }
}

/*
 * -------------------------------------------------------------------------------------------------------------------------------
 * UTILITIES
 */
std::string UEClusterizeApp::getFollower(ClusterizeConfigPacket* pkt){

    int i=0;
    for(; i < pkt->getClusterListArraySize(); i++){
       if(strcmp(pkt->getClusterList(i), sourceSimbolicAddress) == 0){
           break;
       }
    }

    if(i+1 < pkt->getClusterListArraySize() && strcmp(pkt->getClusterList(i+1), "") != 0)
       return pkt->getClusterList(i+1);

    return "";
}

std::string UEClusterizeApp::getFollowing(ClusterizeConfigPacket* pkt){

    int i=0;
    for(; i < pkt->getClusterListArraySize(); i++){
       if(strcmp(pkt->getClusterList(i), sourceSimbolicAddress) == 0){
           break;
       }
    }

    if(i-1 >= 0)
       return pkt->getClusterList(i-1);

    return "";
}
