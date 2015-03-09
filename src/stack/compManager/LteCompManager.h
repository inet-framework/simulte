//
//                           SimuLTE
// Copyright (C) 2015 Antonio Virdis, Giovanni Nardini, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef LTE_LTECOMPMANAGER_H_
#define LTE_LTECOMPMANAGER_H_

#include "LteCommon.h"
#include "LteMacEnb.h"
#include "LteMacBuffer.h"
#include "X2ControlInfo_m.h"
#include "X2CompMsg.h"
#include "X2CompRequestIE.h"
#include "X2CompReplyIE.h"

enum CompNodeType
{
    COMP_MASTER,
    COMP_SLAVE
};

class LteCompManager : public cSimpleModule {

    // X2 identifier
    X2NodeId nodeId_;

    // reference to the gates
    cGate* x2Manager_[2];

    // reference to the MAC layer
    LteMacEnb* mac_;

    // master or slave?
    CompNodeType type_;

    /// TTI self message
    cMessage* ttiTick_;

    // Last received usable bands
    UsableBands usableBands_;

    /*
     * Master-related info
     */
    // IDs of the eNB that are slaves of this master node
    std::vector<X2NodeId> slavesList_;
    // requests from slaves
    std::map<X2NodeId, bool> receivedRequests_;
    // requests from slaves
    std::map<X2NodeId, unsigned int> reqBlocksMap_;

    /*
     * Slave-related info
     */
    X2NodeId masterId_;

    // statistics
    simsignal_t compReservedBlocks_;

    // utility function: convert a vector of double to a vector of integer,
    // preserving the sum of the elements
    std::vector<unsigned int> roundVector(std::vector<double>& vec, int sum);

    UsableBands parseAllowedBlocksMap(std::vector<CompRbStatus>& allowedBlocksMap);
    void setUsableBands(UsableBands& usableBands);

protected:

    void initialize();
    void handleMessage(cMessage *msg);
    void handleSelfMessage();

    /*
     * Master methods
     */
    // store requests coming from slave nodes
    void handleRequestFromSlave(cPacket* pkt);
    // run the partition algorithm: bandwidth is partitioned proportionally among slaves
    // according to their requests
    void doBandwidthPartitioning();
    // send the map of allowed blocks to slave nodes
    void sendReplyToSlave(X2NodeId nodeId, std::vector<CompRbStatus> allowedBlocksMap);

    /*
     * Slave methods
     */
    // receive the map of allowed blocks from master node
    void handleReplyFromMaster(cPacket* pkt);

    /*
     * Common methods
     */
    // generate the number of required blocks to satisfy the eNB's load
    void provisionalSchedule();

public:
    LteCompManager() {}
    virtual ~LteCompManager() {}
};

#endif /* LTE_LTECOMPMANAGER_H_ */
