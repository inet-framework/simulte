//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "corenetwork/binder/LteBinder.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include <cctype>

#include "../lteCellInfo/LteCellInfo.h"
#include "corenetwork/nodes/InternetMux.h"

using namespace std;

Define_Module(LteBinder);

void LteBinder::unregisterNode(MacNodeId id)
{
    EV << NOW << " LteBinder::unregisterNode - unregistering node " << id << endl;

    if(nodeIds_.erase(id) != 1){
        EV_ERROR << "Cannot unregister node - node id \"" << id << "\" - not found";
    }
    std::map<IPv4Address, MacNodeId>::iterator it;
    for(it = macNodeIdToIPAddress_.begin(); it != macNodeIdToIPAddress_.end(); )
    {
        if(it->second == id)
        {
            macNodeIdToIPAddress_.erase(it++);
        }
        else
        {
            it++;
        }
    }
}

MacNodeId LteBinder::registerNode(cModule *module, LteNodeType type,
    MacNodeId masterId)
{
    Enter_Method_Silent("registerNode");

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
    Enter_Method_Silent("registerNextHop");
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
    if (stage == inet::INITSTAGE_LOCAL)
    {
        numBands_ = par("numBands");
    }
}

void LteBinder::unregisterNextHop(MacNodeId masterId, MacNodeId slaveId)
{
    Enter_Method_Silent("unregisterNextHop");
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
    if(it != nodeIds_.end())
        return it->second;
    return 0;
}

std::map<int, OmnetId>::const_iterator LteBinder::getNodeIdListBegin()
{
    return nodeIds_.begin();
}

std::map<int, OmnetId>::const_iterator LteBinder::getNodeIdListEnd()
{
    return nodeIds_.end();
}

MacNodeId LteBinder::getMacNodeIdFromOmnetId(OmnetId id){
	std::map<int, OmnetId>::iterator it;
	for (it = nodeIds_.begin(); it != nodeIds_.end(); ++it )
	    if (it->second == id)
	        return it->first;
	return 0;
}

LteMacBase* LteBinder::getMacFromMacNodeId(MacNodeId id)
{
    if (id == 0)
        return NULL;

    LteMacBase* mac;
    if (macNodeIdToModule_.find(id) == macNodeIdToModule_.end())
    {
        mac = check_and_cast<LteMacBase*>(getMacByMacNodeId(id));
        macNodeIdToModule_[id] = mac;
    }
    else
    {
        mac = macNodeIdToModule_[id];
    }
    return mac;
}

MacNodeId LteBinder::getNextHop(MacNodeId slaveId)
{
    Enter_Method_Silent("getNextHop");
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
    Enter_Method_Silent("getDeployedUes");
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

    // enable DM only if the two endpoints are served by the same cell
    if (nextHop_[src] == nextHop_[dst])
        d2dPeeringMode_[src][dst] = DM;
    else
        d2dPeeringMode_[src][dst] = IM;

    EV << "LteBinder::addD2DCapability - UE " << src << " may transmit to UE " << dst << " using D2D (current mode " << ((d2dPeeringMode_[src][dst] == DM) ? "DM)" : "IM)") << endl;
}

bool LteBinder::checkD2DCapability(MacNodeId src, MacNodeId dst)
{
    if (src < UE_MIN_ID || src >= macNodeIdCounter_[2] || dst < UE_MIN_ID || dst >= macNodeIdCounter_[2])
        return false;

    if (d2dPeeringCapability_.find(src) != d2dPeeringCapability_.end() && d2dPeeringCapability_[src].find(dst) != d2dPeeringCapability_[src].end())
        return d2dPeeringCapability_[src][dst];
    return false;
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

bool LteBinder::isFrequencyReuseEnabled(MacNodeId nodeId)
{
    // a d2d-enabled UE can use frequency reuse if it can communicate using DM with all its peers
    // in fact, the scheduler does not know to which UE it will communicate when it grants some RBs
    if (d2dPeeringMode_.find(nodeId) == d2dPeeringMode_.end())
        return false;

    std::map<MacNodeId, LteD2DMode>::iterator it = d2dPeeringMode_[nodeId].begin();
    if (it == d2dPeeringMode_[nodeId].end())
        return false;

    for (; it != d2dPeeringMode_[nodeId].end(); ++it)
    {
        if (it->second == IM)
            return false;
    }
    return true;
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
