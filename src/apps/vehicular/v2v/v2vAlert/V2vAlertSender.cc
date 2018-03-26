//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <cmath>
#include <string.h>

#include "V2vAlertSender.h"
#include "inet/common/ModuleAccess.h"  // for multicast support


Define_Module(V2vAlertSender);

V2vAlertSender::V2vAlertSender()
{
    selfSender_ = NULL;
    nextSno_ = 0;
}

V2vAlertSender::~V2vAlertSender()
{
    cancelAndDelete(selfSender_);
}


void V2vAlertSender::initialize(int stage)
{
    EV << "V2vAlertSender::initialize - stage " << stage << endl;

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
        EV << "V2vAlertSender::initialize - ERROR getting lteNic module!" << endl;
        throw cRuntimeError("V2vAlertSender::initialize - \tFATAL! Error when getting lteNIC!");
    }

    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort_);

    // for multicast support                                                                                                       //TODO
    /*
    inet::IInterfaceTable *ift = inet::getModuleFromPar< inet::IInterfaceTable >(par("interfaceTableModule"), this);
    inet::MulticastGroupList mgl = ift->collectMulticastGroups();
    socket.joinLocalMulticastGroups(mgl);
    inet::InterfaceEntry *ie = ift->getInterfaceByName("wlan");
    if (!ie)
        throw cRuntimeError("Wrong multicastInterface setting: no interface named wlan");
    socket.setMulticastOutputInterface(ie->getInterfaceId());
     */

    v2vAlertSentMsg_ = registerSignal("v2vAlertSentMsg");

    EV << "V2vAlertSender::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;

    // calculating traffic starting time
    simtime_t startTime = par("startTime");

    scheduleAt(simTime() + startTime, selfSender_);
    EV << "\t starting traffic in " << startTime << " seconds " << endl;
}

void V2vAlertSender::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendV2vAlertPacket();
        else
            throw cRuntimeError("V2vAlertSender::handleMessage - Unrecognized self message");
    }
}

void V2vAlertSender::sendV2vAlertPacket()
{
    //sending in unicast to all the addresses!
    if( txMode == UNICAST_TX_MODE){

        std::vector<std::string> v2vPeerAddresses = splitString(v2vPeerList, " ");

        if(v2vPeerAddresses.size() > 0){

            EV << "V2vAlertSender::sendV2vAlertPacket - Sending V2vAlert SeqN[" << nextSno_ << "]\n";
            EV << "V2vAlertSender::sendV2vAlertPacket - ClusterID: " << clusterID << " TxMode: " << txMode << endl;

            for(int i = 0; i < v2vPeerAddresses.size(); i++){

                V2vAlertPacket* packet = new V2vAlertPacket("V2vAlert");
                packet->setSno(nextSno_);
                packet->setTimestamp(simTime());
                packet->setByteLength(size_);

                if(strcmp(getParentModule()->getFullName() ,v2vPeerAddresses.at(i).c_str())){

                    EV << "V2vAlertSender::sendV2vAlertPacket - \tsending V2vAlert to " << v2vPeerAddresses.at(i).c_str() << endl;
                    destAddress_ = inet::L3AddressResolver().resolve(v2vPeerAddresses.at(i).c_str());
                    socket.sendTo(packet, destAddress_, destPort_);
                }
                else
                    EV << "V2vAlertSender::sendV2vAlertPacket - \tWARNING: can't send message to myself!" << endl;
            }
            nextSno_++;
        }
    }

    emit(v2vAlertSentMsg_, (long)1);

    scheduleAt(simTime() + period_, selfSender_);
}

std::vector<std::string> V2vAlertSender::splitString(std::string v2vReceivers, const char* delimiter){

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

void V2vAlertSender::setClusterConfiguration(ClusterizeConfigPacket* pkt){

    EV << "V2vAlertSender::setClusterConfiguration - new v2vPeerList: " << pkt->getV2vPeerList() << endl;

    clusterID = pkt->getClusterID();
    txMode = pkt->getTxMode();
    v2vPeerList = pkt->getV2vPeerList();                //list of Car[*] module names!

    //d2dConfiguration dynamic
    if(lteNic->hasPar("d2dPeerAddresses"))
        lteNic->par("d2dPeerAddresses") = v2vPeerList;
    else
        EV << "V2vAlertSender::setClusterConfiguration - \tERROR configuring lteNic.d2dPeerAddresses!" << endl;

}
