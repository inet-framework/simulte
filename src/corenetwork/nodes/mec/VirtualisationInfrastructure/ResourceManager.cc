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


#include "ResourceManager.h"

Define_Module(ResourceManager);

ResourceManager::ResourceManager(){

    maxRam = 0;
    maxDisk = 0;
    maxCPU = 0;

    allocatedRam = 0;
    allocatedDisk = 0;
    allocatedCPU = 0;
}

void ResourceManager::initialize(int stage)
{
    EV << "ResourceManager::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    virtualisationInfr = getParentModule();

    if(virtualisationInfr != NULL)
        meHost = virtualisationInfr->getParentModule();
    if(meHost != NULL){
        maxRam = meHost->par("maxRam").doubleValue();
        maxDisk = meHost->par("maxDisk").doubleValue();
        maxCPU = meHost->par("maxCpu").doubleValue();
    }

    //creating a MEClusterizeApp module to get parameters about resource allocation
    cModuleType *moduleType = cModuleType::get("lte.apps.vehicular.mec.clusterize.MEClusterizeApp");
    cModule *module = moduleType->create("MEClusterizeApp", this);     //name & its Parent Module
    templateMEClusterizeApp = check_and_cast<MEClusterizeApp*>(module);
    templateMEClusterizeApp->finalizeParameters();
}

void ResourceManager::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;
    /*
     * Handling Resources for the clusterizing service: MEClusterizeApp
     */
    ClusterizePacket* pkt = check_and_cast<ClusterizePacket*>(msg);
    if (pkt == 0)
        throw cRuntimeError("ResourceManager::handleMessage - \tFATAL! Error when casting to ClusterizePacket");
    else
        handleClusterizeResources(pkt);
}

void ResourceManager::finish(){

    templateMEClusterizeApp->deleteModule();
}

/*
 * #######################################CLUSTERIZE PACKETS HANDLERS####################################
 */

void ResourceManager::handleClusterizeResources(ClusterizePacket* pkt){

    EV << "ResourceManager::handleClusterize - "<< pkt->getType() << " received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]"<< endl;

    bool availableResources = true;

    double reqRam = templateMEClusterizeApp->par("ram").doubleValue();
    double reqDisk = templateMEClusterizeApp->par("disk").doubleValue();
    double reqCPU = templateMEClusterizeApp->par("cpu").doubleValue();

    /* -------------------------------
     * Handling ClusterizeStartPacket */
    if(!strcmp(pkt->getType(), START_CLUSTERIZE)){

        availableResources = ((maxRam-allocatedRam-reqRam >= 0) && (maxDisk-allocatedDisk-reqDisk >= 0) && (maxCPU-allocatedCPU-reqCPU >= 0))? true : false;

        if(availableResources){
            allocatedRam += reqRam;
            allocatedDisk += reqDisk;
            allocatedCPU += reqCPU;
            EV << "ResourceManager::handleClusterizeResources - resources allocated for " << pkt->getSourceAddress() << endl;
            //Sending back the START_CLUSTERIZE packet to Virtualisation Manager to instantiate the MEClusterizeApp & ACK_START
            send(pkt, "virtualisationManagerOut");
        }
        else{
            EV << "ResourceManager::handleClusterizeResources - resources NOT AVAILABLE for " << pkt->getSourceAddress() << endl;
            delete(pkt);
        }
    }
    /* -------------------------------
     * Handling ClusterizeStopPacket */
    else if(!strcmp(pkt->getType(), STOP_CLUSTERIZE)){

        allocatedRam -= reqRam;
        allocatedDisk -= reqDisk;
        allocatedCPU -= reqCPU;

        EV << "ResourceManager::handleClusterizeResources - resources deallocated for " << pkt->getSourceAddress() << endl;

        //Sending back the STOP_CLUSTERIZE packet to Virtualisation Manager to ACK_STOP
        send(pkt, "virtualisationManagerOut");
    }

    EV << "ResourceManager::handleClusterizeResources - allocated Ram: " << allocatedRam << " / " << maxRam << endl;
    EV << "ResourceManager::handleClusterizeResources - allocated Disk: " << allocatedDisk << " / " << maxDisk << endl;
    EV << "ResourceManager::handleClusterizeResources - allocated CPU: " << allocatedCPU << " / " << maxCPU << endl;
}
/*
 * ##############################################################################################################
 */

