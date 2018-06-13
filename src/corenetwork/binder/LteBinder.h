//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEBINDER_H_
#define _LTE_LTEBINDER_H_

#include <omnetpp.h>
#include <string>

#include "common/LteCommon.h"
#include "inet/networklayer/contract/ipv4/IPv4Address.h"
#include "inet/networklayer/common/L3Address.h"
#include "corenetwork/binder/PhyPisaData.h"
#include "corenetwork/nodes/ExtCell.h"
#include "stack/mac/layer/LteMacBase.h"

using namespace inet;

/**
 * The LTE Binder module has one instance in the whole network.
 * It stores global mapping tables with OMNeT++ module IDs,
 * IP addresses, etc.
 *
 * After this it fills the two tables:
 * - nextHop, binding each master node id with its slave
 * - nodeId, binding each node id with the module id used by Omnet.
 * - dMap_, binding each master with all its slaves (used by amc)
 *
 * The binder is accessed to gather:
 * - the nextHop table (by the eNodeB)
 * - the Omnet module id (by any module)
 * - the map of deployed UEs per master (by amc)
 *
 */

class LteBinder : public cSimpleModule
{
  private:
    typedef std::map<MacNodeId, std::map<MacNodeId, bool> > DeployedUesMap;

    unsigned int numBands_;  // number of logical bands
    std::map<IPv4Address, MacNodeId> macNodeIdToIPAddress_;
    std::map<MacNodeId, char*> macNodeIdToModuleName_;
    std::map<MacNodeId, LteMacBase*> macNodeIdToModule_;
    std::vector<MacNodeId> nextHop_; // MacNodeIdMaster --> MacNodeIdSlave
    std::map<int, OmnetId> nodeIds_;

    // list of static external cells. Used for intercell interference evaluation
    ExtCellList extCellList_;

    // list of all eNBs. Used for inter-cell interference evaluation
    std::vector<EnbInfo*> enbList_;

    // list of all UEs. Used for inter-cell interference evaluation
    std::vector<UeInfo*> ueList_;

    MacNodeId macNodeIdCounter_[3]; // MacNodeId Counter
    DeployedUesMap dMap_; // DeployedUes --> Master Mapping

    /*
     * Uplink interference support
     */
    typedef std::vector< std::vector< std::vector<UeAllocationInfo> > > UplinkTransmissionMap;
    // for both previous and current TTIs, for each RB, stores the UE (nodeId and ref to the PHY module) that transmitted/are transmitting within that RB
    UplinkTransmissionMap ulTransmissionMap_;
    // TTI of the last update of the UL band status
    simtime_t lastUpdateUplinkTransmissionInfo_;

    /*
     * X2 Support
     */
    typedef std::map<X2NodeId, std::list<int> > X2ListeningPortMap;
    X2ListeningPortMap x2ListeningPorts_;

    std::map<MacNodeId, std::map<MacNodeId, L3Address> > x2PeerAddress_;

    /*
     * D2D Support
     */
    // determines if two D2D-capable UEs are communicating in D2D mode or Infrastructure Mode
    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> > d2dPeeringMap_;

    /*
     * Multicast support
     */
    // register here the IDs of the multicast group where UEs participate
    typedef std::set<uint32> MulticastGroupIdSet;
    std::map<MacNodeId, MulticastGroupIdSet> multicastGroupMap_;
    std::set<MacNodeId> multicastTransmitterSet_;

    /*
     * Handover support
     */
    // store the id of the UEs that are performing handover
    std::set<MacNodeId> ueHandoverTriggered_;
  protected:
    virtual void initialize(int stages);

    virtual int numInitStages() const { return INITSTAGE_LAST; }

    virtual void handleMessage(cMessage *msg)
    {
    }

  public:
    LteBinder()
    {
        macNodeIdCounter_[0] = ENB_MIN_ID;
        macNodeIdCounter_[1] = RELAY_MIN_ID;
        macNodeIdCounter_[2] = UE_MIN_ID;

        ulTransmissionMap_.resize(2); // store transmission map of previous and current TTI
    }

    unsigned int getNumBands()
    {
        return numBands_;
    }

    virtual ~LteBinder()
    {
        while(enbList_.size() > 0){
            delete enbList_.back();
            enbList_.pop_back();
        }
    }

    /**
     * Registers a node to the global LteBinder module.
     *
     * The binder assigns an IP address to the node, from which it is derived
     * an unique macNodeId.
     * The node registers its moduleId (omnet ID), and if it's a relay or an UE,
     * it registers also the association with its master node.
     *
     * @param module pointer to the module to be registered
     * @param type type of this node (ENODEB, RELAY, UE)
     * @param masterId id of the master of this node, 0 if none (node is an eNB)
     * @return macNodeId assigned to the module
     */
    MacNodeId registerNode(cModule *module, LteNodeType type, MacNodeId masterId = 0);

    /**
     * Un-registers a node from the global LteBinder module.
     */
    void unregisterNode(MacNodeId id);

    /**
     * registerNextHop() is called by the IP2LTE module at network startup
     * to bind each slave (UE or Relay) with its masters. It is also
     * called on handovers to synchronize the nextHop table:
     *
     * It registers a slave to its current master
     *
     * @param masterId MacNodeId of the Master
     * @param slaveId MacNodeId of the Slave
     */
    void registerNextHop(MacNodeId masterId, MacNodeId slaveId);

    /**
     * registerNextHop() is called on handovers to sychronize
     * the nextHop table:
     *
     * It unregisters the slave from its old master
     *
     * @param masterId MacNodeId of the Master
     * @param slaveId MacNodeId of the Slave
     */
    void unregisterNextHop(MacNodeId masterId, MacNodeId slaveId);

    /**
     * getOmnetId() returns the Omnet Id of the module
     * given its MacNodeId
     *
     * @param nodeId MacNodeId of the module
     * @return OmnetId of the module
     */
    OmnetId getOmnetId(MacNodeId nodeId);

    /*
     * get iterators for the list of nodes
     */
    std::map<int, OmnetId>::const_iterator getNodeIdListBegin();
    std::map<int, OmnetId>::const_iterator getNodeIdListEnd();

    /**
     * getMacNodeIdFromOmnetId() returns the MacNodeId of the module
     * given its OmnetId
     *
     * @param id OmnetId of the module
     * @return MacNodeId of the module
     */
    MacNodeId getMacNodeIdFromOmnetId(OmnetId id);

    /*
     * getMacFromMacNodeId() returns the reference to the LteMacBase module
     * given the MacNodeId of a node
     *
     * @param id MacNodeId of the module
     * @return LteMacBase* of the module
     */
    LteMacBase* getMacFromMacNodeId(MacNodeId id);

    /**
     * getNextHop() returns the master of
     * a given slave
     *
     * @param slaveId MacNodeId of the Slave
     * @return MacNodeId of the master
     */
    MacNodeId getNextHop(MacNodeId slaveId);

    /**
     * Returns the MacNodeId for the given IP address
     *
     * @param address IP address
     * @return MacNodeId corresponding to the IP addres
     */
    MacNodeId getMacNodeId(IPv4Address address)
    {
        if (macNodeIdToIPAddress_.find(address) == macNodeIdToIPAddress_.end())
            return 0;
        return macNodeIdToIPAddress_[address];
    }

    /**
     * Returns the X2NodeId for the given IP address
     *
     * @param address IP address
     * @return X2NodeId corresponding to the IP address
     */
    X2NodeId getX2NodeId(IPv4Address address)
    {
        return getMacNodeId(address);
    }

    /**
     * Associates the given IP address with the given MacNodeId.
     *
     * @param address IP address
     */
    void setMacNodeId(IPv4Address address, MacNodeId nodeId)
    {
        macNodeIdToIPAddress_[address] = nodeId;
    }
    /**
     * Associates the given IP address with the given X2NodeId.
     *
     * @param address IP address
     */
    void setX2NodeId(IPv4Address address, X2NodeId nodeId)
    {
        setMacNodeId(address, nodeId);
    }
    L3Address getX2PeerAddress(X2NodeId srcId, X2NodeId destId)
    {
        return x2PeerAddress_[srcId][destId];
    }
    void setX2PeerAddress(X2NodeId srcId, X2NodeId destId, L3Address interfAddr)
    {
        std::pair<X2NodeId, L3Address> p(destId, interfAddr);
        x2PeerAddress_[srcId].insert(p);
    }
    /**
     * Associates the given MAC node ID to the name of the module
     */
    void registerName(MacNodeId nodeId, const char* moduleName);
    /**
     * Returns the module name for the given MAC node ID
     */
    const char* getModuleNameByMacNodeId(MacNodeId nodeId);

    /*
     * getDeployedUes() returns the affiliates
     * of a given eNodeB
     */
    ConnectedUesMap getDeployedUes(MacNodeId localId, Direction dir);
    PhyPisaData phyPisaData;

    int getNodeCount(){
        return nodeIds_.size();
    }

    int addExtCell(ExtCell* extCell)
    {
        extCellList_.push_back(extCell);
        return extCellList_.size() - 1;
    }

    ExtCellList getExtCellList()
    {
        return extCellList_;
    }

    void addEnbInfo(EnbInfo* info)
    {
        enbList_.push_back(info);
    }

    std::vector<EnbInfo*> * getEnbList()
    {
        return &enbList_;
    }

    void addUeInfo(UeInfo* info)
    {
        ueList_.push_back(info);
    }

    std::vector<UeInfo*> * getUeList()
    {
        return &ueList_;
    }

    Cqi meanCqi(std::vector<Cqi> bandCqi,MacNodeId id,Direction dir);

    /*
     * Uplink interference support
     */
    simtime_t getLastUpdateUlTransmissionInfo();
    void initAndResetUlTransmissionInfo();
    void storeUlTransmissionMap(Remote antenna, RbMap& rbMap, MacNodeId nodeId, MacCellId cellId, LtePhyBase* phy, Direction dir);
    const std::vector<UeAllocationInfo>* getUlTransmissionMap(UlTransmissionMapTTI t, Band b);
    /*
     * X2 Support
     */
    void registerX2Port(X2NodeId nodeId, int port);
    int getX2Port(X2NodeId nodeId);

    /*
     * D2D Support
     */
    bool checkD2DCapability(MacNodeId src, MacNodeId dst);
    bool getD2DCapability(MacNodeId src, MacNodeId dst);

    std::map<MacNodeId, std::map<MacNodeId, LteD2DMode> >* getD2DPeeringMap();
    void setD2DMode(MacNodeId src, MacNodeId dst, LteD2DMode mode);
    LteD2DMode getD2DMode(MacNodeId src, MacNodeId dst);
    bool isFrequencyReuseEnabled(MacNodeId nodeId);
    /*
     * Multicast Support
     */
    // add the group to the set of multicast group of nodeId
    void registerMulticastGroup(MacNodeId nodeId, int32 groupId);
    // check if the node is enrolled in the group
    bool isInMulticastGroup(MacNodeId nodeId, int32 groupId);
    // add one multicast transmitter
    void addD2DMulticastTransmitter(MacNodeId nodeId);
    // get multicast transmitters
    std::set<MacNodeId>& getD2DMulticastTransmitters();
    /*
     *  Handover support
     */
    void addUeHandoverTriggered(MacNodeId nodeId);
    bool hasUeHandoverTriggered(MacNodeId nodeId);
    void removeUeHandoverTriggered(MacNodeId nodeId);
    void updateUeInfoCellId(MacNodeId nodeId, MacCellId cellId);
};

#endif
