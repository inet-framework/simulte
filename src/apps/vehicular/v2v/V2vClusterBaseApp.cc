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

#include "V2vClusterBaseApp.h"

Define_Module(V2vClusterBaseApp);

V2vClusterBaseApp::V2vClusterBaseApp()
{
    clusterLeader = false;
    selfSender_ = NULL;
    nextSno_ = 0;
}

V2vClusterBaseApp::~V2vClusterBaseApp()
{
    cancelAndDelete(selfSender_);
}

void V2vClusterBaseApp::initialize(int stage)
{
    EV << "V2vClusterBaseApp::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    selfSender_ = new cMessage("selfSender");

    size_ = par("packetSize");
    period_ = par("period");
    localPort_ = par("localPort");
    destPort_ = par("destPort");
    //destAddress_ dinamically updated

    lteNic = getParentModule()->getModuleByPath(".lteNic");
    if(lteNic == NULL){
        EV << "V2vClusterBaseApp::initialize - ERROR getting lteNic module!" << endl;
        throw cRuntimeError("V2vClusterAlertApp::initialize - \tFATAL! Error when getting lteNIC!");
    }

    carSymbolicAddress = getParentModule()->getFullName();

    EV << "V2vClusterBaseApp::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort_);

    // calculating traffic starting time
    simtime_t startTime = par("startTime");

    scheduleAt(simTime() + startTime, selfSender_);
    EV << "\t starting traffic in " << startTime << " seconds " << endl;

}

void V2vClusterBaseApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender")){

            EV << "V2vClusterBaseApp::handleMessage - running (" << nextSno_++ << ")\n";

            if(!selfSender_->isScheduled())
                scheduleAt(simTime() + period_, selfSender_);
        }
    }
}


/*
 * utility function
 */
std::vector<std::string> V2vClusterBaseApp::splitString(std::string v2vReceivers, const char* delimiter){

    std::vector<std::string> v2vAddresses;
    char* token;
    char* str = new char[v2vReceivers.length() + 1];
    std:strcpy(str, v2vReceivers.c_str());

    token = strtok (str , delimiter);            // split by spaces

    while (token != NULL)
    {
      v2vAddresses.push_back(token);
      token = strtok (NULL, delimiter);         // split by spaces
    }

    delete(str);

    return v2vAddresses;
}

/**
 * CrossModule function, called by UECluserizeApp to set the cluster configuration!
 */
void V2vClusterBaseApp::setClusterConfiguration(ClusterizeConfigPacket* pkt){

    EV << "V2vClusterBaseApp::setClusterConfiguration - configuring cluster informations" << endl;

    clusterID = pkt->getClusterID();
    txMode = pkt->getTxMode();
    follower = pkt->getClusterFollower();                //list of Car[*] module names!

    std::string following = pkt->getClusterFollowing();
    clusterLeader = following.empty();                      //if following string is empty I'm a cluster leader!


    // update receivers v2v (for now jsut the follower!)
    if(follower.compare("") != 0)
    {
        v2vReceivers = follower;

        //configuring the D2D LTE NIC
        if(lteNic->hasPar("d2dPeerAddresses"))
        {
            lteNic->par("d2dPeerAddresses") = follower;
            EV << "V2vClusterAlertApp::setClusterConfiguration - lteNic.d2dPeerAddresses: " << follower << " configured!" << endl;
        }
        else
            EV << "V2vClusterAlertApp::setClusterConfiguration - \tERROR configuring lteNic.d2dPeerAddresses!" << endl;
    }

    EV << "V2vClusterBaseApp::setClusterConfiguration - Cluster: " << pkt->getClusterString() << endl;

    EV << "V2vClusterBaseApp::setClusterConfiguration - Following: " << pkt->getClusterFollowing() << endl;
    EV << "V2vClusterBaseApp::setClusterConfiguration - Follower: " << pkt->getClusterFollower() << endl;
}
