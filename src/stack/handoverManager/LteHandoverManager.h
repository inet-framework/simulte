//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef LTE_LTEHANDOVERMANAGER_H_
#define LTE_LTEHANDOVERMANAGER_H_

#include "LteCommon.h"
#include "X2ControlInfo_m.h"
#include "X2HandoverControlMsg.h"
#include "X2HandoverDataMsg.h"
#include "IP2lte.h"

class IP2lte;

//
// LteHandoverManager
//
class LteHandoverManager : public cSimpleModule
{

  protected:

    // X2 identifier
    X2NodeId nodeId_;

    // reference to the gates
    cGate* x2Manager_[2];

    // reference to the PDCP layer
    IP2lte* ip2lte_;

    // flag for seamless/lossless handover
    bool losslessHandover_;

    void handleX2Message(cPacket* pkt);

  public:
    LteHandoverManager() {}
    virtual ~LteHandoverManager() {}

    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    // send handover command on X2 to the eNB
    void sendHandoverCommand(MacNodeId ueId, MacNodeId enb, bool startHo);

    // receive handover command on X2 from the source eNB
    void receiveHandoverCommand(MacNodeId ueId, MacNodeId eEnb, bool startHo);

    // send an IP datagram to the X2 Manager
    void forwardDataToTargetEnb(IPv4Datagram* datagram, MacNodeId targetEnb);

    // receive data from X2 message and send it to the X2 Manager
    void receiveDataFromSourceEnb(IPv4Datagram* datagram, MacNodeId sourceEnb);
};

#endif /* LTE_LTEHANDOVERMANAGER_H_ */
