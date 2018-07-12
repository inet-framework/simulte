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

VirtualisationManager::VirtualisationManager()
{
    currentMEApps = 0;
}

void VirtualisationManager::initialize(int stage)
{
    EV << "VirtualisationManager::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    //------------------------------------
    //SIMULTE Binder module
    binder_ = getBinder();
    //------------------------------------
    //communication with UE APPs
    destPort_ = par("destPort").longValue();
    localPort_ = par("localPort").longValue();
    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort_);
    //------------------------------------
    //parent modules
    virtualisationInfr = getParentModule();
    if(virtualisationInfr != NULL)
    {
        meHost = virtualisationInfr->getParentModule();
        if(meHost->hasPar("maxMEApps"))
                maxMEApps = meHost->par("maxMEApps").longValue();
        else
            throw cRuntimeError("VirtualisationManager::initialize - \tFATAL! Error when getting meHost.maxMEApps parameter!");
        //setting gate sizes for VirtualisationManager and VirtualisationInfrastructure
        this->setGateSize("meAppOut", maxMEApps);
        this->setGateSize("meAppIn", maxMEApps);
        virtualisationInfr->setGateSize("meAppOut", maxMEApps);
        virtualisationInfr->setGateSize("meAppIn", maxMEApps);
        //VirtualisationInfrastructure internal gate connections with VirtualisationManager
        for(int index = 0; index < maxMEApps; index++)
        {
            this->gate("meAppOut", index)->connectTo(virtualisationInfr->gate("meAppOut", index));
            virtualisationInfr->gate("meAppIn", index)->connectTo(this->gate("meAppIn", index));
        }
        mePlatform = meHost->getSubmodule("mePlatform");
        //setting  gate sizes for MEPlatform
        if(mePlatform->gateSize("meAppOut") == 0 || mePlatform->gateSize("meAppIn") == 0)
        {
            mePlatform->setGateSize("meAppOut", maxMEApps);
            mePlatform->setGateSize("meAppIn", maxMEApps);
        }
        // retrieving all available ME Services loaded
        numServices = mePlatform->par("numServices").longValue();
        for(int i=0; i<numServices; i++)
        {
            meServices.push_back(mePlatform->getSubmodule("udpService", i));
            EV << "VirtualisationManager::initialize - Available meServices["<<i<<"] " << meServices.at(i)->getClassName() << endl;
        }
    }
    else
    {
        EV << "VirtualisationManager::initialize - ERROR getting virtualisationInfrastructure cModule!" << endl;
        throw cRuntimeError("VirtualisationManager::initialize - \tFATAL! Error when getting getParentModule()");
    }
    //------------------------------------
    //retrieve the set of free gates
    for(int i = 0; i < maxMEApps; i++)
        freeGates.push_back(i);
    //------------------------------------
    interfaceTableModule = par("interfaceTableModule").stringValue();
}

void VirtualisationManager::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;

    //handling resource allocation confirmation
    if(msg->arrivedOn("resourceManagerIn"))
    {
        EV << "VirtualisationManager::handleMessage - received message from ResourceManager" << endl;
        handleResource(msg);
    }
    //handling MEAppPacket between UEApp and MEApp
    else
    {
        MEAppPacket* mePkt = check_and_cast<MEAppPacket*>(msg);
        if(mePkt == 0)
        {
            EV << "VirtualisationManager::handleMessage - \tFATAL! Error when casting to MEAppPacket" << endl;
            throw cRuntimeError("VirtualisationManager::handleMessage - \tFATAL! Error when casting to MEAppPacket");
        }
        else{
            EV << "VirtualisationManager::handleMessage - received MEAppPacket from " << mePkt->getSourceAddress() << endl;
            handleMEAppPacket(mePkt);
        }
    }
}

/*
 * ###################################RESOURCE MANAGER decision HANDLER#################################
 */

void VirtualisationManager::handleResource(cMessage* msg){

    MEAppPacket* mePkt = check_and_cast<MEAppPacket*>(msg);
    if(mePkt == 0)
    {
        EV << "VirtualisationManager::handleResource - \tFATAL! Error when casting to MEAppPacket" << endl;
        throw cRuntimeError("VirtualisationManager::handleResource - \tFATAL! Error when casting to MEAppPacket");
    }
    //handling MEAPP instantiation
    else if(!strcmp(mePkt->getType(), START_MEAPP))
    {
        EV << "VirtualisationManager::handleResource - calling instantiateMEApp" << endl;
        instantiateMEApp(mePkt);
    }
    //handling MEAPP termination
    else if(!strcmp(mePkt->getType(), STOP_MEAPP))
    {
        EV << "VirtualisationManager::handleResource - calling terminateMEApp" << endl;
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

    EV << "VirtualisationManager::handleClusterize - received "<< pkt->getType()<<" with delay: " << (simTime()-pkt->getTimestamp()) << endl;

    /* Handling START_MEAPP */
    if(!strcmp(pkt->getType(), START_MEAPP))        startMEApp(pkt);

    /* Handling INFO_UEAPP */
    else if(!strcmp(pkt->getType(), INFO_UEAPP))    upstreamToMEApp(pkt);

    /* Handling INFO_MEAPP */
    else if(!strcmp(pkt->getType(), INFO_MEAPP))    downstreamToUEApp(pkt);

    /* Handling STOP_MEAPP */
    else if(!strcmp(pkt->getType(), STOP_MEAPP))    stopMEApp(pkt);
}

void VirtualisationManager::startMEApp(MEAppPacket* pkt){

    EV << "VirtualisationManager::startMEApp - processing..." << endl;

    //checking if the service required is available
    if(findService(pkt->getRequiredService()) == SERVICE_NOT_AVAILABLE)
    {
        EV << "VirtualisationManager::startMEApp - Service required is not available: " << pkt->getRequiredService() << endl;
        throw cRuntimeError("VirtualisationManager::startMEApp - \tFATAL! Service required is not available!" );
        return;
    }

    //retrieve UE App ID
    int ueAppID = pkt->getUeAppID();

    //checking if there are available slots for MEApp && the required MEApp instance is not already instantiated!
    if(currentMEApps < maxMEApps && ueAppIdToMeAppMapKey.find(ueAppID) == ueAppIdToMeAppMapKey.end())
    {
        //Checking for resource availability to the Resource Manager
        send(pkt, "resourceManagerOut");
        //testing
        EV << "VirtualisationManager::startMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;
        EV << "VirtualisationManager::startMEApp - forwarding " << pkt->getType() << " to Resource Manager!" << endl;
    }
    else
    {
        if(ueAppIdToMeAppMapKey.find(ueAppID) != ueAppIdToMeAppMapKey.end())
        {
            //Sending ACK to the UEApp to confirm the instantiation in case of previous ack lost!
            ackMEAppPacket(pkt, ACK_START_MEAPP);
            //testing
            EV << "VirtualisationManager::startMEApp - \tWARNING: required MEApp instance ALREADY STARTED!" << endl;
            EV << "VirtualisationManager::startClusterize  - calling ackMEAppPacket with  "<< ACK_START_MEAPP << endl;
        }
        else EV << "VirtualisationManager::startMEApp - \tWARNING: maxMEApp LIMIT REACHED!" << endl;
    }
}

void VirtualisationManager::upstreamToMEApp(MEAppPacket* pkt){

    EV << "VirtualisationManager::upstreamToMEApp - processing..."<< endl;

    //retrieve UE App ID
    int ueAppID = pkt->getUeAppID();

    //checking if ueAppIdToMeAppMapKey entry map does exist
    if(ueAppIdToMeAppMapKey.empty() || ueAppIdToMeAppMapKey.find(ueAppID) == ueAppIdToMeAppMapKey.end())
    {
        EV << "VirtualisationeManager::upstreamToMEApp - \tWARNING forwarding to ueAppIdToMeAppMapKey["<< ueAppID <<"] not found!" << endl;
        throw cRuntimeError("VirtualisationeManager::upstreamToMEApp - \tERROR ueAppIdToMeAppMapKey entry not found!");
        return;
    }
    //getting the meAppMap map key from the ueAppIdToMeAppMapKey map
    int key = ueAppIdToMeAppMapKey[ueAppID];

    if(!meAppMap.empty() && meAppMap.find(key) != meAppMap.end())
    {
        EV << "VirtualisationManager::upstreamToMEApp - forwarding " << pkt->getName() << " to meAppOut["<< meAppMap[key].meAppGateIndex << "] gate" << endl;
        send(pkt, "meAppOut", meAppMap[key].meAppGateIndex);
    }
    else EV << "VirtualisationManager::upstreamToMEApp - \tWARNING: NO " << pkt->getMEModuleName() << " INSTANCE FOUND!" << endl;
}

void VirtualisationManager::downstreamToUEApp(MEAppPacket* pkt){

    EV << "VirtualisationManager::downstreamToUEApp - processing..."<< endl;

    //getting the meAppMap map key (meAppIn gate index)
    int key = pkt->getArrivalGate()->getIndex();

    const char* destSimbolicAddr = pkt->getDestinationAddress();

    if(!meAppMap.empty() && meAppMap.find(key) != meAppMap.end()){

        destAddress_ = meAppMap[key].ueAddress;

        //checking if the Car is in the network & sending by socket
        MacNodeId destId = binder_->getMacNodeId(destAddress_.toIPv4());
        if(destId == 0)
        {
            EV << "VirtualisationeManager::downstreamToUEApp - \tWARNING " << destSimbolicAddr << "has left the network!" << endl;
            //throw cRuntimeError("VirtualisationManager::downstreamClusterize - \tFATAL! Error destination has left the network!");

            //starting the MEApp termination procedure
            MEAppPacket* spkt = new MEAppPacket(STOP_MEAPP);
            spkt->setType(STOP_MEAPP);
            spkt->setSno(pkt->getSno());
            spkt->setByteLength(pkt->getByteLength());
            spkt->setSourceAddress(pkt->getSourceAddress());
            spkt->setDestinationAddress(pkt->getDestinationAddress());
            spkt->setMEModuleName(pkt->getMEModuleName());
            spkt->setUeAppID(pkt->getUeAppID());
            EV << "VirtualisationeManager::downstreamToUEApp - calling stopMEApp for " << destSimbolicAddr << endl;
            stopMEApp(spkt);

            delete(pkt);
        }
        else
        {
            EV << "VirtualisationManager::downstreamToUEApp - forwarding " << pkt->getName() << " to "<< destSimbolicAddr << ": [" << destAddress_.str() <<"]" << endl;
            socket.sendTo(pkt, destAddress_, destPort_);
        }
    }
    else EV << "VirtualisationeManager::downstreamToUEApp - \tWARNING forwarding to "<< destSimbolicAddr << ": meAppMap["<< key <<"] not found!" << endl;
}

void VirtualisationManager::stopMEApp(MEAppPacket* pkt){

    EV << "VirtualisationManager::stopMEApp - processing..." << endl;

    //retrieve UE App ID
    int ueAppID = pkt->getUeAppID();

    //checking if ueAppIdToMeAppMapKey entry map does exist
    if(ueAppIdToMeAppMapKey.empty() || ueAppIdToMeAppMapKey.find(ueAppID) == ueAppIdToMeAppMapKey.end())
    {
        EV << "VirtualisationManager::stopMEApp - \tWARNING forwarding to ueAppIdToMeAppMapKey["<< ueAppID <<"] not found!" << endl;
        throw cRuntimeError("VirtualisationManager::stopMEApp - \tERROR ueAppIdToMeAppMapKey entry not found!");
        return;
    }
    //getting the meAppMap map key from the ueAppIdToMeAppMapKey map
    int key = ueAppIdToMeAppMapKey[ueAppID];

    if(!meAppMap.empty() && meAppMap.find(key) != meAppMap.end())
    {
        //Asking to the Resource Manager to free resources
        EV << "VirtualisationManager::stopMEApp - forwarding "<< pkt->getType() << " to Resource Manager!" << endl;
        send(pkt, "resourceManagerOut");
    }
    else EV << "VirtualisationManager::stopMEApp - \tWARNING: " << pkt->getMEModuleName() << " INSTANCE NOT FOUND!" << endl;
}

void VirtualisationManager::instantiateMEApp(MEAppPacket* pkt)
{
    EV << "VirtualisationManager::instantiateMEApp - processing..." << endl;

    int serviceIndex = findService(pkt->getRequiredService());
    char* sourceAddress = (char*)pkt->getSourceAddress();
    char* meModuleName = (char*)pkt->getMEModuleName();

    //retrieve UE App ID
    int ueAppID = pkt->getUeAppID();

    //checking if there are ME App slots available and if ueAppIdToMeAppMapKey map entry does not exist (that means ME App not already instantiated)
    if(currentMEApps < maxMEApps &&  ueAppIdToMeAppMapKey.find(ueAppID) == ueAppIdToMeAppMapKey.end())
    {
        //getting the first available gate
        int index = freeGates.front();
        freeGates.erase(freeGates.begin());
        EV << "VirtualisationManager::instantiateMEApp - freeGate: " << index << endl;

        //creating ueAppIdToMeAppMapKey map entry
        ueAppIdToMeAppMapKey[ueAppID] = index;
        EV << "VirtualisationManager::instantiateMEApp - ueAppIdToMeAppMapKey[" << ueAppID << "] = " << index << endl;
        int key = ueAppIdToMeAppMapKey[ueAppID] ;

        //getting the UEApp L3Address
        inet::L3Address ueAppAddress = inet::L3AddressResolver().resolve(sourceAddress);
        EV << "VirtualisationManager::instantiateMEApp - UEAppL3Address: " << ueAppAddress.str() << endl;

        // creating MEApp module instance
        cModuleType *moduleType = cModuleType::get(pkt->getMEModuleType());         //MEAPP module package (i.e. path!)
        cModule *module = moduleType->create(meModuleName, meHost);       //MEAPP module-name & its Parent Module
        std::stringstream appName;
        appName << meModuleName << "[" <<  sourceAddress << "]";
        module->setName(appName.str().c_str());

        //creating the meAppMap map entry
        meAppMap[key].meAppGateIndex = index;
        meAppMap[key].meAppModule = module;
        meAppMap[key].ueAddress = ueAppAddress;

        //displaying ME App dynamically created (after 70 they will overlap..)
        std::stringstream display;
        display << "p=" << (80 + ((index%5)*200)%1000) << "," << (100 + (50*(index/5)%350)) << ";is=vs";
        module->setDisplayString(display.str().c_str());

        //initialize IMEApp Parameters
        module->par("ueSimbolicAddress") = sourceAddress;
        module->par("meHostSimbolicAddress") = pkt->getDestinationAddress();
        module->par("interfaceTableModule") = interfaceTableModule;

        module->finalizeParameters();

        EV << "VirtualisationManager::instantiateMEApp - UEAppSimbolicAddress: " << sourceAddress << endl;

        //connecting VirtualisationInfrastructure gates to the MEApp gates
        virtualisationInfr->gate("meAppOut", index)->connectTo(module->gate("virtualisationInfrastructureIn"));
        module->gate("virtualisationInfrastructureOut")->connectTo(virtualisationInfr->gate("meAppIn", index));

        // if there is a service required: link the MEApp to MEPLATFORM to MESERVICE
        if(serviceIndex != NO_SERVICE)
        {
            EV << "VirtualisationManager::instantiateMEApp - Connecting to the: " << pkt->getRequiredService()<< endl;
            //connecting MEPlatform gates to the MEApp gates
            mePlatform->gate("meAppOut", index)->connectTo(module->gate("mePlatformIn"));
            module->gate("mePlatformOut")->connectTo(mePlatform->gate("meAppIn", index));

            //connecting internal MEPlatform gates to the required MEService gates
            (meServices.at(serviceIndex))->gate("meAppOut", index)->connectTo(mePlatform->gate("meAppOut", index));
            mePlatform->gate("meAppIn", index)->connectTo((meServices.at(serviceIndex))->gate("meAppIn", index));
        }
        else EV << "VirtualisationManager::instantiateMEApp - NO MEService required!"<< endl;

        module->buildInside();
        module->scheduleStart(simTime());
        module->callInitialize();

        currentMEApps++;

        //Sending ACK to the UEClusterizeApp
        EV << "VirtualisationManager::instantiateMEApp - calling ackMEAppPacket with  "<< ACK_START_MEAPP << endl;
        ackMEAppPacket(pkt, ACK_START_MEAPP);

        //testing
        EV << "VirtualisationManager::instantiateMEApp - "<< module->getName() <<" instanced!" << endl;
        EV << "VirtualisationManager::instantiateMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;
    }
}

void VirtualisationManager::terminateMEApp(MEAppPacket* pkt)
{
    int serviceIndex = findService(pkt->getRequiredService());

    //retrieve UE App ID
    int ueAppID = pkt->getUeAppID();

    if(!ueAppIdToMeAppMapKey.empty() && ueAppIdToMeAppMapKey.find(ueAppID) != ueAppIdToMeAppMapKey.end())
    {
        //retrieve meAppMap map key
        int key = ueAppIdToMeAppMapKey[ueAppID];
        //check if meAppMap map entry does exist
        if(meAppMap.empty() || meAppMap.find(key) == meAppMap.end())
        {
            EV << "VirtualisationManager::terminateMEApp - \ERROR meAppMap["<< key << "] does not exist!" << endl;
            throw cRuntimeError("VirtualisationManager::terminateMEApp - \ERROR meAppMap entry does not exist!");
            return;
        }

        //terminating the ME App instance
        meAppMap[key].meAppModule->callFinish();
        meAppMap[key].meAppModule->deleteModule();
        currentMEApps--;
        EV << "VirtualisationManager::terminateMEApp - currentMEApps: " << currentMEApps << " / " << maxMEApps << endl;
        EV << "VirtualisationManager::terminateMEApp - " << meAppMap[key].meAppModule->getName() << " terminated!" << endl;

        //Sending ACK_STOP_MEAPP to the UEApp
        EV << "VirtualisationManager::terminateMEApp - calling ackMEAppPacket with  "<< ACK_STOP_MEAPP << endl;
        //before to remove the map entry!
        ackMEAppPacket(pkt, ACK_STOP_MEAPP);

        int index = meAppMap[key].meAppGateIndex;
        //disconnecting internal MEPlatform gates to the MEService gates
        (meServices.at(serviceIndex))->gate("meAppOut", index)->disconnect();
        (meServices.at(serviceIndex))->gate("meAppIn", index)->disconnect();
        //disconnecting MEPlatform gates to the MEApp gates
        mePlatform->gate("meAppOut", index)->disconnect();
        mePlatform->gate("meAppIn", index)->disconnect();

        //update maps
        ueAppIdToMeAppMapKey.erase(ueAppIdToMeAppMapKey.find(ueAppID));
        std::map<int, meAppMapEntry>::iterator it1;
        it1 = meAppMap.find(key);
        if (it1 != meAppMap.end())
            meAppMap.erase (it1);

        freeGates.push_back(index);
    }
    else
    {
        EV << "VirtualisationManager::terminateMEClusterizeApp - \tWARNING: NO INSTANCE FOUND!" << endl;
        //Sending ACK_STOP_MEAPP to the UEApp
        EV << "VirtualisationManager::terminateMEClusterizeApp - calling ackMEAppPacket with  "<< ACK_STOP_MEAPP << endl;
        ackMEAppPacket(pkt, ACK_STOP_MEAPP);
    }
}

void VirtualisationManager::ackMEAppPacket(MEAppPacket* pkt, const char* type)
{
    //retrieve UE App ID
    int ueAppID = pkt->getUeAppID();

    const char* destSymbolicAddr = pkt->getSourceAddress();

    if(!ueAppIdToMeAppMapKey.empty() && ueAppIdToMeAppMapKey.find(ueAppID) != ueAppIdToMeAppMapKey.end())
    {
        //retrieve meAppMap map key
        int key = ueAppIdToMeAppMapKey[ueAppID];
        //check if meAppMap map entry does exist
        if(meAppMap.empty() || meAppMap.find(key) == meAppMap.end())
        {
            EV << "VirtualisationManager::ackMEAppPacket - \ERROR meAppMap["<< key << "] does not exist!" << endl;
            throw cRuntimeError("VirtualisationManager::ackMEAppPacket - \ERROR meAppMap entry does not exist!");
            return;
        }

        destAddress_ = meAppMap[key].ueAddress;

        //checking if the Car is in the network & sending by socket
        MacNodeId destId = binder_->getMacNodeId(destAddress_.toIPv4());
        if(destId == 0)
        {
            EV << "VirtualisationManager::ackMEAppPacket - \tWARNING " << destSymbolicAddr << "has left the network!" << endl;
            //throw cRuntimeError("VirtualisationManager::ackClusterize - \tFATAL! Error destination has left the network!");
        }
        else
        {
            EV << "VirtualisationManager::ackMEAppPacket - sending ack " << type <<" to "<< destSymbolicAddr << ": [" << destAddress_.str() <<"]" << endl;

            MEAppPacket* ack = new MEAppPacket();
            ack->setType(type);
            ack->setSno(pkt->getSno());
            ack->setTimestamp(simTime());
            ack->setByteLength(pkt->getByteLength());
            ack->setSourceAddress(pkt->getDestinationAddress());
            ack->setDestinationAddress(destSymbolicAddr);
            socket.sendTo(ack, destAddress_, destPort_);
        }
    }
    else EV << "VirtualisationManager::ackMEAppPacket - \tWARNING in sending ack " << type <<" to "<< destSymbolicAddr << ": ueAppIdToMeAppMapKey["<< ueAppID <<"] not found!" << endl;

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

