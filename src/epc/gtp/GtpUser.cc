//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "epc/gtp/GtpUser.h"
#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include <iostream>

Define_Module(GtpUser);

void GtpUser::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = par("localPort");

    socket_.setOutputGate(gate("udpOut"));
    socket_.bind(localPort_);

    tunnelPeerPort_ = par("tunnelPeerPort");

    //============= Reading XML files =============
    const char *filename = par("teidFileName");
    if (filename == NULL || (!strcmp(filename, "")))
        error("GtpUser::initialize - Error reading configuration from file %s", filename);
    if (!loadTeidTable(filename))
        error("GtpUser::initialize - Wrong xml file format");

    if (par("filter"))
    {
        filename = par("tftFileName");
        if (filename == NULL || (!strcmp(filename, "")))
            error("GtpUser::initialize - Error reading configuration from file %s", filename);
        if (!loadTftTable(filename))
            error("GtpUser::initialize - Wrong xml file format");
    }
    //=============================================
}

void GtpUser::handleMessage(cMessage *msg)
{
    if (strcmp(msg->getArrivalGate()->getFullName(), "trafficFlowFilterGate") == 0)
    {
        EV << "GtpUser::handleMessage - message from trafficFlowFilter" << endl;
        // obtain the encapsulated IPv4 datagram
        IPv4Datagram * datagram = check_and_cast<IPv4Datagram*>(msg);
        handleFromTrafficFlowFilter(datagram);
    }
    else if(strcmp(msg->getArrivalGate()->getFullName(),"udpIn")==0)
    {
        EV << "GtpUser::handleMessage - message from udp layer" << endl;

        GtpUserMsg * gtpMsg = check_and_cast<GtpUserMsg *>(msg);
        handleFromUdp(gtpMsg);
    }
}

void GtpUser::handleFromTrafficFlowFilter(IPv4Datagram * datagram)
{
    // extract control info from the datagram
    TftControlInfo * tftInfo = check_and_cast<TftControlInfo *>(datagram->removeControlInfo());
    TrafficFlowTemplateId flowId = tftInfo->getTft();
    delete (tftInfo);

    EV << "GtpUser::handleFromTrafficFlowFilter - Received a tftMessage with flowId[" << flowId << "]" << endl;

    TunnelEndpointIdentifier nextTeid;
    L3Address tunnelPeerAddress;

    // search a correspondence between the flow id and the pair <teid,nextHop>
    LabelTable::iterator tftIt;
    tftIt = tftTable_.find(flowId);
    if (tftIt == tftTable_.end())
    {
        EV << "GtpUser::handleFromTrafficFlowFilter - Cannot find entry for TFT " << flowId << ". Discarding packet;" << endl;
        return;
    }
    tunnelPeerAddress = tftIt->second.nextHop;
    nextTeid = tftIt->second.teid;

    // create a new gtpUserMessage
    GtpUserMsg * gtpMsg = new GtpUserMsg();
    gtpMsg->setName("gtpUserMessage");

    // assign the nextTeid
    gtpMsg->setTeid(nextTeid);

    // encapsulate the datagram within the gtpUserMessage
    gtpMsg->encapsulate(datagram);

    socket_.sendTo(gtpMsg, tunnelPeerAddress, tunnelPeerPort_);
}

void GtpUser::handleFromUdp(GtpUserMsg * gtpMsg)
{
    TunnelEndpointIdentifier oldTeid, nextTeid;
    L3Address nextHopAddr;

    // obtain the incoming TEID from message
    oldTeid = gtpMsg->getTeid();

    // obtain "ConnectionInfo" from the teidTable
    LabelTable::iterator teidIt = teidTable_.find(oldTeid);
    if (teidIt == teidTable_.end())
    {
        EV << "GtpUser::handleFromUdp - Cannot find entry for TEID " << oldTeid << ". Discarding packet;" << endl;
        return;
    }
    ConnectionInfo teidInfo = teidIt->second;

    // decide here whether performing a label switching or a label removal
    if (teidInfo.teid == LOCAL_ADDRESS_TEID) // tunneling ended.
    {
        EV << "GtpUser::handleFromUdp - IP packet pointing to this network. Decapsulating and sending to local connection." << endl;

        // obtain the original IP datagram and send it to the local network
        IPv4Datagram * datagram = check_and_cast<IPv4Datagram*>(gtpMsg->decapsulate());
        delete(gtpMsg);
        send(datagram,"pppGate");
    }
    else // label switching
    {
        EV << "GtpUser::handleFromUdp - performing label switching: [" << oldTeid << "]->[" << teidInfo.teid
           << "] - nextHop[" << teidInfo.nextHop << "]." << endl;
        // in case of label switching, send the packet to the next tunnel
        gtpMsg->setTeid(teidInfo.teid);
        gtpMsg->removeControlInfo();
        socket_.sendTo(gtpMsg,teidInfo.nextHop,tunnelPeerPort_);
    }
}

//==========================================================================
//============================== XML MANAGEMENT ============================
//==========================================================================
bool GtpUser::loadTeidTable(const char * teidTableFile)
{
    // open and check xml file
    EV << "GtpUser::loadTeidTable - reading file " << teidTableFile << endl;
    cXMLElement* config = getEnvir()->getXMLDocument(teidTableFile);
    if (config == NULL)
    error("GtpUser::loadTeidTable: Cannot read configuration from file: %s", teidTableFile);

    // obtain reference to teidTable
    cXMLElement* teidNode = config->getElementByPath("teidTable");
    if (teidNode == NULL)
    error("GtpUser::loadTeidTable: No configuration for teidTable");

    cXMLElementList teidList = teidNode->getChildren();

    TunnelEndpointIdentifier teidIn,teidOut;
    L3Address nextHop;

    // teid attributes management
    const unsigned int numAttributes = 3;
    char const * attributes[numAttributes] =
    {   "teidIn" , "teidOut" , "nextHop"};
    unsigned int attrId = 0;

    char const * temp[numAttributes];

    // foreach teid element in the list, read the parameters and fill the teid table
    for (cXMLElementList::iterator teidsIt = teidList.begin(); teidsIt != teidList.end(); teidsIt++)
    {
        std::string elementName = (*teidsIt)->getTagName();

        if ((elementName == "teid"))
        {
            // clean attributes
            for( attrId = 0; attrId<numAttributes; ++attrId)
            temp[attrId] = NULL;

            for( attrId = 0; attrId<numAttributes; ++attrId)
            {
                temp[attrId] = (*teidsIt)->getAttribute(attributes[attrId]);
                if(temp[attrId]==NULL)
                {
                    EV << "GtpUser::loadTeidTable - unable to find attribute " << attributes[attrId] << endl;
                    return false;
                }
            }

            teidIn = atoi(temp[0]);
            teidOut = atoi(temp[1]);
            nextHop.set(IPv4Address(temp[2]));

            std::pair<LabelTable::iterator,bool> ret;
            ret = teidTable_.insert(std::pair<TunnelEndpointIdentifier,ConnectionInfo>(teidIn,ConnectionInfo(teidOut,nextHop)));;
            if (ret.second==false)
            EV << "GtpUser::loadTeidTable - skipping duplicate entry  with TEID " << ret.first->first << '\n';
            else
            EV << "GtpUser::loadTeidTable - inserted entry: TEIDin[" << teidIn << "] - TEIDout[" << teidOut << "] - NextHop[" << nextHop << "]" << endl;
        }
    }
    return true;
}

// TODO avoid replicating the xmlLoad code. Use an array of attributes as input and a array of strings as return
bool GtpUser::loadTftTable(const char * tftTableFile)
{
    // open and check xml file
    EV << "GtpUser::loadTftTable - reading file " << tftTableFile << endl;
    cXMLElement* config = getEnvir()->getXMLDocument(tftTableFile);
    if (config == NULL)
    error("GtpUser::loadTftTable: Cannot read configuration from file: %s", tftTableFile);

    // obtain reference to teidTable
    cXMLElement* tftNode = config->getElementByPath("tftTable");
    if (tftNode == NULL)
    error("GtpUser::loadTftTable: No configuration for tftTable");

    // obtain list of TFTs
    cXMLElementList tftList = tftNode->getChildren();

    TrafficFlowTemplateId tft;
    TunnelEndpointIdentifier teidOut;
    L3Address nextHop;

    // TFT attributes management
    const unsigned int numAttributes = 3;
    char const * attributes[numAttributes] =
    {   "tftId" , "teidOut" , "nextHop"};
    unsigned int attrId = 0;

    char const * temp[numAttributes];

    // foreach TFT element in the list, read the parameters and fill the teid table
    for (cXMLElementList::iterator tftIt = tftList.begin(); tftIt != tftList.end(); tftIt++)
    {
        std::string elementName = (*tftIt)->getTagName();

        if ((elementName == "tft"))
        {
            // clean attributes
            for( attrId = 0; attrId<numAttributes; ++attrId)
            temp[attrId] = NULL;

            // read the values of each attributes of the table
            for( attrId = 0; attrId<numAttributes; ++attrId)
            {
                temp[attrId] = (*tftIt)->getAttribute(attributes[attrId]);
                if(temp[attrId]==NULL)
                {
                    EV << "GtpUser::loadTftTable - unable to find attribute " << attributes[attrId] << endl;
                    return false;
                }
            }

            // convert attributes
            tft= atoi(temp[0]);
            teidOut = atoi(temp[1]);
            nextHop.set(IPv4Address(temp[2]));

            // create a new entry in the TEID table,
            std::pair<LabelTable::iterator,bool> ret;
            ret = tftTable_.insert(std::pair<TrafficFlowTemplateId,ConnectionInfo>(tft,ConnectionInfo(teidOut,nextHop)));;
            if (ret.second==false)
            EV << "GtpUser::loadTftTable - skipping duplicate entry  with TFT " << ret.first->first << '\n';
            else
            EV << "GtpUser::loadTtftTable - inserted entry: TFT[" << tft << "] - TEIDout[" << teidOut << "] - NextHop[" << nextHop << "]" << endl;
        }
    }
    return true;
}
// ==========================================================================

