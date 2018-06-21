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


#include "VirtualisationManager.h"

Define_Module(VirtualisationManager);

void VirtualisationManager::initialize(int stage)
{
    EV << "VirtualisationManager::initialize - stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    // get reference to the binder
    binder_ = getBinder();

    destPort_ = par("destPort").longValue();
    localPort_ = par("localPort").longValue();

    EV << "VirtualisationManager::initialize - binding to port: local:" << localPort_ << " , dest:" << destPort_ << endl;
    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort_);

    virtualisationInfr = getParentModule();

    //initializing VirtualisationInfrastructure gates & connections
    if(virtualisationInfr != NULL){
        meHost = virtualisationInfr->getParentModule();
        if(meHost->hasPar("maxMEApps"))
                maxMEApps = meHost->par("maxMEApps").longValue();
        else
            throw cRuntimeError("VirtualisationManager::initialize - \tFATAL! Error when getting meHost.maxMEApps parameter!");

        // VirtualisationInfrastructure internal gate connections with VirtualisationManager
        virtualisationInfr->setGateSize("meAppOut", maxMEApps);
        virtualisationInfr->setGateSize("meAppIn", maxMEApps);
        this->setGateSize("meAppOut", maxMEApps);
        this->setGateSize("meAppIn", maxMEApps);
        for(int index = 0; index < maxMEApps; index++){
            this->gate("meAppOut", index)->connectTo(virtualisationInfr->gate("meAppOut", index));
            virtualisationInfr->gate("meAppIn", index)->connectTo(this->gate("meAppIn", index));
        }
    }
    else{
        EV << "VirtualisationManager::initialize - ERROR getting virtualisationInfrastructure cModule!" << endl;
        throw cRuntimeError("VirtualisationManager::initialize - \tFATAL! Error when getting getParentModule()");
    }
    currentMEApps = 0;

    //initializing MEPlatform Services
    if(meHost != NULL){
        mePlatform = meHost->getSubmodule("mePlatform");
        if(mePlatform == NULL){
            EV << "VirtualisationManager::initialize - ERROR getting mePlatform cModule!" << endl;
            throw cRuntimeError("VirtualisationManager::initialize - \tFATAL! Error when getting meHost->getSubmodule()");
        }

        // retrieving all available services
        numServices = mePlatform->par("numServices").longValue();
        for(int i=0; i<numServices; i++){
            meServices.push_back(mePlatform->getSubmodule("udpService", i));

            EV << "VirtualisationManager::initialize - Available meServices["<<i<<"] " << meServices.at(i)->getClassName() << endl;
        }
    }

    interfaceTableModule = par("interfaceTableModule").stringValue();

    for(int i = 0; i < maxMEApps; i++)
        freeGates.push_back(i);
}

void VirtualisationManager::handleMessage(cMessage *msg)
{
    EV << "VirtualisationManager::handleMessage" << endl;

    if (msg->isSelfMessage())
        return;

    if(msg->arrivedOn("resourceManagerIn")){

        handleResource(msg);
    }
    else{
        MEAppPacket* mePkt = check_and_cast<MEAppPacket*>(msg);
        if(mePkt == 0){
            EV << "VirtualisationManager::handleMessage - \tFATAL! Error when casting to MEAppPacket" << endl;
            throw cRuntimeError("VirtualisationManager::handleMessage - \tFATAL! Error when casting to MEAppPacket");
        }
        else
            handleMEAppPacket(mePkt);
    }
}

/*
 * ###################################RESOURCE MANAGER decision HANDLER#################################
 */

void VirtualisationManager::handleResource(cMessage* msg){

    MEAppPacket* mePkt = check_and_cast<MEAppPacket*>(msg);
    if(mePkt == 0){
        EV << "VirtualisationManager::handleMessage - \tFATAL! Error when casting to MEAppPacket" << endl;
        throw cRuntimeError("VirtualisationManager::handleMessage - \tFATAL! Error when casting to MEAppPacket");
    }
    /*
     * HANDLING CLUSTERIZE PACKETS for RESOURCES ALLOCATION/DEALLOCATION
     */
    else if(!strcmp(mePkt->getType(), START_MEAPP)){

        EV << "VirtualisationManager::handleMessage - calling instantiateMEClusterizeApp" << endl;
        instantiateMEApp(mePkt);
    }
    else if(!strcmp(mePkt->getType(), STOP_MEAPP)){

        //Sending ACK to the UEClusterizeApp
        EV << "VirtualisationManager::handleResource - calling terminateMEClusterizeApp" << endl;
        terminateMEApp(mePkt);
    }
}
/*
 * ######################################################################################################
 */

/*
 * #######################################CLUSTERIZE PACKETS HANDLERS####################################
 */

void VirtualisationManager::handleMEAppPacket(MEAppPacket* pkt){

    EV << "VirtualisationManager::handleClusterize - received "<< pkt->getType()<<" Delay: " << (simTime()-pkt->getTimestamp()) << endl;

    /* -------------------------------
     * Handling ClusterizeStartPacket */
    if(!strcmp(pkt->getType(), START_MEAPP))        startMEApp(pkt);

    /* -------------------------------
     * Handling ClusterizeInfoPacket */
    else if(!strcmp(pkt->getType(), INFO_UEAPP))    upstreamToMEApp(pkt);

    /* -------------------------------
     * Handling ClusterizeConfigPacket */
    else if(!strcmp(pkt->getType(), INFO_MEAPP))    downstreamToUEApp(pkt);

    /* -------------------------------
     * Handling ClusterizeStopPacket */
    else if(!strcmp(pkt->getType(), STOP_MEAPP))    stopMEApp(pkt);

}

void VirtualisationManager::startMEApp(MEAppPacket* pkt){

    EV << "VirtualisationManager::startMEApp - processing..." << endl;

    //checking if the service required is available
    if(findService(pkt->getRequiredService()) == SERVICE_NOT_AVAILABLE){
        EV << "VirtualisationManager::startMEApp - Service required is not available: " << pkt->getRequiredService() << endl;
        throw cRuntimeError("VirtualisationManager::startMEApp - \tFATAL! Service required is not available!" );
        return;
    }

    //creating the map key
    std::stringstream key;
    key << pkt->getSourceAddress();

    //checking if there are available slots for MEClusterizeApp && the MEClusterizeApp is not already instantiated!
    if(currentMEApps < maxMEApps && meAppMapTable.find(key.str()) == meAppMapTable.end()){

        EV << "VirtualisationManager::startMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;
        //Checking for resource availability to the Resource Manager
        EV << "VirtualisationManager::startMEApp - forwarding " << pkt->getType() << " to Resource Manager!" << endl;
        send(pkt, "resourceManagerOut");
    }
    else{
        if(meAppMapTable.find(key.str()) != meAppMapTable.end()){

            EV << "VirtualisationManager::startMEApp - \tWARNING: MEClusterizeApp ALREADY STARTED!" << endl;
            //Sending ACK to the UEClusterizeApp to confirm the instantiation in case of previous ack lost!
            EV << "VirtualisationManager::startClusterize  - calling ackClusterize with  "<< ACK_START_MEAPP << endl;
            ackMEAppPacket(pkt, ACK_START_MEAPP);
        }else
            EV << "VirtualisationManager::startMEApp - \tWARNING: maxMEApp LIMIT REACHED!" << endl;
    }
}


void VirtualisationManager::upstreamToMEApp(MEAppPacket* pkt){

    EV << "VirtualisationManager::upstreamToMEApp - processing..."<< endl;

    //creating the map key
    std::stringstream key;
    key << pkt->getSourceAddress();

    if(!meAppMapTable.empty() && meAppMapTable.find(key.str()) != meAppMapTable.end()){

        send(pkt, "meAppOut", meAppMapTable[key.str()].meAppGateIndex);

        EV << "VirtualisationManager::ToMEApp - forwarding " << pkt->getName() << " to meAppOut["<< meAppMapTable[key.str()].meAppGateIndex << "] gate" << endl;
    }
    else{

        EV << "VirtualisationManager::upstreamToMEApp - \tWARNING: NO " << pkt->getMEModuleName() << " INSTANCE FOUND!" << endl;
    }
}

void VirtualisationManager::downstreamToUEApp(MEAppPacket* pkt){

    //creating the map key
    std::stringstream key;
    key << pkt->getDestinationAddress();

    const char* destSimbolicAddr = pkt->getDestinationAddress();

    if(!meAppMapTable.empty() && meAppMapTable.find(key.str()) != meAppMapTable.end()){

        destAddress_ = meAppMapTable[key.str()].ueAddress;

        EV << "VirtualisationManager::downstreamToUEApp - forwarding" << pkt->getName() << " to "<< destSimbolicAddr << ": [" << destAddress_.str() <<"]" << endl;

        //checking if the Car is in the network & sending by socket
        MacNodeId destId = binder_->getMacNodeId(destAddress_.toIPv4());
        if(destId == 0){
            EV << "VirtualisationeManager::downstreamToUEApp - \tWARNING " << destSimbolicAddr << "has left the network!" << endl;
            //throw cRuntimeError("VirtualisationManager::downstreamClusterize - \tFATAL! Error destination has left the network!");

            //starting the MEClusterizeApp termination procedure
            //creating the STOP_CLUSTERIZE ClusterizePacket
            MEAppPacket* spkt = new MEAppPacket();
            spkt->setType(STOP_MEAPP);
            spkt->setSno(pkt->getSno());
            spkt->setByteLength(pkt->getByteLength());

            spkt->setSourceAddress(pkt->getSourceAddress());
            spkt->setMEModuleName(pkt->getMEModuleName());

            //
            EV << "VirtualisationeManager::downstreamToUEApp - calling stopMEApp for " << destSimbolicAddr << endl;
            //calling the stopClusterize to handle the MEClusterizeApp termination
            //due to correspondent Car exit the network
            stopMEApp(spkt);

            delete(pkt);
        }
        else
            socket.sendTo(pkt, destAddress_, destPort_);
    }
    else
        EV << "VirtualisationeManager::downstreamToUEApp - \tWARNING forwarding to "<< destSimbolicAddr << ": map entry "<< key.str() <<" not found!" << endl;
}


void VirtualisationManager::stopMEApp(MEAppPacket* pkt){

    EV << "VirtualisationManager::stopMEApp - processing..." << endl;

    //creating the map key
    std::stringstream key;
    key << pkt->getSourceAddress();

    if(!meAppMapTable.empty() && meAppMapTable.find(key.str()) != meAppMapTable.end()){

        //Asking to free resources  to the Resource Manager
        EV << "VirtualisationManager::stopMEApp - forwarding "<< pkt->getType() << " to Resource Manager!" << endl;
        send(pkt, "resourceManagerOut");
    }
    else{
        EV << "VirtualisationManager::stopMEApp - \tWARNING: NO MEClusterizeApp INSTANCE FOUND!" << endl;
    }
}

void VirtualisationManager::instantiateMEApp(MEAppPacket* pkt){

    int serviceIndex = findService(pkt->getRequiredService());

    //creating the map key
    std::stringstream key;
    key << pkt->getSourceAddress();

    if(currentMEApps < maxMEApps &&  meAppMapTable.find(key.str()) == meAppMapTable.end()){

        EV << "VirtualisationManager::instantiateMEApp - key: " << key.str() << endl;

        //getting the first available gates
        int index = freeGates.front();
        freeGates.erase(freeGates.begin());
        EV << "VirtualisationManager::instantiateMEApp - freeGate: " << index << endl;

        //getting the UEClusterizeApp L3Address
        inet::L3Address ueAppAddress = inet::L3AddressResolver().resolve(pkt->getSourceAddress());
        EV << "VirtualisationManager::instantiateMEApp - UEAppL3Address: " << ueAppAddress.str() << endl;

        // creating MEClusterizeApp instance
        cModuleType *moduleType = cModuleType::get(pkt->getMEModuleType());
        cModule *module = moduleType->create(pkt->getMEModuleName(), meHost);     //name & its Parent Module
        std::stringstream appName;
        appName << pkt->getMEModuleName() << "[" <<  key.str().c_str() << "]";
        module->setName(appName.str().c_str());

        //displaying ME App dynamically created (after 70 they will overlap..)
        std::stringstream display;
        display << "p=" << (50 + ((index%10)*100)%1000) << "," << (100 + (50*(index/10)%350)) << ";is=vs";
        module->setDisplayString(display.str().c_str());

        module->par("sourceAddress") = pkt->getDestinationAddress();
        module->par("destAddress") = pkt->getSourceAddress();
        module->par("interfaceTableModule") = interfaceTableModule;

        module->finalizeParameters();

        EV << "VirtualisationManager::instantiateMEApp - UEAppSimbolicAddress: " << pkt->getSourceAddress()<< endl;

        //connecting VirtualisationInfrastructure gates to the MEClusterizeApp gates
        virtualisationInfr->gate("meAppOut", index)->connectTo(module->gate("virtualisationInfrastructureIn"));
        module->gate("virtualisationInfrastructureOut")->connectTo(virtualisationInfr->gate("meAppIn", index));

        // if there is a service required: link the MEApp to MEPLATFORM to MESERVICE
        if(serviceIndex != NO_SERVICE){

            EV << "VirtualisationManager::instantiateMEApp - Connecting to the: " << pkt->getRequiredService()<< endl;
            //connecting MEPlatform gates to the MEClusterizeApp gates
            mePlatform->gate("meAppOut", index)->connectTo(module->gate("mePlatformIn"));
            module->gate("mePlatformOut")->connectTo(mePlatform->gate("meAppIn", index));

            //connecting internal MEPlatform gates to the MEClusterizeService gates
            (meServices.at(serviceIndex))->gate("meAppOut", index)->connectTo(mePlatform->gate("meAppOut", index));
            mePlatform->gate("meAppIn", index)->connectTo((meServices.at(serviceIndex))->gate("meAppIn", index));
        }
        else
            EV << "VirtualisationManager::instantiateMEApp - NO ME Service required!"<< endl;

        module->buildInside();
        module->scheduleStart(simTime());
        module->callInitialize();

        //creating the map entry
        meAppMapTable[key.str()].meAppGateIndex = index;
        meAppMapTable[key.str()].meAppModule = module;
        meAppMapTable[key.str()].ueAddress = ueAppAddress;

        currentMEApps++;

        EV << "VirtualisationManager::instantiateMEApp - "<< pkt->getMEModuleName() << "[" << key.str() <<"] instanced!" << endl;
        EV << "VirtualisationManager::instantiateMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;

        //Sending ACK to the UEClusterizeApp
        EV << "VirtualisationManager::instantiateMEApp - calling ackMEAppPacket with  "<< ACK_START_MEAPP << endl;
        ackMEAppPacket(pkt, ACK_START_MEAPP);
    }
}

void VirtualisationManager::terminateMEApp(MEAppPacket* pkt){

    int serviceIndex = findService(pkt->getRequiredService());

    std::stringstream moduleName;
    moduleName << pkt->getMEModuleName();

    //creating the map key
    std::stringstream key;
    key << pkt->getSourceAddress();

    if(!meAppMapTable.empty() && meAppMapTable.find(key.str()) != meAppMapTable.end()){

        //Sending ACK to the UEClusterizeApp
        EV << "VirtualisationManager::terminateMEApp - calling ackMEAppPacket with  "<< ACK_STOP_MEAPP << endl;
        //before to remove the map entry!
        ackMEAppPacket(pkt, ACK_STOP_MEAPP);

        EV << "VirtualisationManager::terminateMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;

        int index = meAppMapTable[key.str()].meAppGateIndex;

        meAppMapTable[key.str()].meAppModule->callFinish();
        meAppMapTable[key.str()].meAppModule->deleteModule();

        //disconnecting internal MEPlatform gates to the MEClusterizeService gates
        (meServices.at(serviceIndex))->gate("meAppOut", index)->disconnect();
        (meServices.at(serviceIndex))->gate("meAppIn", index)->disconnect();

        //disconnecting MEPlatform gates to the MEClusterizeApp gates
        mePlatform->gate("meAppOut", index)->disconnect();
        mePlatform->gate("meAppIn", index)->disconnect();

        //update map
        std::map<std::string, meAppMapEntry>::iterator it1;
        it1 = meAppMapTable.find(key.str());
        if (it1 != meAppMapTable.end())
            meAppMapTable.erase (it1);

        freeGates.push_back(index);

        currentMEApps--;

        EV << "VirtualisationManager::terminateMEApp - " << moduleName.str() << "[" << key.str() << "] terminated!" << endl;
        EV << "VirtualisationManager::terminateMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;
    }
    else{
        //Sending ACK to the UEClusterizeApp
        EV << "VirtualisationManager::terminateMEClusterizeApp - calling ackClusterize with  "<< ACK_STOP_CLUSTERIZE << endl;
        ackMEAppPacket(pkt, ACK_STOP_MEAPP);

        EV << "VirtualisationManager::terminateMEClusterizeApp - \tWARNING: NO MEClusterizeApp INSTANCE FOUND!" << endl;
    }
}

void VirtualisationManager::ackMEAppPacket(MEAppPacket* pkt, const char* type){

    //creating the map key
    std::stringstream key;
    key << pkt->getSourceAddress();

    const char* destSimbolicAddr = pkt->getSourceAddress();

    if(!meAppMapTable.empty() && meAppMapTable.find(key.str()) != meAppMapTable.end()){

        destAddress_ = meAppMapTable[key.str()].ueAddress;

        //checking if the Car is in the network & sending by socket
        MacNodeId destId = binder_->getMacNodeId(destAddress_.toIPv4());
        if(destId == 0){
            EV << "VirtualisationManager::ackMEAppPacket - \tWARNING " << destSimbolicAddr << "has left the network!" << endl;
            //throw cRuntimeError("VirtualisationManager::ackClusterize - \tFATAL! Error destination has left the network!");
        }
        else{
            EV << "VirtualisationManager::ackMEAppPacket - sending ack " << type <<" to "<< destSimbolicAddr << ": [" << destAddress_.str() <<"]" << endl;

            MEAppPacket* ack = new MEAppPacket();
            ack->setType(type);
            ack->setSno(pkt->getSno());
            ack->setTimestamp(simTime());
            ack->setByteLength(pkt->getByteLength());
            ack->setSourceAddress(pkt->getDestinationAddress());
            ack->setDestinationAddress(destSimbolicAddr);

            socket.sendTo(ack, destAddress_, destPort_);
        }
    }
    else
        EV << "VirtualisationManager::ackMEAppPacket - \tWARNING sending ack " << type <<" to "<< destSimbolicAddr << ": map entry "<< key.str() <<" not found!" << endl;

    delete(pkt);
}
/*
 * ######################################################################################################
 */

int VirtualisationManager::findService(const char* serviceName){

    if(!strcmp(serviceName, "NULL"))
        return NO_SERVICE;

    std::vector<cModule*>::iterator it = meServices.begin();
    for(it ; it != meServices.end(); ++it){
       if(!strcmp((*it)->getClassName(), serviceName))
           break;
    }
    if(it == meServices.end())
        return SERVICE_NOT_AVAILABLE;

    return it - meServices.begin();
}

