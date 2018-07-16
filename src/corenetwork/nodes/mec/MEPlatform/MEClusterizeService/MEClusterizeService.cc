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
    //----------------------------------
    //auto-scheduling
    selfSender_ = new cMessage("selfSender");
    period_ = par("period");
    //----------------------------------
    //parent modules
    mePlatform = getParentModule();
    if(mePlatform != NULL){
        meHost = mePlatform->getParentModule();
        //configuring gate sizes
        int maxMEApps = 0;
        if(meHost->hasPar("maxMEApps"))
                maxMEApps = meHost->par("maxMEApps").longValue();
        else
            throw cRuntimeError("MEClusterizeService::initialize - \tFATAL! Error when getting meHost.maxMEApps parameter!");
        this->setGateSize("meAppOut", maxMEApps);
        this->setGateSize("meAppIn", maxMEApps);
    }
    else{
        EV << "MEClusterizeService::initialize - ERROR getting mePlatform cModule!" << endl;
        throw cRuntimeError("MEClusterizeService::initialize - \tFATAL! Error when getting getParentModule()");
    }
    //----------------------------------
    //txMode information for sending INFO_MEAPP
    preconfiguredTxMode = par("preconfiguredTxMode").stringValue();
    //----------------------------------
    //binder
    binder_ = getBinder();
    //----------------------------------
    //statistics
    stat_ = check_and_cast<MultihopD2DStatistics*>(getModuleByPath("d2dMultihopStatistics"));
    //--------------------------------------
    //starting MEClusterizeService
    simtime_t startTime = par("startTime");
    scheduleAt(simTime() + startTime, selfSender_);
    EV << "MEClusterizeService::initialize - \t starting compute() in " << startTime << " seconds " << endl;
}
/*
 * #################################################################################################################################
 */
void MEClusterizeService::handleMessage(cMessage *msg)
{
    EV << "MEClusterizeService::handleMessage" << endl;
    if (msg->isSelfMessage()){
        compute();
        sendConfig();
        scheduleAt(simTime() + period_, selfSender_);
    }
    else
    {
        ClusterizePacket* pkt = check_and_cast<ClusterizePacket*>(msg);
        if (pkt == 0)
            throw cRuntimeError("MEClusterizeService::handleMessage - \tFATAL! Error when casting to ClusterizePacket");

        if(!strcmp(pkt->getType(), INFO_UEAPP))
        {
            ClusterizeInfoPacket* ipkt = check_and_cast<ClusterizeInfoPacket*>(msg);
            handleClusterizeInfo(ipkt);
        }
        else if(!strcmp(pkt->getType(), STOP_MEAPP))     handleClusterizeStop(pkt);

        delete pkt;
    }
}

/*
 * ################################################################################################################################################
 */
void MEClusterizeService::sendConfig()
{
    if(cars.empty())
        return;

    std::map<int, cluster>::iterator cl_it;
    //DOWNLINK_UNICAST_TX_MODE
    if (!strcmp(preconfiguredTxMode.c_str(), DOWNLINK_UNICAST_TX_MODE))
    {
        for(cl_it = clusters.begin(); cl_it != clusters.end(); cl_it++)
        {
            for(int memberKey : cl_it->second.members)
            {
                ClusterizeConfigPacket* pkt = ClusterizePacketBuilder().buildClusterizeConfigPacket(0, 0, eventID, 1, 0, 0, "", "", cars[memberKey].clusterID, cl_it->second.color.c_str(), cars[memberKey].txMode.c_str(), cars[memberKey].following.c_str(), cars[memberKey].follower.c_str(),cl_it->second.membersList.c_str(), cl_it->second.accelerations);
                //sending to the member MEClusterizeApp (on the corresponded gate!)

                //sending distanceGaps
                int size = cl_it->second.distancies.size();
                pkt->setDistanciesArraySize(size);
                for(int i=0; i < size; i++)     pkt->setDistancies(i, cl_it->second.distancies.at(i));

                send(pkt, "meAppOut", memberKey);
                //testing
                EV << "\nMEClusterizeService::sendConfig - sending ClusterizeConfig to CM: "  << cars[memberKey].symbolicAddress << " (txMode: INFRASTRUCTURE_UNICAST_TX_MODE) " << endl;
                EV << "MEClusterizeService::sendConfig - \t\tclusters[" << cars[memberKey].clusterID << "].membersList: " << cl_it->second.membersList.c_str() << endl;
            }
            //creating global-statistics event record
            std::set<MacNodeId> targetSet;
            for(int memberKey : cl_it->second.members)
                targetSet.insert(cars[memberKey].macID);
            stat_->recordNewBroadcast(eventID, targetSet);
            eventID++;
        }
    }
    //V2V_UNICAST_TX_MODE or  V2V_MULTICAST_TX_MODE
    else if(!strcmp(preconfiguredTxMode.c_str(), V2V_UNICAST_TX_MODE) || !strcmp(preconfiguredTxMode.c_str(), V2V_MULTICAST_TX_MODE))
    {
        for(cl_it = clusters.begin(); cl_it != clusters.end(); cl_it++)
        {
            int leaderKey = cl_it->second.members.at(0);

            ClusterizeConfigPacket* pkt = ClusterizePacketBuilder().buildClusterizeConfigPacket(0, 0, eventID, 1, 0, 0, "", "", cars[leaderKey].clusterID, cl_it->second.color.c_str(), cars[leaderKey].txMode.c_str(), cars[leaderKey].following.c_str(), cars[leaderKey].follower.c_str(),cl_it->second.membersList.c_str(), cl_it->second.accelerations);
            //sending to the leader MEClusterizeApp (on the corresponded gate!)

            //sending distanceGaps
            int size = cl_it->second.distancies.size();
            pkt->setDistanciesArraySize(size);
            for(int i=0; i < size; i++)     pkt->setDistancies(i, cl_it->second.distancies.at(i));

            send(pkt, "meAppOut", leaderKey);

            //testing
            std::string txmode = (!strcmp(preconfiguredTxMode.c_str(), V2V_UNICAST_TX_MODE))? V2V_UNICAST_TX_MODE : V2V_MULTICAST_TX_MODE;
            EV << "\nMEClusterizeService::sendConfig - sending ClusterizeConfig to CL: " << cars[leaderKey].symbolicAddress << " (txMode: "<< txmode << ") " << endl;
            EV << "MEClusterizeService::sendConfig - \t\tclusters[" << cars[leaderKey].clusterID << "].membersList: " << cl_it->second.membersList.c_str() << endl << endl;

            //creating global-statistics event record
            std::set<MacNodeId> targetSet;
            for(int memberKey : cl_it->second.members)
                targetSet.insert(cars[memberKey].macID);
            stat_->recordNewBroadcast(eventID,targetSet);
            eventID++;
        }
    }
    //HYBRID_TX_MODE
    else
    {
        //TODO
        //for each cluster in clusters map (cl : clusters)
                //for each cluster member (c : cl.members)
                        //check its txMode (cars[c].txMode) computed by the compute() implementing the algorithm
                        //i.e. if DOWNLINK_UNICAST then send to it
                        //i.e. if V2V_UNICAST or V2V_MULTICAST then to it only if it is the leader!
        //TODO
    }

    // triggering the MEClusterize App to emit statistics (also if the INFO_MEAPP ClsuterizeConfigPacket is sent only to the leader and then propagated!)
    for(cl_it = clusters.begin(); cl_it != clusters.end(); cl_it++)
        for(int memberKey : cl_it->second.members)
        {
            cMessage* trigger = new cMessage("triggerClusterizeConfigStatistics");
            send(trigger, "meAppOut", memberKey);
        }
}

void MEClusterizeService::handleClusterizeInfo(ClusterizeInfoPacket* pkt){

    if(pkt->getTimestamp() >= lastRun)
    {
        //retrieve the cars map key
        int key = pkt->getArrivalGate()->getIndex();
        //updating the cars map entry
        cars[key].id = pkt->getCarOmnetID();
        cars[key].symbolicAddress = pkt->getSourceAddress();
        cars[key].macID = binder_->getMacNodeIdFromOmnetId(cars[key].id);
        cars[key].timestamp = pkt->getTimestamp();
        cars[key].position.x = pkt->getPositionX();
        cars[key].position.y = pkt->getPositionY();
        cars[key].position.z = pkt->getPositionZ();
        cars[key].speed.x = pkt->getSpeedX();
        cars[key].speed.y = pkt->getSpeedY();
        cars[key].speed.z = pkt->getSpeedZ();
        cars[key].acceleration = pkt->getAcceleration();
        cars[key].angularPosition.alpha = pkt->getAngularPositionA();
        cars[key].angularPosition.beta = pkt->getAngularPositionB();
        cars[key].angularPosition.gamma = pkt->getAngularPositionC();
        cars[key].angularSpeed.alpha = pkt->getAngularSpeedA();
        cars[key].angularSpeed.beta = pkt->getAngularSpeedB();
        cars[key].angularSpeed.gamma = pkt->getAngularSpeedC();
        cars[key].isFollower = false;

        //testing
        EV << "MEClusterizeService::handleClusterizeInfo - Updating cars[" << key <<"] --> " << cars[key].symbolicAddress << " (carID: "<< cars[key].id << ") " << endl;
        EV << "MEClusterizeService::handleClusterizeInfo - cars[" << key << "].position = " << "[" << cars[key].position.x << " ; "<< cars[key].position.y << " ; " << cars[key].position.z  << "]" << endl;
        EV << "MEClusterizeService::handleClusterizeInfo - cars[" << key << "].speed = " << "[" << cars[key].speed.x << " ; "<< cars[key].speed.y << " ; " << cars[key].speed.z  << "]" << endl;
        EV << "MEClusterizeService::handleClusterizeInfo - cars[" << key << "].acceleration = " << cars[key].acceleration << endl;
        EV << "MEClusterizeService::handleClusterizeInfo - cars[" << key << "].angularPostion = " << "[" << cars[key].angularPosition.alpha << " ; "<< cars[key].angularPosition.beta << " ; " << cars[key].angularPosition.gamma  << "]" << endl;
        EV << "MEClusterizeService::handleClusterizeInfo - cars[" << key << "].angularSpeed = " << "[" << cars[key].angularSpeed.alpha << " ; "<< cars[key].angularSpeed.beta << " ; " << cars[key].angularSpeed.gamma  << "]" << endl;
    }
    else
    {
        EV << "MEClusterizeService::handleClusterizeInfo - Discarding update from " << pkt->getSourceAddress() << ": too old time-stamp!" << endl;
    }
}

void MEClusterizeService::handleClusterizeStop(ClusterizePacket* pkt){

    //retrieve the cars map key
    int key = pkt->getArrivalGate()->getIndex();
    //erasing the cars map entry
    if(!cars.empty() && cars.find(key) != cars.end())
    {
        std::map<int, car>::iterator it;
        it = cars.find(key);
        if (it != cars.end())
            cars.erase (it);
    }
    //testing
    EV << "MEClusterizeService::handleClusterizeStop - Erasing cars[" << key <<"]" << endl;
}
