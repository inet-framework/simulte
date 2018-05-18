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


#include "V2vClusterAlertApp.h"

Define_Module(V2vClusterAlertApp);


V2vClusterAlertApp::V2vClusterAlertApp()
{
}

V2vClusterAlertApp::~V2vClusterAlertApp()
{
}


void V2vClusterAlertApp::initialize(int stage)
{
    EV << "V2vClusterAlertApp::initialize - stage " << stage << endl;

    V2vClusterBaseApp::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    v2vClusterAlertSentMsg_ = registerSignal("v2vClusterAlertSentMsg");
    v2vClusterAlertDelay_ = registerSignal("v2vClusterAlertDelay");
    v2vClusterAlertRcvdMsg_ = registerSignal("v2vClusterAlertRcvdMsg");
}


void V2vClusterAlertApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendV2vAlertPacket(NULL);
        else
            throw cRuntimeError("V2vClusterAlertApp::handleMessage - Unrecognized self message");
    }
    else{

        V2vAlertPacket* pkt = check_and_cast<V2vAlertPacket*>(msg);

        if (pkt == 0)
            throw cRuntimeError("V2vClusterAlertApp::handleMessage - FATAL! Error when casting to V2vAlertPacket");
        else
            handleV2vAlertPacket(pkt);
    }
}

void V2vClusterAlertApp::sendV2vAlertPacket(V2vAlertPacket* rcvdPkt)
{
    EV << "V2vClusterAlertApp::sendV2vAlertPacket - Sending V2vAlert\n";

    //for now just using V2V UNICAST MODE!
    sendV2vUnicast(rcvdPkt);

    if(!selfSender_->isScheduled())
        scheduleAt(simTime() + period_, selfSender_);
}

void V2vClusterAlertApp::handleV2vAlertPacket(V2vAlertPacket* pkt){

    // emit statistics
    simtime_t delay = simTime() - pkt->getTimestamp();
    emit(v2vClusterAlertDelay_, delay);
    emit(v2vClusterAlertRcvdMsg_, (long)1);

    EV << "V2vClusterAlertApp::handleV2vAlertPacket - Received: " << pkt->getName() << " from " << pkt->getForwarderAddress() << " started by " << pkt->getSourceAddress() << endl;
    EV << "V2vClusterAlertApp::handleV2vAlertPacket - Packet received: SeqNo[" << pkt->getSno() << "] Delay[" << delay << "] (from starting Car)" << endl;

    // forwarding the alertPacket behind!
    sendV2vAlertPacket(pkt);
}


void V2vClusterAlertApp::sendV2vUnicast(V2vAlertPacket* rcvdPkt){

    //sending in v2v - unicast to all the addresses!
    std::vector<std::string> v2vReceiverAddress = splitString(v2vReceivers, " ");

    if(v2vReceiverAddress.size() > 0){

        //I'm Leader and I start sending the Alert
        if(clusterLeader && rcvdPkt == NULL){

            EV << "V2vClusterAlertApp::sendV2vUnicast - Sending V2vAlert SeqN[" << nextSno_ << "]\n";

            for(int i = 0; i < v2vReceiverAddress.size(); i++){

                bool sent = sendSocketPacket(nextSno_, simTime(), size_, carSymbolicAddress, carSymbolicAddress, v2vReceiverAddress.at(i));

                if(sent){
                    nextSno_++;
                    emit(v2vClusterAlertSentMsg_, (long)1);
                }
            }
        }
        //I'm Not the Leader and I forward the Alert
        else if(rcvdPkt != NULL){

            EV << "V2vClusterAlertApp::sendV2vUnicast - Forwarding V2vAlert SeqN[" << rcvdPkt->getSno() << "]\n";

            for(int i = 0; i < v2vReceiverAddress.size(); i++){

                bool sent = sendSocketPacket(rcvdPkt->getSno(), rcvdPkt->getTimestamp(), rcvdPkt->getByteLength(), rcvdPkt->getSourceAddress(), carSymbolicAddress, v2vReceiverAddress.at(i));

                if(sent){
                   emit(v2vClusterAlertSentMsg_, (long)1);
                }
            }
        }
    }
    else
        EV << "V2vClusterAlertApp::sendV2vUnicast - No Receiver Car available!";

    delete(rcvdPkt);
}


void V2vClusterAlertApp::sendV2vMulticast(V2vAlertPacket* pkt){

}


bool V2vClusterAlertApp::sendSocketPacket(int sqn, simtime_t time, int size, std::string source, std::string forwarder, std::string destination){

    V2vAlertPacket* packet = new V2vAlertPacket("V2vAlert");

    //creating a new V2vAlertPacket
    packet->setSno(sqn);
    packet->setTimestamp(time);
    packet->setByteLength(size);
    packet->setSourceAddress(source.c_str());
    packet->setForwarderAddress(forwarder.c_str());
    packet->setDestinationAddress(destination.c_str());

    //check if i'm not the receiver!                                                                    //maybe not needed
    if(strcmp(getParentModule()->getFullName() , destination.c_str())){

        EV << "V2vClusterAlertApp::sendSocketPacket - \tSending V2vAlert to " << destination << endl;
        destAddress_ = inet::L3AddressResolver().resolve(destination.c_str());
        socket.sendTo(packet, destAddress_, destPort_);

        return true;
    }
    else
        EV << "V2vClusterAlertApp::sendSocketPacket - \tWARNING: can't send message to myself!" << endl;

    return false;
}

