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


#include "MEClusterizeService.h"

Define_Module(MEClusterizeService);

MEClusterizeService::MEClusterizeService(){

    clusterSN = 1;
}

MEClusterizeService::~MEClusterizeService(){

}

void MEClusterizeService::initialize(int stage)
{
    EV << "MEClusterizeService::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    selfSender_ = new cMessage("selfSender");
    period_ = par("period");

    mePlatform = getParentModule();

    if(mePlatform != NULL){
        meHost = mePlatform->getParentModule();
        if(meHost->hasPar("maxMEApps"))
                maxMEApps = meHost->par("maxMEApps").longValue();
        else
            throw cRuntimeError("MEClusterizeService::initialize - \tFATAL! Error when getting meHost.maxMEApps parameter!");

        // MEPlatform internal gate connections with ClusterizeService
        if(mePlatform->gateSize("meAppOut") == 0 || mePlatform->gateSize("meAppIn") == 0){
            mePlatform->setGateSize("meAppOut", maxMEApps);
            mePlatform->setGateSize("meAppIn", maxMEApps);
        }
        this->setGateSize("meClusterizeAppOut", maxMEApps);
        this->setGateSize("meClusterizeAppIn", maxMEApps);
    }
    else{
        EV << "MEClusterizeService::initialize - ERROR getting mePlatform cModule!" << endl;
        throw cRuntimeError("MEClusterizeService::initialize - \tFATAL! Error when getting getParentModule()");
    }

    simtime_t startTime = par("startTime");
    scheduleAt(simTime() + startTime, selfSender_);
    EV << "\t starting computations in " << startTime << " seconds " << endl;
}
/*
 * #################################################################################################################################
 */
void MEClusterizeService::handleMessage(cMessage *msg)
{
    EV << "MEClusterizeService::handleMessage - " << endl;

    //PERIODICALLY RUNNING THE CLUSTERING ALGORITHM
    if (msg->isSelfMessage()){
        compute();
        sendConfig();
        scheduleAt(simTime() + period_, selfSender_);
    }
    else{
        ClusterizePacket* pkt = check_and_cast<ClusterizePacket*>(msg);
        if (pkt == 0)
            throw cRuntimeError("MEClusterizeService::handleMessage - \tFATAL! Error when casting to ClusterizePacket");

        //INFO_CLUSTERIZE_PACKET
        if(!strcmp(pkt->getType(), INFO_CLUSTERIZE)){

            ClusterizeInfoPacket* ipkt = check_and_cast<ClusterizeInfoPacket*>(msg);
            handleClusterizeInfo(ipkt);
        }
        //STOP_CLUSTERIZE_PACKET
       else if(!strcmp(pkt->getType(), STOP_CLUSTERIZE)){

           handleClusterizeStop(pkt);
       }
    }
}
/*
 * ##################################################################################################################################
 */
                                                         //OLD CLUSTERING ALGORITHM
///
//utility comparing function for sort()
//
//ordering the vector of "cluster members": order by highest position w.r.t. the direction
// in this way we propagate messages to the car behind!
//
bool compareOnPosition(const std::map<int, ueLocalInfo>::iterator it1, const std::map<int, ueLocalInfo>::iterator it2){
    double pos1 = it1->second.position.x * std::cos(it1->second.angularPosition.alpha);
    pos1 += it1->second.position.y * std::sin(it1->second.angularPosition.alpha);
    double pos2 = it2->second.position.x * std::cos(it2->second.angularPosition.alpha);
    pos2 += it2->second.position.y * std::sin(it2->second.angularPosition.alpha);

    //testing
    //EV << "MEClusterizeService::computeChainCluster -> compareOnPosition - \t\t" << it1->second.carSimbolicAddress << " pos1: " << pos1 << endl;
    //EV << "MEClusterizeService::computeChainCluster -> compareOnPosition - \t\t" << it2->second.carSimbolicAddress << " pos2: " << pos2 << endl;

    return (pos1 > pos2);
}


/*
 * ################################################################################################################################################
 */
void MEClusterizeService::sendConfig(){

    if(v2vConfig.empty())
        return;

    std::map<int, ueClusterConfig>::iterator it;

    for(it = v2vConfig.begin(); it != v2vConfig.end(); it++){

        ClusterizeConfigPacket* pkt = new ClusterizeConfigPacket(CONFIG_CLUSTERIZE);
        pkt->setTimestamp(simTime());
        pkt->setType(CONFIG_CLUSTERIZE);

        pkt->setClusterID((it->second).clusterID);
        pkt->setTxMode((it->second).txMode);
        pkt->setV2vPeerList((it->second).follower.c_str());

        //testing
        EV << "MEClusterizeService::sendConfig - sending ClusterizeConfigPacket to: " << (it->second).carSimbolicAddress << endl;
        EV << "MEClusterizeService::sendConfig - \t\t\t\tv2vConfig[" << it->first << "].follower: " << it->second.follower << endl;

        //empty follower stringStream
        //(it->second).follower.str("");
        //(it->second).following.str("");
        //(it->second).followingKey = -1;
        //(it->second).followerKey = -1;

        //sending to the MEClusterizeApp (on the corresponded gate!)
        send(pkt, "meClusterizeAppOut", it->first);
    }
}

void MEClusterizeService::handleClusterizeInfo(ClusterizeInfoPacket* pkt){

    int key = pkt->getArrivalGate()->getIndex();

    EV << "MEClusterizeService::handleClusterizeInfo - Updating v2vInfo[" << key <<"] --> " << pkt->getSourceAddress() << pkt->getV2vAppName() << endl;

    v2vInfo[key].carSimbolicAddress = pkt->getSourceAddress();
    v2vInfo[key].position.x = pkt->getPositionX();
    v2vInfo[key].position.y = pkt->getPositionY();
    v2vInfo[key].position.z = pkt->getPositionZ();
    v2vInfo[key].speed.x = pkt->getSpeedX();
    v2vInfo[key].speed.y = pkt->getSpeedY();
    v2vInfo[key].speed.z = pkt->getSpeedZ();
    v2vInfo[key].angularPosition.alpha = pkt->getAngularPositionA();
    v2vInfo[key].angularPosition.beta = pkt->getAngularPositionB();
    v2vInfo[key].angularPosition.gamma = pkt->getAngularPositionC();
    v2vInfo[key].angularSpeed.alpha = pkt->getAngularSpeedA();
    v2vInfo[key].angularSpeed.beta = pkt->getAngularSpeedB();
    v2vInfo[key].angularSpeed.gamma = pkt->getAngularSpeedC();
    v2vInfo[key].checked = false;

    v2vConfig[key].carSimbolicAddress = pkt->getSourceAddress();

    //testing
    EV << "MEClusterizeService::handleClusterizeInfo - v2vInfo[" << key << "].position = " << "[" << v2vInfo[key].position.x << " ; "<< v2vInfo[key].position.y << " ; " << v2vInfo[key].position.z  << "]" << endl;
    EV << "MEClusterizeService::handleClusterizeInfo - v2vInfo[" << key << "].speed = " << "[" << v2vInfo[key].speed.x << " ; "<< v2vInfo[key].speed.y << " ; " << v2vInfo[key].speed.z  << "]" << endl;
    EV << "MEClusterizeService::handleClusterizeInfo - v2vInfo[" << key << "].angularPostion = " << "[" << v2vInfo[key].angularPosition.alpha << " ; "<< v2vInfo[key].angularPosition.beta << " ; " << v2vInfo[key].angularPosition.gamma  << "]" << endl;
    EV << "MEClusterizeService::handleClusterizeInfo - v2vInfo[" << key << "].angularSpeed = " << "[" << v2vInfo[key].angularSpeed.alpha << " ; "<< v2vInfo[key].angularSpeed.beta << " ; " << v2vInfo[key].angularSpeed.gamma  << "]" << endl;
    delete pkt;
}


void MEClusterizeService::handleClusterizeStop(ClusterizePacket* pkt){

    int key = pkt->getArrivalGate()->getIndex();

    EV << "MEClusterizeService::handleClusterizeStop - Erasing v2vInfo[" << key <<"]" << endl;

    if(!v2vInfo.empty() && v2vInfo.find(key) != v2vInfo.end()){

        //erasing the map v2vInfo entry
        //
        std::map<int,ueLocalInfo>::iterator it;
        it = v2vInfo.find(key);
        if (it != v2vInfo.end())
            v2vInfo.erase (it);
    }

    EV << "MEClusterizeService::handleClusterizeStop - Erasing v2vConfig[" << key <<"]" << endl;

    if(!v2vConfig.empty() && v2vConfig.find(key) != v2vConfig.end()){

        //erasing the map v2vConfig entry
        //
        std::map<int,ueClusterConfig>::iterator it;
        it = v2vConfig.find(key);
        if (it != v2vConfig.end())
            v2vConfig.erase (it);
    }

    delete pkt;
}
