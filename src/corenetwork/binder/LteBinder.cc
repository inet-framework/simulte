//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteBinder.h"
#include "LteDeployer.h"
#include "L3AddressResolver.h"
#include <cctype>
#include "InternetMux.h"

using namespace std;

Define_Module(LteBinder);

void LteBinder::registerDeployer(LteDeployer* pDeployer, MacCellId macCellId)
{
    deployersMap_[macCellId] = pDeployer;
}

//void LteBinder::nodesConfiguration() {
//
//    if (nodesConfigured_ == true)
//        return;
//
//    cModule* LteInternet = getSimulation()->getModuleByPath("lteInternet");
//
//    cModule** vUes = NULL;
//    //cModule** rUes = NULL;
//    cXMLElement* network = par("traffic").xmlValue();
//
//    if (network == NULL) {
//        error(
//                "FATAL! LteBinder::UesCreator No network  configuration file specified. Aborting");
//    }
//    cXMLElementList cellList = network->getChildren();
//
//    // in cell list we have cells with both uplink and downlink sections,related to eNodeBs and Ues.
//
//    unsigned int deployedCells = 0;
//
//    // reference to macro node
//    cModule * refEnodeB = NULL;
//
//    // micro/macro management
//    const char * microValue;
//    bool micro = false;
//
//    LteDeployer * macroDeployer,
//                * microDeployer;
//
//    for (cXMLElementList::const_iterator cit = cellList.begin();
//            cit != cellList.end(); ++cit) { // for each cell
//        // Items defined for this direction are also considered for node deploying
//        Direction masterDir;
//        bool masterSet = false;
//
//        cXMLElement* cell = *cit;
//        cXMLElementList trafficList = cell->getChildren();
//
//        unsigned int cellId = 0;
//        LteDeployer* deployer = NULL;
//        unsigned int currentNumberOfUes = 0;
//        cModule * eNodeB = NULL;
//
//
//        for (cXMLElementList::const_iterator it = trafficList.begin();
//                it != trafficList.end(); ++it) { // for each traffic direction
//
//            // Downlink and Uplink
//            cXMLElement* trafficDirection = *it;
//            const char * dir = trafficDirection->getTagName();
//            const char * value = trafficDirection->getAttribute("value");
//
//            Direction currentDir;
//
//            // link direction is not connected
//            if (strcasecmp(value, "disabled") == 0)
//                continue;
//            else {
//                if (strcasecmp(dir, "Downlink") == 0) {
//                    currentDir = DL;
//                } else {
//                    currentDir = UL;
//                }
//                if (!masterSet) {
//                    masterDir = currentDir;
//                    masterSet = true;
//                }
//            }
//            // requesting all children for enabled direction
//            cXMLElementList nodeList = trafficDirection->getChildren();
//            int nDl=0;
//            for (cXMLElementList::const_iterator jt = nodeList.begin();
//                    jt != nodeList.end(); ++jt)
//            {
//                cXMLElement* xmlElement = *jt;
//                const char * nodeName = xmlElement->getTagName();
//
//                //--------------- eNb Creation -------------------
//                if (strcasecmp(nodeName, "eNodeB") == 0)
//                {
//                    //TODO aggiustare l'uso del parametro micro
//                    //TODO togliere i commenti in italiano
//
//                    // reset micro indicator
//                    micro = false;
//                    EnbType enbType;
//
//                    // check whether the eNb is configured as micro or not (this may be true, false or undefined)
//                    microValue = xmlElement->getAttribute("micro");
//                    if(microValue!=NULL)
//                        micro = (strcmp(microValue,"true") == 0) ? true : false;
//                    if(micro)
//                        enbType = MICRO_ENB;
//                    else
//                        enbType = MACRO_ENB;
//
//                    if (masterDir == currentDir)
//                    {
//                        //create a new cell. The type of eNb must be specified
//                        eNodeB = createNodeB(enbType);
//                        cellId = eNodeB->par("macCellId");
//                        ++deployedCells;
//                        EV << " LteBinder::nodesConfiguration direction "
//                                << dirToA(currentDir) << " deployed cell "
//                                << deployedCells
//                                << " - is micro?" << ((micro==true)?"yes":"no") << endl;
//
//                        // this is the macro node for the cell
//                        if(micro == false)
//                        {
//                            EV << "LteBinder::nodesConfiguration - Macro Node Added" << endl;
//                            refEnodeB = eNodeB;    // maintain Macro Node reference
//                        }
//                        else // this is a micro node
//                        {
//                            // update micro list in the macro node
//                            macroDeployer = check_and_cast<LteDeployer*>(refEnodeB->getSubmodule("deployer"));
//                            macroDeployer->addMicroNode(cellId,eNodeB);
//
//                            // set the macro node for this micro node
//                            microDeployer = check_and_cast<LteDeployer*>(eNodeB->getSubmodule("deployer"));
//                            microDeployer->setMacroNode(macroDeployer->getCellId(),refEnodeB);
//
//                            EV << "LteBinder::nodesConfiguration - Micro Node Added" << endl;
//                        }
//                    }
//                }
//                //------------------------------------------------
//
//                //            else if (strcasecmp(nodeName, "Relay") == 0) {
//                //                int nRelay = atoi(xmlElement->getAttribute("num"));
//                //                if (nRelay == 0)
//                //                    continue;
//                //                cellId = atoi(xmlElement->getAttribute("cellId"));
//                //                deployer = deployersMap_[cellId];
//                //                double startAngle = atof(
//                //                        xmlElement->getAttribute("startAngle"));
//                //                unsigned int idRelay;
//                //                double x;
//                //                double y;
//                //
//                //                for (int i = 0; i < nRelay; i++) {
//                //                    idRelay = deployer->deployRelays(startAngle, i, nRelay,
//                //                            &x, &y);
//                //                    cXMLElementList applList = xmlElement->getChildren();
//                //                    for (cXMLElementList::const_iterator yt =
//                //                            applList.begin(); yt != applList.end(); ++yt) {
//                //                        cXMLElement* application = *yt;
//                //                        const char * nodeName = application->getTagName();
//                //                        if (strcasecmp(nodeName, "Ue") == 0) {
//                //                            if (rUes != NULL)
//                //                                delete[] rUes;
//                //                            currentNumberOfUes = atoi(
//                //                                    application->getAttribute("num"));
//                //                            rUes = new cModule*[currentNumberOfUes];
//                //                            double speed = atof(
//                //                                    xmlElement->getAttribute("speed"))
//                //                                    / 3.6;
//                //                            int distance = atoi(
//                //                                    xmlElement->getAttribute("distance"));
//                //                            std::string mobility =
//                //                                    xmlElement->getAttribute(
//                //                                            "mobility_model");
//                //                            for (unsigned int i = 0; i < currentNumberOfUes; ++i) {
//                //                                rUes[i] = deployer->deployUes(x, y, i,
//                //                                        currentNumberOfUes, mobility,
//                //                                        distance, idRelay, speed);
//                //
//                //                            }
//                //                        }
//                //
//                //                        cXMLElementList applList =
//                //                                application->getChildren();
//                //                        for (cXMLElementList::const_iterator xt =
//                //                                applList.begin(); xt != applList.end(); ++xt) {
//                //                            cXMLElement* application = *xt;
//                //                            const char * nodeName =
//                //                                    application->getTagName();
//                //                            if (strcasecmp(nodeName, "App") == 0) {
//                //                                for (unsigned int i = 0; i
//                //                                        < currentNumberOfUes; ++i) {
//                //                                    cXMLAttributeMap attr =
//                //                                            application->getAttributes();
//                //                                    if (strcasecmp(
//                //                                            trafficDirection->getTagName(),
//                //                                            "Downlink") == 0) {
//                //                                        const char* addr =
//                //                                                application->getAttribute(
//                //                                                        "addr");
//                //                                        attachAppModule(rUes[i], addr, attr);
//                //                                        addr = increment_address(addr);
//                //                                    } else
//                //                                        attachAppModule(vUes[i], "", attr);
//                //                                }
//                //                            }
//                //                        }
//                //
//                //                    }
//                //                }
//                //            }
//
//                else if (strcasecmp(nodeName, "Ue") == 0) {
//                    currentNumberOfUes = atoi(xmlElement->getAttribute("num"));
//
//                    if (masterDir == currentDir) {
//                        double speed = atof(xmlElement->getAttribute("speed"))
//                                * (1.0 / 3.6);
//                        bool ueUniformDrop = false;
//                        int maxDistance = 0;
//                        int minDistance = 20;
//                        //if ueUniformDrop==true users will be dropped uniformly in the cell
//                        // and distance parameter will be the maximum distance
//                        int distance = atoi(
//                                xmlElement->getAttribute("distance"));
//
//                        if (strcmp(xmlElement->getAttribute("position"), "drop")
//                                == 0) {
//                            ueUniformDrop = true;
//                            maxDistance = atoi(
//                                    xmlElement->getAttribute("distance"));
//                        }
//
//                        std::string mobility = xmlElement->getAttribute(
//                                "mobility_model");
//
//                        EV
//                                << "LteBinder::nodesConfiguration master direction is "
//                                << dirToA(masterDir) << " cellid " << cellId
//                                << ", handling " << currentNumberOfUes
//                                << " setting up speed at " << speed
//                                << "  mps, distance at " << distance
//                                << " meters " << endl;
//                        deployer = deployersMap_[cellId];
//                        vUes = new cModule*[currentNumberOfUes];
//                        for (unsigned int i = 0; i < currentNumberOfUes; ++i) {
//                            EV
//                                    << " LteBinder::nodesConfiguration deploying ues of cell  "
//                                    << cellId << endl;
//                            if (ueUniformDrop)
//                                distance = uniform(minDistance, maxDistance);
//
//                            vUes[i] = deployer->deployUes(
//                                    deployer->getNodeX(),
//                                    deployer->getNodeY(),
//                                    i,
//                                    currentNumberOfUes,
//                                    mobility,
//                                    distance,
//                                    (uint16_t)(
//                                            deployer->getAncestorPar(
//                                                    "macNodeId")), speed);
//                        }
//                    }
//                }
//
//                //--------- external cell creation --------------
//                else if( strcasecmp(nodeName, "extCell") == 0 )
//                {
//                    EV << "LteBinder::nodesConfiguration - External Cells Creation" << endl;
//                    int num = atoi(xmlElement->getAttribute("num"));
//
//                    double pwr = atof(xmlElement->getAttribute("txPwr"));
//
//                    for( int i= 0 ; i<num ; i++ )
//                        extCellList_.push_back(new ExtCell( pwr, i , num , 1000));
//                }
//                //-----------------------------------------------
//
//                // loading eNodeB IP Address
//                // FIXME eNodeB can be NULL !!!
//                const char* enodeBIp = eNodeB->par("ipAddress").stringValue();
//
//                cXMLElementList applList = xmlElement->getChildren();
//                for (cXMLElementList::const_iterator xt = applList.begin();
//                        xt != applList.end(); ++xt) {
//                    cXMLElement* application = *xt;
//                    const char * num = NULL;
//                    int nAppsSameType = 0;
//                    if (strcasecmp(nodeName, "eNodeB") == 0)
//                    {
//                        num = application->getAttribute("num");
//                        if(num==NULL)
//                            throw cRuntimeError("Unspecified number of application for the eNodeB. (add the num field in xml)");
//                        nAppsSameType = atoi(num);
//                    }
//                    else if (strcasecmp(nodeName, "Ue") == 0)
//                    {
//                        nAppsSameType = currentNumberOfUes;
//                    }
//                    else
//                    {
//                        throw cRuntimeError("unsupported node found in LteBinder::nodesConfiguration");
//                    }
//                    cXMLAttributeMap attr = application->getAttributes();
//
//                    if (currentDir == DL) {
//
//                        for (int i = 0; i < nAppsSameType; i++) {
//                            uint32_t ueIp = 0; // NOTE: removed ipCounter_[2] + nDl;
//                            nDl++;
//                            std::string test=IPv4Address(ueIp).str();
//                            attachAppModule(
//                                    ((strcasecmp(nodeName, "Ue") == 0) ?
//                                            vUes[i] : LteInternet),
//                                    ((strcasecmp(nodeName, "eNodeB") == 0) ?
//                                            IPv4Address(ueIp).str() : enodeBIp),
//                                    attr, -1);
//                        }
//                    } else {
//                        for (int i = 0; i < nAppsSameType; i++) {
//                            attachAppModule(
//                                    ((strcasecmp(nodeName, "Ue") == 0) ?
//                                            vUes[i] : LteInternet),
//                                    IPv4Address(enodeBIp).str(), attr , i);
//                        }
//                    }
//                }
//            }
//
//        }
//        nodesConfigured_ = true;
//    }
//}

void LteBinder::setTransportAppPort(cModule* module, unsigned int counter, cXMLAttributeMap attr)
{
    // obtain app type
    cXMLAttributeMap::iterator jt;
    jt = attr.find("type");
    std::string appType = jt->second;

    std::string basePort;
    int port;

    // the sender is located inside the UE
    if (appType == "VoIPSender")
    {
        // read the destination base port from xml
        basePort = attr.find("baseDestPort")->second;
        // Add an offset equal to the number of app of the same type that have been already configured
        port = atoi(basePort.c_str()) + counter;
        EV << "LteBinder::setTransportAppPort - setting dest port to " << port << endl;

        module->par("destPort") = port;
    }
    // the receiver is located inside the eNb
    else if (appType == "VoIPReceiver")
    {
        // read the local base port from xml.
        basePort = attr.find("baseLocalPort")->second;
        // Add an offset equal to the number of app of the same type that have been already configured
        port = atoi(basePort.c_str()) + counter;
        EV << "LteBinder::setTransportAppPort - setting Local port to " << port << endl;

        module->par("localPort") = port;
    }
}

void LteBinder::parseParam(cModule* module, cXMLAttributeMap attr)
{
    cXMLAttributeMap::iterator it;
    for (it = attr.begin(); it != attr.end(); ++it)
    {
        if (it->first == "num" || it->first == "type" || it->first == "baseLocalPort" || it->first == "baseDestPort")
            continue;
        // FIXME numbers could start with negative sign - or even space !!!
        if (isdigit((it->second)[0]))
        {
            if (it->second.find(".") != string::npos)
                module->par((it->first).c_str()) = atof((it->second).c_str());
            else
                module->par((it->first).c_str()) = atoi((it->second).c_str());
        }
        else
            module->par((it->first).c_str()) = it->second;
    }
}

cModule* LteBinder::createNodeB(EnbType type)
{
    cModule* lteInternet = getSimulation()->getModuleByPath("lteInternet");

    cModuleType *mt = cModuleType::get("lte.corenetwork.nodes.eNodeB");

    cDatarateChannel * iChOut = cDatarateChannel::create("internetChannelOut");
    cDatarateChannel * iChIn = cDatarateChannel::create("internetChannelIn");

    // setting infinite datarate for internet Channel
        iChOut->setDatarate(0.0);
        iChIn->setDatarate(0.0);

        cModule *enodeb = mt->create("enodeb", getSimulation()->getSystemModule());
        MacNodeId cellId = registerNode(enodeb, ENODEB);

        lteInternet->setGateSize("peerLteIp",
            (lteInternet->gateSize("peerLteIp") + 1));

        // connecting to lteInternet : creating multiple gates for multiple nodeBs

        enodeb->gate("peerLteIp$o")->connectTo(
            lteInternet->gate("peerLteIp$i",
                lteInternet->gateSize("peerLteIp$i") - 1), iChOut);
        lteInternet->gate("peerLteIp$o", lteInternet->gateSize("peerLteIp$o") - 1)->connectTo(
            enodeb->gate("peerLteIp$i"), iChIn);

        // access the internet multiplexer

        cModule *mux = lteInternet->getSubmodule("mux");

        // prepare new mux gates
        mux->setGateSize("inExt", (mux->gateSize("inExt") + 1));
        mux->setGateSize("outExt", (mux->gateSize("outExt") + 1));

        // create inner lteInternet queue and connect it to internal gates, then initialize the mux.

        cModuleType *it = cModuleType::get("lte.corenetwork.lteip.InternetQueue");
        cModule *iQueue = it->create("internetqueue", lteInternet);

        // fix all connections

        // LTE Internet ingress goes to MUX ingress.
        lteInternet->gate("peerLteIp$i", lteInternet->gateSize("peerLteIp$i") - 1)->connectTo(
            mux->gate("inExt", mux->gateSize("inExt") - 1));
        // MUX Output goes to queue input
        mux->gate("outExt", mux->gateSize("outExt") - 1)->connectTo(
            iQueue->gate("lteIpIn"));
        // queue output goes to lteInternet output
        iQueue->gate("internetChannelOut")->connectTo(
            lteInternet->gate("peerLteIp$o",
                lteInternet->gateSize("peerLteIp$o") - 1));

        // register mux routing entry
        (dynamic_cast<InternetMux*>(mux))->setRoutingEntry(cellId,
            mux->gate("outExt", mux->gateSize("outExt") - 1));

        // finalize building of new modules

        enodeb->finalizeParameters();
        enodeb->buildInside();
        enodeb->scheduleStart(simTime());

        iQueue->finalizeParameters();
        iQueue->buildInside();
        iQueue->scheduleStart(simTime());

        // initialize newly added internet queue
        iQueue->callInitialize();

        //initializeAllChannels(enodeb);

        iChOut->callInitialize();
        iChIn->callInitialize();

        LteDeployer * deployer = check_and_cast<LteDeployer*>(
            enodeb->getSubmodule("deployer"));
        registerDeployer(deployer, cellId);

        EV << "LteBinder::createNodeB - enbType set to " << type << endl;
        deployer->setEnbType(type);

        deployer->preInitialize();

        return enodeb;
    }

void LteBinder::transportAppAttach(cModule* parentModule, cModule* appModule,
    std::string transport)
{
    // prepare transport gates strings
    string appTgateOut, appTgateIn;

    if (transport == "udp" || transport == "tcp")
    {
        appTgateOut = transport + "Out";
        appTgateIn = transport + "In";
    }
    else
        throw cRuntimeError("LteBinder::transportAppAttach(): unrecognized transport layer %s",
            transport.c_str());

    cModule* tLayer = parentModule->getSubmodule(transport.c_str());

    tLayer->setGateSize("appIn", (tLayer->gateSize("appIn") + 1));
    tLayer->setGateSize("appOut", (tLayer->gateSize("appOut") + 1));

    appModule->gate(appTgateOut.c_str())->connectTo(
        tLayer->gate("appIn", (tLayer->gateSize("appIn") - 1)));

    tLayer->gate("appOut", (tLayer->gateSize("appOut") - 1))->connectTo(
        appModule->gate(appTgateIn.c_str()));
}

void LteBinder::attachAppModule(cModule *parentModule, std::string IPAddr,
    cXMLAttributeMap attr, int counter)
{
    cXMLAttributeMap::iterator jt;
    jt = attr.find("type");
    std::string appType = jt->second;

    EV << NOW << " LteBinder::attachAppModule " << appType << " from "
       << parentModule->getName() << " to IP " << IPAddr.c_str() << endl;

    cModuleType *mt = NULL;
    cModule *module = NULL;

    if (appType == "UDPSink")
    {
        mt = cModuleType::get("inet.applications.udpapp.UDPSink");
        module = mt->create("udpApp", parentModule);
        transportAppAttach(parentModule, module, string("udp"));
    }
    else if (appType == "UDPBasicApp")
    {
        mt = cModuleType::get("inet.applications.udpapp.UDPBasicApp");
        module = mt->create("udpApp", parentModule);
        module->par("destAddresses") = IPAddr;
        transportAppAttach(parentModule, module, string("udp"));
    }
    else if (appType == "VoIPSender")
    {
        mt = cModuleType::get("voip.VoIPSender");
        module = mt->create("udpApp", parentModule);
        module->par("destAddress") = IPAddr;
        transportAppAttach(parentModule, module, string("udp"));
        if (counter != -1) // set the appropriate port for UL direction
            setTransportAppPort(module, counter, attr);
    }
    else if (appType == "VoIPReceiver")
    {
        mt = cModuleType::get("voip.VoIPReceiver");
        // use a different name foreach UL voip receiver (eg: udpApp1)
        std::stringstream str;
        str << "udpApp";
        if (counter != -1)    // only in UL
            str << counter;
        module = mt->create(str.str().c_str(), parentModule);
        transportAppAttach(parentModule, module, string("udp"));
        if (counter != -1) // set the appropriate port for UL direction
            setTransportAppPort(module, counter, attr);
    }
    else if (appType == "TCPBasicClientApp")
    {
        mt = cModuleType::get("inet.applications.tcpapp.TCPBasicClientApp");
        module = mt->create("tcpApp", parentModule);
        module->par("connectAddress") = IPAddr;
        transportAppAttach(parentModule, module, string("tcp"));
    }
    else if (appType == "TCPSinkApp")
    {
        mt = cModuleType::get("inet.applications.tcpapp.TCPSinkApp");
        module = mt->create("tcpApp", parentModule);
        transportAppAttach(parentModule, module, string("tcp"));
    }
    else if (appType == "VoDUDPServer")
    {
        mt = cModuleType::get("vod.VoDUDPServer");
        module = mt->create("vodServer", parentModule);
        module->par("destAddresses") = IPAddr;
        transportAppAttach(parentModule, module, string("udp"));
    }
    else if (appType == "VoDUDPClient")
    {
        mt = cModuleType::get("vod.VoDUDPClient");
        module = mt->create("vodClient", parentModule);
        transportAppAttach(parentModule, module, string("udp"));
    }
    else if (appType == "gaming")
    {
        mt = cModuleType::get("gaming.gaming");
        module = mt->create("gaming", parentModule);
        module->par("destAddresses") = IPAddr;
        transportAppAttach(parentModule, module, string("udp"));
    }
    else if (appType == "ftp_3gpp_sender")
    {
        mt = cModuleType::get("ftp_3gpp.ftp_3gpp_sender");
        module = mt->create("ftp_3gpp_sender", parentModule);
        module->par("destAddresses") = IPAddr;
        transportAppAttach(parentModule, module, string("udp"));
    }
    else if (appType == "ftp_3gpp_sink")
    {
        mt = cModuleType::get("ftp_3gpp.ftp_3gpp_sink");
        module = mt->create("udpApp", parentModule);
        transportAppAttach(parentModule, module, string("udp"));
    }
    else
    {
        throw cRuntimeError("LteBinder::attachAppModule(): unrecognized application type %s", appType.c_str());
    }

    parseParam(module, attr);
    module->finalizeParameters();
    module->buildInside();
    module->scheduleStart(simTime());

    // if we are attaching app to the Internet Module , we have to initialize the app , too
//    if (strcmp(parentModule->getName() ,"lteInternet")==0)
//    {
//        module->callInitialize();
//    }
}

void LteBinder::unregisterNode(MacNodeId id){
    if(nodeIds_.erase(id) != 1){
        EV_ERROR << "Cannot unregister node - node id \"" << id << "\" - not found";
    }
    std::map<IPv4Address, MacNodeId>::iterator it;
    for(it = macNodeIdToIPAddress_.begin(); it != macNodeIdToIPAddress_.end(); ){
        if(it->second == id){
            macNodeIdToIPAddress_.erase(it++);
        } else {
                it++;
        }
    }
}

MacNodeId LteBinder::registerNode(cModule *module, LteNodeType type,
    MacNodeId masterId)
{
    Enter_Method("registerNode");

    MacNodeId macNodeId = -1;

    if (type == UE)
    {
        macNodeId = macNodeIdCounter_[2]++;
    }
    else if (type == RELAY)
    {
        macNodeId = macNodeIdCounter_[1]++;
    }
    else if (type == ENODEB)
    {
        macNodeId = macNodeIdCounter_[0]++;
    }

    EV << "LteBinder : Assigning to module " << module->getName()
       << " with OmnetId: " << module->getId() << " and MacNodeId " << macNodeId
       << "\n";

    // registering new node to LteBinder

    nodeIds_[macNodeId] = module->getId();

    module->par("macNodeId") = macNodeId;

    if (type == RELAY || type == UE)
    {
        registerNextHop(masterId, macNodeId);
    }
    else if (type == ENODEB)
    {
        module->par("macCellId") = macNodeId;
        registerNextHop(macNodeId, macNodeId);
    }
    return macNodeId;
}

void LteBinder::registerNextHop(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method("registerNextHop");
    EV << "LteBinder : Registering slave " << slaveId << " to master "
       << masterId << "\n";

    if (masterId != slaveId)
    {
        dMap_[masterId][slaveId] = true;
    }

    if (nextHop_.size() <= slaveId)
        nextHop_.resize(slaveId + 1);
    nextHop_[slaveId] = masterId;
}

void LteBinder::initialize(int stage)
{
    if (stage == 0)
    {
        const char * stringa;

        std::vector<int> apppriority;
        std::vector<double> appdelay;
        std::vector<double> applossrate;

        stringa = par("priority");
        apppriority = cStringTokenizer(stringa).asIntVector();
        stringa = par("packetDelayBudget");
        appdelay = cStringTokenizer(stringa).asDoubleVector();
        stringa = par("packetErrorLossRate");
        applossrate = cStringTokenizer(stringa).asDoubleVector();

        for (int i = 0; i < LTE_QCI_CLASSES; i++)
        {
            QCIParam_[i].priority = apppriority[i];
            QCIParam_[i].packetDelayBudget = appdelay[i];
            QCIParam_[i].packetErrorLossRate = applossrate[i];
        }
        nodesConfigured_ = false;

        // execute node creation and setup.
        // nodesConfiguration();
    }
    if (stage == 1)
    {
        // all UEs completed registration, so build the table of D2D capabilities

        // build and initialize the matrix of D2D peering capabilities
        MacNodeId maxUe = macNodeIdCounter_[2];
        d2dPeeringCapability_ = new bool*[maxUe];
        for (int i=0; i<maxUe; i++)
        {
            d2dPeeringCapability_[i] = new bool[maxUe];
            for (int j=0; j<maxUe; j++)
            {
                d2dPeeringCapability_[i][j] = false;
            }
        }
    }
}

std::string LteBinder::increment_address(const char* address_string)  //TODO unused function
{
    IPv4Address addr(address_string);
    return IPv4Address(addr.getInt() + 1).str();
}

int LteBinder::getQCIPriority(int QCI)
{
    return QCIParam_[QCI - 1].priority;
}

double LteBinder::getPacketDelayBudget(int QCI)
{
    return QCIParam_[QCI - 1].packetDelayBudget;
}

double LteBinder::getPacketErrorLossRate(int QCI)
{
    return QCIParam_[QCI - 1].packetErrorLossRate;
}

void LteBinder::unregisterNextHop(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method("unregisterNextHop");
    EV << "LteBinder : Unregistering slave " << slaveId << " from master "
       << masterId << "\n";
    dMap_[masterId][slaveId] = false;

    if (nextHop_.size() <= slaveId)
        return;
    nextHop_[slaveId] = 0;
}

OmnetId LteBinder::getOmnetId(MacNodeId nodeId)
{
    std::map<int, OmnetId>::iterator it = nodeIds_.find(nodeId);
    if(it != nodeIds_.end()){
        return it->second;
    } else {
        return 0;
    }
}

MacNodeId LteBinder::getMacNodeIdFromOmnetId(OmnetId id){
	std::map<int, OmnetId>::iterator it;
	for (it = nodeIds_.begin(); it != nodeIds_.end(); ++it ){
	    if (it->second == id){
	        return it->first;
	    }
	}
	return 0;
}

MacNodeId LteBinder::getNextHop(MacNodeId slaveId)
{
    Enter_Method("getNextHop");
    if (slaveId >= nextHop_.size())
        throw cRuntimeError("LteBinder::getNextHop(): bad slave id %d", slaveId);
    return nextHop_[slaveId];
}

void LteBinder::registerName(MacNodeId nodeId, const char* moduleName)
{
    int len = strlen(moduleName);
    macNodeIdToModuleName_[nodeId] = new char[len+1];
    strcpy(macNodeIdToModuleName_[nodeId], moduleName);
}

const char* LteBinder::getModuleNameByMacNodeId(MacNodeId nodeId)
{
    if (macNodeIdToModuleName_.find(nodeId) == macNodeIdToModuleName_.end())
        throw cRuntimeError("LteBinder::getModuleNameByMacNodeId - node ID not found");
    return macNodeIdToModuleName_[nodeId];
}

ConnectedUesMap LteBinder::getDeployedUes(MacNodeId localId, Direction dir)
{
    Enter_Method("getDeployedUes");
    return dMap_[localId];
}

void LteBinder::registerX2Port(X2NodeId nodeId, int port)
{
    if (x2ListeningPorts_.find(nodeId) == x2ListeningPorts_.end() )
    {
        // no port has yet been registered
        std::list<int> ports;
        ports.push_back(port);
        x2ListeningPorts_[nodeId] = ports;
    }
    else
    {
        x2ListeningPorts_[nodeId].push_back(port);
    }
}

int LteBinder::getX2Port(X2NodeId nodeId)
{
    if (x2ListeningPorts_.find(nodeId) == x2ListeningPorts_.end() )
        throw cRuntimeError("LteBinder::getX2Port - No ports available on node %d", nodeId);

    int port = x2ListeningPorts_[nodeId].front();
    x2ListeningPorts_[nodeId].pop_front();
    return port;
}

Cqi LteBinder::meanCqi(std::vector<Cqi> bandCqi,MacNodeId id,Direction dir)
{
    std::vector<Cqi>::iterator it;
    Cqi mean=0;
    for (it=bandCqi.begin();it!=bandCqi.end();++it)
    {
        mean+=*it;
    }
    mean/=bandCqi.size();

    if(mean==0)
        mean = 1;

    return mean;
}

void LteBinder::addD2DCapability(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || src >= macNodeIdCounter_[2] || dst < UE_MIN_ID || dst >= macNodeIdCounter_[2])
        throw cRuntimeError("LteBinder::addD2DCapability - Node Id not valid. Src %d Dst %d", src, dst);

    d2dPeeringCapability_[src][dst] = true;

    // insert initial communication mode
    // TODO make it configurable from NED
    d2dPeeringMode_[src][dst] = DM;

    EV << "LteBinder::addD2DCapability - UE " << src << " may transmit to UE " << dst << " using D2D" << endl;
}

bool LteBinder::checkD2DCapability(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || src >= macNodeIdCounter_[2] || dst < UE_MIN_ID || dst >= macNodeIdCounter_[2])
        throw cRuntimeError("LteBinder::checkD2DCapability - Node Id not valid. Src %d Dst %d", src, dst);

    return d2dPeeringCapability_[src][dst];
}

std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> >* LteBinder::getD2DPeeringModeMap()
{
    return &d2dPeeringMode_;
}

LteD2DMode LteBinder::getD2DMode(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || src >= macNodeIdCounter_[2] || dst < UE_MIN_ID || dst >= macNodeIdCounter_[2])
        throw cRuntimeError("LteBinder::getD2DMode - Node Id not valid. Src %d Dst %d", src, dst);

    return d2dPeeringMode_[src][dst];
}

void LteBinder::registerMulticastGroup(MacNodeId nodeId, int32 groupId)
{
    if (multicastGroupMap_.find(nodeId) == multicastGroupMap_.end())
    {
        MulticastGroupIdSet newSet;
        newSet.insert(groupId);
        multicastGroupMap_[nodeId] = newSet;
    }
    else
    {
        multicastGroupMap_[nodeId].insert(groupId);
    }
}

bool LteBinder::isInMulticastGroup(MacNodeId nodeId, int32 groupId)
{
    if (multicastGroupMap_.find(nodeId) == multicastGroupMap_.end())
        return false;   // the node is not enrolled in any group
    if (multicastGroupMap_[nodeId].find(groupId) == multicastGroupMap_[nodeId].end())
        return false;   // the node is not enrolled in the given group

    return true;
}

void LteBinder::updateUeInfoCellId(MacNodeId id, MacCellId newCellId)
{
    std::vector<UeInfo*>::iterator it = ueList_.begin();
    for (; it != ueList_.end(); ++it)
    {
        if ((*it)->id == id)
        {
            (*it)->cellId = newCellId;
            return;
        }
    }
}

void LteBinder::addUeHandoverTriggered(MacNodeId nodeId)
{
    ueHandoverTriggered_.insert(nodeId);
}

bool LteBinder::hasUeHandoverTriggered(MacNodeId nodeId)
{
    if (ueHandoverTriggered_.find(nodeId) == ueHandoverTriggered_.end())
        return false;
    return true;
}

void LteBinder::removeUeHandoverTriggered(MacNodeId nodeId)
{
    ueHandoverTriggered_.erase(nodeId);
}
