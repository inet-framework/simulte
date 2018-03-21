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
        if(mePlatform->gateSize("meClusterizeAppOut") == 0 || mePlatform->gateSize("meClusterizeAppIn") == 0){
            mePlatform->setGateSize("meClusterizeAppOut", maxMEApps);
            mePlatform->setGateSize("meClusterizeAppIn", maxMEApps);
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

void MEClusterizeService::handleMessage(cMessage *msg)
{
    EV << "MEClusterizeService::handleMessage - " << endl;

    if (msg->isSelfMessage()){
        compute();
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

void MEClusterizeService::compute(){

    EV << "MEClusterizeService::compute - starting..." << endl;

    //implementing the algorithm & updating the v2vInfo & v2vConfig maps!                                           //TODO

    ueLocalInfo info1, info2;

    //iterating on the v2vInfo map
    //
    std::map<int, ueLocalInfo>::iterator it1, it2;

    if(!v2vInfo.empty()){

        EV << "MEClusterizeService::compute - computing..." << endl;

        it1 = v2vInfo.begin();


        int key1 = (int)it1->first;
        int key2;

        while(it1 != v2vInfo.end()){

            info1 = it1->second;
            key1 = (int)it1->first;

            EV << "MEClusterizeService::compute - clusterizing: " << info1.carSimbolicAddress << endl;

            //
            //creare le entry nella v2vConfig map & nel ciclo successivo inviare                                    //TODO
            //

            ClusterizeConfigPacket* pkt = new ClusterizeConfigPacket(CONFIG_CLUSTERIZE);
            pkt->setTimestamp(simTime());
            pkt->setType(CONFIG_CLUSTERIZE);

            //pkt->setV2vPeerList(info2->getSourceAddress());

            pkt->setV2vPeerList("");

            pkt->setTxMode(UNICAST_TX_MODE);
            pkt->setClusterID(0);

            send(pkt, "meClusterizeAppOut", key1);

            it1++;
        }
    }

    scheduleAt(simTime() + period_, selfSender_);
}

void MEClusterizeService::sendConfig(){

}

void MEClusterizeService::handleClusterizeInfo(ClusterizeInfoPacket* pkt){

    int key = pkt->getArrivalGate()->getIndex();

    EV << "MEClusterizeService::handleClusterizeInfo - Updating v2vInfo[" << key <<"] --> " << pkt->getSourceAddress() << pkt->getV2vAppName() << endl;

    v2vInfo[key].carSimbolicAddress = pkt->getSourceAddress();
    v2vInfo[key].position.x = pkt->getPositionX();
    v2vInfo[key].position.y = pkt->getPositionY();
    v2vInfo[key].speed.x = pkt->getSpeedX();
    v2vInfo[key].speed.y = pkt->getSpeedY();

    //testing
    EV << "MEClusterizeService::handleClusterizeInfo - v2vInfo[" << key << "].position = " << "[" << v2vInfo[key].position.x << " ; "<< v2vInfo[key].position.y << " ; " << v2vInfo[key].position.z  << "]" << endl;
    EV << "MEClusterizeService::handleClusterizeInfo - v2vInfo[" << key << "].speed = " << "[" << v2vInfo[key].speed.x << " ; "<< v2vInfo[key].speed.y << " ; " << v2vInfo[key].speed.z  << "]" << endl;

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

    delete pkt;
}
