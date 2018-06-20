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

    colors.push_back("black");
    colors.push_back("grey");
    colors.push_back("brown");
    colors.push_back("purple");
    colors.push_back("magenta");
    colors.push_back("red");
    colors.push_back("orange");
    colors.push_back("yellow");
    colors.push_back("green");
    colors.push_back("olive");
    colors.push_back("cyan");
    colors.push_back("blue");
    colors.push_back("navy");
    colors.push_back("violet");

    eventID = 0;
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

    preconfiguredTxMode = par("preconfiguredTxMode").longValue();

    binder_ = getBinder();

    // global statistics recorder
    stat_ = check_and_cast<MultihopD2DStatistics*>(getModuleByPath("d2dMultihopStatistics"));

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
 * ################################################################################################################################################
 */
void MEClusterizeService::sendConfig(){

    if(cars.empty())
        return;

    std::map<int, cluster>::iterator cl_it;

    //sending directly to all the cars the configuration computed
    if (preconfiguredTxMode == INFRASTRUCTURE_UNICAST_TX_MODE){

        for(cl_it = clusters.begin(); cl_it != clusters.end(); cl_it++){

            for(int memberKey : cl_it->second.members){

                ClusterizeConfigPacket* pkt = ClusterizePacketBuilder().buildClusterizeConfigPacket(0, 0, eventID, 1, 0, 0, "", "", cars[memberKey].clusterID, cl_it->second.color.c_str(), cars[memberKey].txMode, cars[memberKey].following.c_str(), cars[memberKey].follower.c_str(),cl_it->second.membersList.c_str(), cl_it->second.accelerations);

                //testing
                EV << "MEClusterizeService::sendConfig - sending ClusterizeConfig to CM: "  << cars[memberKey].simbolicAddress << " (txMode: INFRASTRUCTURE_UNICAST_TX_MODE) " << endl;
                EV << "MEClusterizeService::sendConfig - \t\tcars[" << memberKey << "].clusterID: " << cars[memberKey].clusterID << endl;
                EV << "MEClusterizeService::sendConfig - \t\tclusters[" << cars[memberKey].clusterID << "].membersList: " << cl_it->second.membersList.c_str() << endl;

                //sending to the MEClusterizeApp (on the corresponded gate!)
                send(pkt, "meClusterizeAppOut", memberKey);
            }
            //creating global-statistics event record
            std::set<MacNodeId> targetSet;
            for(int memberKey : cl_it->second.members)
                targetSet.insert(cars[memberKey].macID);
            stat_->recordNewBroadcast(eventID, targetSet);
            eventID++;
        }
    }
    //sending directly only to the cluster leader
    else if(preconfiguredTxMode == V2V_UNICAST_TX_MODE || preconfiguredTxMode == V2V_MULTICAST_TX_MODE){

        for(cl_it = clusters.begin(); cl_it != clusters.end(); cl_it++){

            int leaderKey = cl_it->second.members.at(0);

            ClusterizeConfigPacket* pkt = ClusterizePacketBuilder().buildClusterizeConfigPacket(0, 0, eventID, 1, 0, 0, "", "", cars[leaderKey].clusterID, cl_it->second.color.c_str(), cars[leaderKey].txMode, cars[leaderKey].following.c_str(), cars[leaderKey].follower.c_str(),cl_it->second.membersList.c_str(), cl_it->second.accelerations);

            //testing
            std::string txmode = (preconfiguredTxMode == V2V_UNICAST_TX_MODE)? "V2V_UNICAST_TX_MODE" : "V2V_MULTICAST_TX_MODE";
            EV << "MEClusterizeService::sendConfig - sending ClusterizeConfig to CL: " << cars[leaderKey].simbolicAddress << " (txMode: "<< txmode << ") " << endl;
            EV << "MEClusterizeService::sendConfig - \t\tcars[" << leaderKey << "].clusterID: " << cars[leaderKey].clusterID  << endl;
            EV << "MEClusterizeService::sendConfig - \t\tclusters[" << cars[leaderKey].clusterID << "].membersList: " << cl_it->second.membersList.c_str() << endl << endl;

            //sending to the MEClusterizeApp (on the corresponded gate!)
            send(pkt, "meClusterizeAppOut", leaderKey);

            //creating global-statistics event record
            std::set<MacNodeId> targetSet;
            for(int memberKey : cl_it->second.members)
                targetSet.insert(cars[memberKey].macID);
            stat_->recordNewBroadcast(eventID,targetSet);
            eventID++;
        }
    }
    else{
        //hybrid approach!
        //check for every car in the clusters the txMode choosen!
    }

    //
    // triggering the MEClusterize App to emit statistics!
    for(cl_it = clusters.begin(); cl_it != clusters.end(); cl_it++){

        for(int memberKey : cl_it->second.members){

            cMessage* trigger = new cMessage("triggerClusterizeConfigStatistics");
            send(trigger, "meClusterizeAppOut", memberKey);
        }
    }
}

void MEClusterizeService::handleClusterizeInfo(ClusterizeInfoPacket* pkt){

    int key = pkt->getArrivalGate()->getIndex();

    cars[key].id = pkt->getCarID();
    cars[key].simbolicAddress = pkt->getSourceAddress();
    cars[key].macID = binder_->getMacNodeIdFromOmnetId(cars[key].id);
    cars[key].position.x = pkt->getPositionX();
    cars[key].position.y = pkt->getPositionY();
    cars[key].position.z = pkt->getPositionZ();
    cars[key].speed.x = pkt->getSpeedX();
    cars[key].speed.y = pkt->getSpeedY();
    cars[key].speed.z = pkt->getSpeedZ();
    cars[key].angularPosition.alpha = pkt->getAngularPositionA();
    cars[key].angularPosition.beta = pkt->getAngularPositionB();
    cars[key].angularPosition.gamma = pkt->getAngularPositionC();
    cars[key].angularSpeed.alpha = pkt->getAngularSpeedA();
    cars[key].angularSpeed.beta = pkt->getAngularSpeedB();
    cars[key].angularSpeed.gamma = pkt->getAngularSpeedC();
    cars[key].isFollower = false;

    EV << "MEClusterizeService::handleClusterizeInfo - Updating cars[" << key <<"] --> " << cars[key].simbolicAddress << " (carID: "<< cars[key].id << ") " << endl;

    //testing
    EV << "MEClusterizeService::handleClusterizeInfo - cars[" << key << "].position = " << "[" << cars[key].position.x << " ; "<< cars[key].position.y << " ; " << cars[key].position.z  << "]" << endl;
    EV << "MEClusterizeService::handleClusterizeInfo - cars[" << key << "].speed = " << "[" << cars[key].speed.x << " ; "<< cars[key].speed.y << " ; " << cars[key].speed.z  << "]" << endl;
    EV << "MEClusterizeService::handleClusterizeInfo - cars[" << key << "].angularPostion = " << "[" << cars[key].angularPosition.alpha << " ; "<< cars[key].angularPosition.beta << " ; " << cars[key].angularPosition.gamma  << "]" << endl;
    EV << "MEClusterizeService::handleClusterizeInfo - cars[" << key << "].angularSpeed = " << "[" << cars[key].angularSpeed.alpha << " ; "<< cars[key].angularSpeed.beta << " ; " << cars[key].angularSpeed.gamma  << "]" << endl;
    delete pkt;
}


void MEClusterizeService::handleClusterizeStop(ClusterizePacket* pkt){

    int key = pkt->getArrivalGate()->getIndex();

    EV << "MEClusterizeService::handleClusterizeStop - Erasing v2vInfo[" << key <<"]" << endl;

    if(!cars.empty() && cars.find(key) != cars.end()){

        //erasing the map cars entry
        //
        std::map<int, car>::iterator it;
        it = cars.find(key);
        if (it != cars.end())
            cars.erase (it);
    }

    EV << "MEClusterizeService::handleClusterizeStop - Erasing cars[" << key <<"]" << endl;

    if(!cars.empty() && cars.find(key) != cars.end()){

        //erasing the map cars entry
        //
        std::map<int, car>::iterator it;
        it = cars.find(key);
        if (it != cars.end())
            cars.erase (it);
    }

    delete pkt;
}
