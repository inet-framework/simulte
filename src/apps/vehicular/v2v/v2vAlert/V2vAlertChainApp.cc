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


#include "V2vAlertChainApp.h"

Define_Module(V2vAlertChainApp);


V2vAlertChainApp::V2vAlertChainApp()
{
    selfSender_ = NULL;
    nextSno_ = 0;
}

V2vAlertChainApp::~V2vAlertChainApp()
{
    cancelAndDelete(selfSender_);
}


void V2vAlertChainApp::initialize(int stage)
{
    EV << "V2vAlertChainApp::initialize - stage " << stage << endl;

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
        EV << "V2vAlertChainApp::initialize - ERROR getting lteNic module!" << endl;
        throw cRuntimeError("V2vAlertChainApp::initialize - \tFATAL! Error when getting lteNIC!");
    }

    carSymbolicAddress = getParentModule()->getFullName();

    EV << "V2vAlertChainApp::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort_);

    v2vAlertChainSentMsg_ = registerSignal("v2vAlertChainSentMsg");

    v2vAlertChainDelay_ = registerSignal("v2vAlertChainDelay");
    v2vAlertChainRcvdMsg_ = registerSignal("v2vAlertChainRcvdMsg");

    // calculating traffic starting time
    simtime_t startTime = par("startTime");

    scheduleAt(simTime() + startTime, selfSender_);
    EV << "\t starting traffic in " << startTime << " seconds " << endl;
}


void V2vAlertChainApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendV2vAlertPacket(NULL);
        else
            throw cRuntimeError("V2vAlertChainApp::handleMessage - Unrecognized self message");
    }
    else{

        V2vAlertPacket* pkt = check_and_cast<V2vAlertPacket*>(msg);

        if (pkt == 0)
            throw cRuntimeError("V2vAlertChainApp::handleMessage - FATAL! Error when casting to V2vAlertPacket");
        else
            handleV2vAlertPacket(pkt);
    }
}

void V2vAlertChainApp::sendV2vAlertPacket(V2vAlertPacket* rcvdPkt)
{
    //sending in unicast to all the addresses!
    std::vector<std::string> v2vPeerAddresses = splitString(v2vPeerList, " ");

    if(v2vPeerAddresses.size() > 0){

        if(rcvdPkt == NULL)
            EV << "V2vAlertChainApp::sendV2vAlertPacket - Sending V2vAlert SeqN[" << nextSno_ << "]\n";
        else
            EV << "V2vAlertChainApp::sendV2vAlertPacket - Sending V2vAlert SeqN[" << rcvdPkt->getSno() << "]\n";
        EV << "V2vAlertChainApp::sendV2vAlertPacket - ClusterID: " << clusterID << " TxMode: " << txMode << endl;

        for(int i = 0; i < v2vPeerAddresses.size(); i++){

            V2vAlertPacket* packet = new V2vAlertPacket("V2vAlert");

            //creating a new V2vAlertPacket (I'm the propagation starting car!)
            if(rcvdPkt == NULL){
                packet->setSno(nextSno_);
                packet->setTimestamp(simTime());
                packet->setByteLength(size_);
                packet->setSourceAddress(carSymbolicAddress.c_str());
                packet->setForwarderAddress(carSymbolicAddress.c_str());
            }
            //forwarding the received V2vAlertPacke
            else{
                packet->setSno(rcvdPkt->getSno());
                packet->setTimestamp(rcvdPkt->getTimestamp());
                packet->setByteLength(rcvdPkt->getByteLength());
                packet->setSourceAddress(rcvdPkt->getSourceAddress());
                packet->setForwarderAddress(carSymbolicAddress.c_str());
            }
            packet->setDestinationAddress(v2vPeerAddresses.at(i).c_str());

            if(strcmp(getParentModule()->getFullName() ,v2vPeerAddresses.at(i).c_str())){

                EV << "V2vAlertChainApp::sendV2vAlertPacket - \tsending V2vAlert to " << v2vPeerAddresses.at(i).c_str() << endl;
                destAddress_ = inet::L3AddressResolver().resolve(v2vPeerAddresses.at(i).c_str());

                socket.sendTo(packet, destAddress_, destPort_);
            }
            else
                EV << "V2vAlertChainApp::sendV2vAlertPacket - \tWARNING: can't send message to myself!" << endl;
        }

        if(rcvdPkt == NULL)
            nextSno_++;
        emit(v2vAlertChainSentMsg_, (long)1);
    }
    else
        EV << "V2vAlertChainApp::sendV2vAlertPacket - No Receiver Car available!";


    if(!selfSender_->isScheduled())
        scheduleAt(simTime() + period_, selfSender_);
}

void V2vAlertChainApp::handleV2vAlertPacket(V2vAlertPacket* pkt){

    // emit statistics
    simtime_t delay = simTime() - pkt->getTimestamp();
    emit(v2vAlertChainDelay_, delay);
    emit(v2vAlertChainRcvdMsg_, (long)1);

    EV << "V2vAlertChainApp::handleV2vAlertPacket - Received: " << pkt->getName() << " from " << pkt->getForwarderAddress() << " started by " << pkt->getSourceAddress() << endl;
    EV << "V2vAlertChainApp::handleV2vAlertPacket - Packet received: SeqNo[" << pkt->getSno() << "] Delay[" << delay << "] (from starting Car)" << endl;

    // forwarding the alertPacket behind!
    sendV2vAlertPacket(pkt);
}

std::vector<std::string> V2vAlertChainApp::splitString(std::string v2vReceivers, const char* delimiter){

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
void V2vAlertChainApp::setClusterConfiguration(ClusterizeConfigPacket* pkt){

    EV << "V2vAlertChainApp::setClusterConfiguration - configuring cluster informations" << endl;

    clusterID = pkt->getClusterID();
    txMode = pkt->getTxMode();
    v2vPeerList = pkt->getV2vPeerList();                //list of Car[*] module names!

    //d2dConfiguration dynamic
    if(v2vPeerList.compare("") != 0)
    {
        if(lteNic->hasPar("d2dPeerAddresses"))
        {
            lteNic->par("d2dPeerAddresses") = v2vPeerList;
            EV << "V2vAlertChainApp::setClusterConfiguration - lteNic.d2dPeerAddresses: " << pkt->getV2vPeerList() << "configured!" << endl;
        }
        else
            EV << "V2vAlertChainApp::setClusterConfiguration - \tERROR configuring lteNic.d2dPeerAddresses!" << endl;
    }
}

