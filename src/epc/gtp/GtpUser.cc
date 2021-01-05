//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "epc/gtp/GtpUser.h"
#include <inet/common/ModuleAccess.h>
#include "inet/applications/common/SocketTag_m.h"
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <iostream>

Define_Module(GtpUser);
using namespace inet;

void GtpUser::initialize(int stage) {
    cSimpleModule::initialize(stage);

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    socket_.setOutputGate(gate("socketOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");

    ownerType_ = selectOwnerType(getAncestorPar("nodeType"));

    ie_ = detectInterface();

    //============= Reading XML files =============
    const char *filename = par("teidFileName");
    if (filename == nullptr || (!strcmp(filename, "")))
        error("GtpUser::initialize - Error reading configuration from file %s",
                filename);
    if (!loadTeidTable(filename))
        error("GtpUser::initialize - Wrong xml file format");

    if (par("filter")) {
        filename = par("tftFileName");
        if (filename == nullptr || (!strcmp(filename, "")))
            error(
                    "GtpUser::initialize - Error reading configuration from file %s",
                    filename);
        if (!loadTftTable(filename))
            error("GtpUser::initialize - Wrong xml file format");
    }
    //=============================================
}

InterfaceEntry *GtpUser::detectInterface()
{
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    const char *interfaceName = par("ipOutInterface");
    InterfaceEntry *ie = nullptr;

    if (strlen(interfaceName) > 0) {
        ie = ift->findInterfaceByName(interfaceName);
        if (ie == nullptr)
            throw cRuntimeError("Interface \"%s\" does not exist", interfaceName);
    }

    return ie;
}


EpcNodeType GtpUser::selectOwnerType(const char * type)
{
    EV << "GtpUser::selectOwnerType - setting owner type to " << type << endl;
    if(strcmp(type,"ENODEB") == 0)
        return ENB;
    else if(strcmp(type,"PGW") == 0)
        return PGW;
    else if(strcmp(type,"SGW") == 0)
        return SGW;
    else
        error("GtpUser::selectOwnerType - unknown owner type [%s]. Aborting...",type);

    // should never be reached - return default value in order to avoid compile warnings
    return ENB;
}

void GtpUser::handleMessage(cMessage *msg) {
    if (strcmp(msg->getArrivalGate()->getFullName(), "trafficFlowFilterGate")
            == 0) {
        EV << "GtpUser::handleMessage - message from trafficFlowFilter" << endl;
        // obtain the encapsulated IPv4 datagram
        auto pkt = check_and_cast<Packet*>(msg);
        pkt->trim();
        auto ipDatagram = pkt->peekAtFront<Ipv4Header>();
        pkt->addTagIfAbsent<NetworkProtocolInd>()->setProtocol(&Protocol::ipv4);
        pkt->addTagIfAbsent<NetworkProtocolInd>()->setNetworkProtocolHeader(
                ipDatagram);
        handleFromTrafficFlowFilter(pkt);
    } else if (strcmp(msg->getArrivalGate()->getFullName(), "socketIn") == 0) {
        EV << "GtpUser::handleMessage - message from udp layer" << endl;
        auto pkt = check_and_cast<Packet*>(msg);
        pkt->trim();
        auto ipDatagram = pkt->peekAtFront<GtpUserMsg>();
        handleFromUdp(pkt);
    }
}

void GtpUser::handleFromTrafficFlowFilter(Packet *pkt) {
    // extract control info from the datagram
    auto tftInfo = pkt->removeTag<TftControlInfo>();
    TrafficFlowTemplateId flowId = tftInfo->getTft();
    delete tftInfo;
    removeAllSimuLteTags(pkt);

    EV << "GtpUser::handleFromTrafficFlowFilter - Received a tftMessage with flowId["
       << flowId << "]" << endl;

    TunnelEndpointIdentifier nextTeid;
    L3Address tunnelPeerAddress;

    // search a correspondence between the flow id and the pair <teid,nextHop>
    LabelTable::iterator tftIt;
    tftIt = tftTable_.find(flowId);
    if (tftIt == tftTable_.end()) {
        EV
                  << "GtpUser::handleFromTrafficFlowFilter - Cannot find entry for TFT "
                  << flowId << ". Discarding packet;" << endl;
        return;
    }
    tunnelPeerAddress = tftIt->second.nextHop;
    nextTeid = tftIt->second.teid;

    // create a new gtpUserMessage
    auto gtpMsg = makeShared<GtpUserMsg>();

    // assign the nextTeid
    gtpMsg->setTeid(nextTeid);

    // add GTP header
    pkt->insertAtFront(gtpMsg);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&LteProtocol::gtp);


    socket_.sendTo(pkt, tunnelPeerAddress, tunnelPeerPort_);
}

void GtpUser::handleFromUdp(Packet *pkt) {
    TunnelEndpointIdentifier oldTeid;
    L3Address nextHopAddr;

    // remove GTP header
    auto gtpMsg = pkt->removeAtFront<GtpUserMsg>();

    // remove any pending socket indications
    auto sockInd = pkt->removeTagIfPresent<SocketInd>();
    if (sockInd)
        delete sockInd;

    // obtain the incoming TEID from message
    oldTeid = gtpMsg->getTeid();

    // obtain "ConnectionInfo" from the teidTable
    LabelTable::iterator teidIt = teidTable_.find(oldTeid);
    if (teidIt == teidTable_.end()) {
        EV << "GtpUser::handleFromUdp - Cannot find entry for TEID " << oldTeid
                  << ". Discarding packet;" << endl;
        delete pkt;
        return;
    }
    ConnectionInfo teidInfo = teidIt->second;

    // decide here whether performing a label switching or a label removal
    if (teidInfo.teid == LOCAL_ADDRESS_TEID) // tunneling ended.
    {
        EV
                  << "GtpUser::handleFromUdp - IP packet pointing to this network. Decapsulating and sending to local connection."
                  << endl;

        // send original IP datagram to the local network
        pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
        pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);

        if (ownerType_ == ENB && ie_ != nullptr) {
            // on the ENB we need an InterfaceReq to send the packet via WLAN
            pkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie_->getInterfaceId());
        }

        send(pkt, "pppGate");
    } else // label switching
    {
        EV << "GtpUser::handleFromUdp - performing label switching: ["
                  << oldTeid << "]->[" << teidInfo.teid << "] - nextHop["
                  << teidInfo.nextHop << "]." << endl;
        // in case of label switching, send the packet to the next tunnel
        gtpMsg->setTeid(teidInfo.teid);
        pkt->insertAtFront(gtpMsg);
        removeAllSimuLteTags(pkt);
        socket_.sendTo(pkt, teidInfo.nextHop, tunnelPeerPort_);
    }
}

//==========================================================================
//============================== XML MANAGEMENT ============================
//==========================================================================
bool GtpUser::loadTeidTable(const char *teidTableFile) {
    // open and check xml file
    EV << "GtpUser::loadTeidTable - reading file " << teidTableFile << endl;
    cXMLElement *config = getEnvir()->getXMLDocument(teidTableFile);
    if (config == nullptr)
        error("GtpUser::loadTeidTable: Cannot read configuration from file: %s",
                teidTableFile);

    // obtain reference to teidTable
    cXMLElement *teidNode = config->getElementByPath("teidTable");
    if (teidNode == nullptr)
        error("GtpUser::loadTeidTable: No configuration for teidTable");

    cXMLElementList teidList = teidNode->getChildren();

    TunnelEndpointIdentifier teidIn, teidOut;
    L3Address nextHop;

    // teid attributes management
    const unsigned int numAttributes = 3;
    char const *attributes[numAttributes] = { "teidIn", "teidOut", "nextHop" };
    unsigned int attrId = 0;

    char const *temp[numAttributes];

    // foreach teid element in the list, read the parameters and fill the teid table
    for (cXMLElementList::iterator teidsIt = teidList.begin();
            teidsIt != teidList.end(); teidsIt++) {
        std::string elementName = (*teidsIt)->getTagName();

        if ((elementName == "teid")) {
            // clean attributes
            for (attrId = 0; attrId < numAttributes; ++attrId)
                temp[attrId] = nullptr;

            for (attrId = 0; attrId < numAttributes; ++attrId) {
                temp[attrId] = (*teidsIt)->getAttribute(attributes[attrId]);
                if (temp[attrId] == nullptr) {
                    EV << "GtpUser::loadTeidTable - unable to find attribute "
                              << attributes[attrId] << endl;
                    return false;
                }
            }

            teidIn = atoi(temp[0]);
            teidOut = atoi(temp[1]);
            nextHop.set(Ipv4Address(temp[2]));

            std::pair<LabelTable::iterator, bool> ret;
            ret = teidTable_.insert(
                    std::pair<TunnelEndpointIdentifier, ConnectionInfo>(teidIn,
                            ConnectionInfo(teidOut, nextHop)));
            ;
            if (ret.second == false)
                EV
                          << "GtpUser::loadTeidTable - skipping duplicate entry  with TEID "
                          << ret.first->first << '\n';
            else
                EV << "GtpUser::loadTeidTable - inserted entry: TEIDin["
                          << teidIn << "] - TEIDout[" << teidOut
                          << "] - NextHop[" << nextHop << "]" << endl;
        }
    }
    return true;
}

// TODO avoid replicating the xmlLoad code. Use an array of attributes as input and a array of strings as return
bool GtpUser::loadTftTable(const char *tftTableFile) {
    // open and check xml file
    EV << "GtpUser::loadTftTable - reading file " << tftTableFile << endl;
    cXMLElement *config = getEnvir()->getXMLDocument(tftTableFile);
    if (config == nullptr)
        error("GtpUser::loadTftTable: Cannot read configuration from file: %s",
                tftTableFile);

    // obtain reference to teidTable
    cXMLElement *tftNode = config->getElementByPath("tftTable");
    if (tftNode == nullptr)
        error("GtpUser::loadTftTable: No configuration for tftTable");

    // obtain list of TFTs
    cXMLElementList tftList = tftNode->getChildren();

    TrafficFlowTemplateId tft;
    TunnelEndpointIdentifier teidOut;
    L3Address nextHop;

    // TFT attributes management
    const unsigned int numAttributes = 3;
    char const *attributes[numAttributes] = { "tftId", "teidOut", "nextHop" };
    unsigned int attrId = 0;

    char const *temp[numAttributes];

    // foreach TFT element in the list, read the parameters and fill the teid table
    for (cXMLElementList::iterator tftIt = tftList.begin();
            tftIt != tftList.end(); tftIt++) {
        std::string elementName = (*tftIt)->getTagName();

        if ((elementName == "tft")) {
            // clean attributes
            for (attrId = 0; attrId < numAttributes; ++attrId)
                temp[attrId] = nullptr;

            // read the values of each attributes of the table
            for (attrId = 0; attrId < numAttributes; ++attrId) {
                temp[attrId] = (*tftIt)->getAttribute(attributes[attrId]);
                if (temp[attrId] == nullptr) {
                    EV << "GtpUser::loadTftTable - unable to find attribute "
                              << attributes[attrId] << endl;
                    return false;
                }
            }

            // convert attributes
            tft = atoi(temp[0]);
            teidOut = atoi(temp[1]);
            nextHop.set(Ipv4Address(temp[2]));

            // create a new entry in the TEID table,
            std::pair<LabelTable::iterator, bool> ret;
            ret = tftTable_.insert(
                    std::pair<TrafficFlowTemplateId, ConnectionInfo>(tft,
                            ConnectionInfo(teidOut, nextHop)));
            ;
            if (ret.second == false)
                EV
                          << "GtpUser::loadTftTable - skipping duplicate entry  with TFT "
                          << ret.first->first << '\n';
            else
                EV << "GtpUser::loadTtftTable - inserted entry: TFT[" << tft
                          << "] - TEIDout[" << teidOut << "] - NextHop["
                          << nextHop << "]" << endl;
        }
    }
    return true;
}
// ==========================================================================

