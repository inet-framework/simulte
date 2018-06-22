//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_UMRXENTITY_H_
#define _LTE_UMRXENTITY_H_

#include <omnetpp.h>
#include "stack/rlc/um/LteRlcUm.h"
#include "common/timer/TTimer.h"
#include "common/LteControlInfo.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"
#include "stack/rlc/LteRlcDefs.h"

class LteMacBase;
class LteRlcUm;
class LteRlcUmDataPdu;

/**
 * @class UmRxEntity
 * @brief Receiver entity for UM
 *
 * This module is used to buffer RLC PDUs and to reassemble
 * RLC SDUs in UM mode at RLC layer of the LTE stack.
 *
 * It implements the procedures described in 3GPP TS 36.322
 */
class UmRxEntity : public cSimpleModule
{
  public:
    UmRxEntity();
    virtual ~UmRxEntity();

    /*
     * Enqueues a lower layer packet into the PDU buffer
     * @param pdu the packet to be enqueued
     */
    void enque(cPacket* pkt);

    void setFlowControlInfo(FlowControlInfo* lteInfo) { flowControlInfo_ = lteInfo; }
    FlowControlInfo* getFlowControlInfo() { return flowControlInfo_; }

    // called when a D2D mode switch is triggered
    void rlcHandleD2DModeSwitch(bool oldConnection, bool oldMode, bool clearBuffer=true);

  protected:

    /**
     * Initialize watches
     */
    virtual void initialize();
    virtual void handleMessage(cMessage* msg);

    //Statistics
    static unsigned int totalCellPduRcvdBytes_;
    static unsigned int totalCellRcvdBytes_;
    unsigned int totalPduRcvdBytes_;
    unsigned int totalRcvdBytes_;
    simsignal_t rlcCellPacketLoss_;
    simsignal_t rlcPacketLoss_;
    simsignal_t rlcPduPacketLoss_;
    simsignal_t rlcDelay_;
    simsignal_t rlcPduDelay_;
    simsignal_t rlcCellThroughput_;
    simsignal_t rlcThroughput_;
    simsignal_t rlcPduThroughput_;

    // statistics for D2D
    simsignal_t rlcPacketLossD2D_;
    simsignal_t rlcPduPacketLossD2D_;
    simsignal_t rlcDelayD2D_;
    simsignal_t rlcPduDelayD2D_;
    simsignal_t rlcThroughputD2D_;
    simsignal_t rlcPduThroughputD2D_;

    simsignal_t rlcPacketLossTotal_;

  private:

    LteBinder* binder_;

    // reference to eNB for statistic purpose
    cModule* nodeB_;

    // Node id of the owner module
    MacNodeId ownerNodeId_;

    /*
     * Flow-related info.
     * Initialized with the control info of the first packet of the flow
     */
    FlowControlInfo* flowControlInfo_;

    // The PDU enqueue buffer.
    cArray pduBuffer_;

    // State variables
    RlcUmRxWindowDesc rxWindowDesc_;

    // Timer to manage reordering of the PDUs
    TTimer t_reordering_;

    // Timeout for above timer
    double timeout_;

    // For each PDU a received status variable is kept.
    std::vector<bool> received_;

    // The SDU waiting for the missing portion
    LteRlcSdu* buffered_;

    // Sequence number of the last SDU delivered to the upper layer
    unsigned int lastSnoDelivered_;

    // Sequence number of the last correctly reassembled PDU
    unsigned int lastPduReassembled_;

    bool init_;

    // If true, the next PDU and the corresponding SDUs are considered in order
    // (modify the lastPduReassembled_ and lastSnoDelivered_ counters)
    // useful for D2D after a mode switch
    bool resetFlag_;

    // move forward the reordering window
    void moveRxWindow(const int pos);

    // consider the PDU at position 'index' for reassembly
    void reassemble(unsigned int index);

    // deliver a PDCP PDU to the PDCP layer
    void toPdcp(LteRlcSdu* rlcSdu);
};

#endif

