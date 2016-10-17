//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_UMTXENTITY_H_
#define _LTE_UMTXENTITY_H_

#include <omnetpp.h>
#include "LteRlcUmRealistic.h"
#include "LteRlcDefs.h"

/**
 * @class UmTxEntity
 * @brief Transmission entity for UM
 *
 * This module is used to segment and/or concatenate RLC SDUs
 * in UM mode at RLC layer of the LTE stack.It operates in
 * the following way:
 *
 * - When a RLC SDU is received from upper layer:
 *    a) the RLC SDU is buffered;
 *    b) the arrival of new data is notified to the lower layer.
 *
 * - When lower layer requests for a RLC PDU, this module invokes
 *   the rlcPduMake() function that builds a new SDU by segmenting
 *   and/or concatenating original SDUs stored in the buffer.
 *   Additional information are added to the SDU in order to allow
 *   the receiving RLC entity to rebuild the original SDUs
 *
 * - The newly created SDU is encapsulated into a RLC PDU and sent
 *   to the lower layer
 *
 * The size of PDUs is signalled by the lower layer
 */
class UmTxEntity : public cSimpleModule
{
  public:
    UmTxEntity()
    {
        flowControlInfo_ = NULL;
    }
    virtual ~UmTxEntity()
    {
        delete flowControlInfo_;
    }

    /*
     * Enqueues an upper layer packet into the SDU buffer
     * @param pkt the packet to be enqueued
     */
    void enque(cPacket* pkt);

    /**
     * rlcPduMake() creates a PDU having the specified size
     * and sends it to the lower layer
     *
     * @param size of a pdu
     */
    void rlcPduMake(int pduSize);

    void setFlowControlInfo(FlowControlInfo* lteInfo) { flowControlInfo_ = lteInfo; }
    FlowControlInfo* getFlowControlInfo() { return flowControlInfo_; }

    // force the sequence number to assume the sno passed as argument
    void setNextSequenceNumber(unsigned int nextSno) { sno_ = nextSno; }

    // remove the last SDU from the queue
    void removeDataFromQueue();

    // called when a D2D mode switch is triggered
    void rlcHandleD2DModeSwitch(bool oldConnection);

  protected:

    /*
     * Flow-related info.
     * Initialized with the control info of the first packet of the flow
     */
    FlowControlInfo* flowControlInfo_;

    /*
     * The SDU enqueue buffer.
     */
    cPacketQueue sduQueue_;

    /*
     * Determine whether the first item in the queue is a fragment or a whole SDU
     */
    bool firstIsFragment_;

    /**
     * Initialize fragmentSize and
     * watches
     */
    virtual void initialize();

  private:

    // Node id of the owner module
    MacNodeId ownerNodeId_;

    /// Next PDU sequence number to be assigned
    unsigned int sno_;
};

#endif
