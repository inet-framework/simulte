//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef LTE_LTEX2MANAGER_H_
#define LTE_LTEX2MANAGER_H_

#include "common/LteCommon.h"
#include "x2/packet/LteX2Message.h"
#include "x2/packet/X2ControlInfo_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "x2/X2AppClient.h"
#include "corenetwork/binder/LteBinder.h"

class LteX2Manager : public cSimpleModule {

    // X2 identifier
    X2NodeId nodeId_;

    // "interface table" for data gates
    // for each X2 message type, this map stores the index of the gate vector data
    // where the destination of that msg is connected to
    std::map<LteX2MessageType, int> dataInterfaceTable_;

    // "interface table" for x2 gates
    // for each destination ID, this map stores the index of the gate vector x2
    // where the X2AP for that destination is connected to
    std::map<X2NodeId, int> x2InterfaceTable_;

protected:

    void initialize(int stage);
    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg);

    virtual void fromStack(cPacket* pkt);
    virtual void fromX2(cPacket* pkt);

public:
    LteX2Manager();
    virtual ~LteX2Manager();
};

#endif /* LTE_LTEX2MANAGER_H_ */
