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

    double reqRam = pkt->getRequiredRam();
    double reqDisk = pkt->getRequiredDisk();
    double reqCpu = pkt->getRequiredCpu();
    /* -------------------------------
     * Handling StartPacket */
    if(!strcmp(pkt->getType(), START_MEAPP))
    {
        availableResources = ((maxRam-allocatedRam-reqRam >= 0) && (maxDisk-allocatedDisk-reqDisk >= 0) && (maxCPU-allocatedCPU-reqCpu >= 0))? true : false;
        if(availableResources)
        {
            EV << "ResourceManager::handleMEAppResources - resources ALLOCATED for " << pkt->getSourceAddress() << endl;
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
        EV << "ResourceManager::handleMEAppResources - resources DEALLOCATED for " << pkt->getSourceAddress() << endl;
        deallocateResources(reqRam, reqDisk, reqCpu);
        send(pkt, "virtualisationManagerOut");
    }
}
