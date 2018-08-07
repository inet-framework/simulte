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

}

void ResourceManager::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;

    MEAppPacket* mepkt = check_and_cast<MEAppPacket*>(msg);
    if(mepkt == 0)
    {
        EV << "VirtualisationManager::handleMessage - \tFATAL! Error when casting to MEAppPacket" << endl;
        throw cRuntimeError("VirtualisationManager::handleMessage - \tFATAL! Error when casting to MEAppPacket");
    }
    /*
     * DEFAULT HANDLING RESOURCES REQUIRED BY THE MEAppPacket
     */
    else{
        handleMEAppResources(mepkt);
    }
}

void ResourceManager::finish(){

}

void ResourceManager::handleMEAppResources(MEAppPacket* pkt){

    EV << "ResourceManager::handleMEAppResources - "<< pkt->getType() << " received: "<< pkt->getSourceAddress() <<" SeqNo[" << pkt->getSno() << "]"<< endl;

    bool availableResources = true;

    int ueAppID = pkt->getUeAppID();
    /* -------------------------------
     * Handling StartPacket */
    if(!strcmp(pkt->getType(), START_MEAPP))
    {
        double reqRam = pkt->getRequiredRam();
        double reqDisk = pkt->getRequiredDisk();
        double reqCpu = pkt->getRequiredCpu();
        availableResources = ((maxRam-allocatedRam-reqRam >= 0) && (maxDisk-allocatedDisk-reqDisk >= 0) && (maxCPU-allocatedCPU-reqCpu >= 0))? true : false;
        if(availableResources)
        {
            //storing informations about ME App allocated resources
            resourceMap[ueAppID].ueAppID = ueAppID;
            resourceMap[ueAppID].ram = reqRam;
            resourceMap[ueAppID].disk = reqDisk;
            resourceMap[ueAppID].cpu = reqCpu;
            EV << "ResourceManager::handleMEAppResources - resources ALLOCATED for " << pkt->getSourceAddress() << endl;
            EV << "ResourceManager::handleMEAppResources - ram: " << resourceMap[ueAppID].ram <<" disk: "<< resourceMap[ueAppID].disk <<" cpu: "<< resourceMap[ueAppID].cpu << endl;
            allocateResources(reqRam, reqDisk, reqCpu);
            send(pkt, "virtualisationManagerOut");
        }
        else{
            EV << "ResourceManager::handleMEAppResources - resources NOT AVAILABLE for " << pkt->getSourceAddress() << endl;
            delete(pkt);
        }
    }
    /* -------------------------------
     * Handling StopPacket */
    else if(!strcmp(pkt->getType(), STOP_MEAPP))
    {
        if(!resourceMap.empty() || resourceMap.find(ueAppID) != resourceMap.end())
        {
            EV << "ResourceManager::handleMEAppResources - resources DEALLOCATED for " << pkt->getSourceAddress() << endl;
            EV << "ResourceManager::handleMEAppResources - ram: " << resourceMap[ueAppID].ram <<" disk: "<< resourceMap[ueAppID].disk <<" cpu: "<< resourceMap[ueAppID].cpu << endl;
            deallocateResources(resourceMap[ueAppID].ram, resourceMap[ueAppID].disk, resourceMap[ueAppID].cpu);
            //erasing map entry
            resourceMap.erase(ueAppID);
            send(pkt, "virtualisationManagerOut");
        }
        else
        {
            EV << "ResourceManager::handleMEAppResources - NO ALLOCATION FOUND for " << pkt->getSourceAddress() << endl;
        }
    }
}
