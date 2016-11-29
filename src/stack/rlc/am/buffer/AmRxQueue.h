//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_AMRXBUFFER_H_
#define _LTE_AMRXBUFFER_H_

#include "stack/rlc/LteRlcDefs.h"
#include "common/timer/TTimer.h"
#include "stack/rlc/am/packet/LteRlcAmPdu.h"
#include "stack/rlc/am/packet/LteRlcAmSdu_m.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"

class AmRxQueue : public cSimpleModule
{
  protected:

    //! Receiver window descriptor
    RlcWindowDesc rxWindowDesc_;

    //! Minimum time between two consecutive ack messages
    simtime_t ackReportInterval_;

    //! The time when the last ack message was sent.
    simtime_t lastSentAck_;

    //! Buffer status report Interval
    simtime_t statusReportInterval_;

    //! SDU reconstructed at the beginning of the Receiver buffer
    int firstSdu_;

    //! Timer to manage the buffer status report
    TTimer timer_;

    //! AM PDU buffer
    cArray pduBuffer_;

    //! AM PDU Received vector
    /** For each AM PDU a received status variable is kept.
     */
    std::vector<bool> received_;

    //! AM PDU Discarded Vector
    /** For each AM PDU a discarded status variable is kept.
     */
    std::vector<bool> discarded_;

    /*
     * FlowControlInfo matrix : used for CTRL messages generation
     */
    FlowControlInfo* flowControlInfo_;

    //Statistics
    static unsigned int totalCellRcvdBytes_;
    unsigned int totalRcvdBytes_;
    simsignal_t rlcCellPacketLoss_;
    simsignal_t rlcPacketLoss_;
    simsignal_t rlcPduPacketLoss_;
    simsignal_t rlcDelay_;
    simsignal_t rlcPduDelay_;
    simsignal_t rlcCellThroughput_;
    simsignal_t rlcThroughput_;
    simsignal_t rlcPduThroughput_;

  public:

    AmRxQueue();

    virtual ~AmRxQueue();

    //! Receive an RLC PDU from the lower layer
    void enque(LteRlcAmPdu* pdu);

    //! Send a buffer status report to the ACK manager
    virtual void handleMessage(cMessage* msg);

    //initialize
    void initialize();

  protected:

    //! Send the RLC SDU stored in the buffer to the upper layer
    /** Note that, the buffer contains a set of RLC PDU. At most,
     *  one RLC SDU can be in the buffer!
     */
    void deque();

    //! Pass an SDU to the upper layer (RLC receiver)
    /** @param <index> index The index of the first PDU related to
     *  the target SDU (i.e.) the SDU that has been completely received
     */

    void passUp(const int index);

    //! Check if the SDU carried in the index PDU has been
    //! completely received
    void checkCompleteSdu(const int index);

    //! send buffer status report to the ACK manager
    void sendStatusReport();

    //! Compute the shift of the rx window
    int computeWindowShift() const;

    //! Move the rx window
    /** Shift the rx window. The number of position to shift is
     *  given by  seqNum - current rx first seqnum.
     */
    void moveRxWindow(const int seqNum);

    //! Discard out of MRW PDUs
    void discard(const int sn);
};

#endif
