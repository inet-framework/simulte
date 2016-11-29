//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_UMRXBUFFER_H_
#define _LTE_UMRXBUFFER_H_

#include <omnetpp.h>
#include "stack/rlc/um/LteRlcUm.h"
#include "common/timer/TTimer.h"
#include "common/LteControlInfo.h"
#include "stack/pdcp_rrc/packet/LtePdcpPdu_m.h"
#include "stack/rlc/LteRlcDefs.h"

class LteMacBase;
class LteRlcUm;

/**
 * @class UmRxQueue
 * @brief Reception buffer for UM
 *
 * This module is used to reassembly packets in UM mode
 * at the RLC layer of the LTE stack. It operates in the
 * following way:
 *
 * - The receiver uses a direct method call to the defragment()
 *   function, that takes ownership of the fragment and stores it
 *
 *   FIXME Fragments are not actually stored, a bitmap is used
 *         to check if all fragments have been received and
 *         decapsulation is made only on last fragment
 *
 * - For each stored packet, performs a check to see if
 *   all fragments have been received
 *
 * - When all fragments are received, decapsulates the original packet
 *   from a fragment (all others are deleted). Remember: there is only
 *   one Main packet, the others are reference counted by fragments.
 *
 * - The main packet is then decapsulated (into a PDCP packet)
 *   end sent to upper layers (via direct method call)
 */
class UmRxQueue : public cSimpleModule
{
  public:
    UmRxQueue();
    virtual ~UmRxQueue();

    /**
     * defragment() is called by the receiver via direct method call.
     * It performs the following tasks:
     * - takes ownership of the packet
     * - stores the packet (identified by its sequence number)
     *   inside the reception buffer
     * - checks if all fragments have been received
     *   for the given sequence number:
     *   - if yes, decapsulates the original packet
     *     and sends it to the upper layers
     *   - if no returns false
     *
     * When a new packet is received (no fragments are stored for
     * this packet) the Buffer starts a timer, and if timer expires
     * acquired fragments are discarded.
     *
     * @param pkt Packet to fragment
     * @return true if packet defragmented, false otherwise
     */
    bool defragment(cPacket* pkt);

  protected:

    /**
     * Initialize watches
     */
    virtual void initialize();
    virtual void handleMessage(cMessage* msg);

    //Statistics
    static unsigned int totalCellRcvdBytes_;
    unsigned int totalRcvdBytes_;
    unsigned int totalPduRcvdBytes_;

    simsignal_t rlcCellPacketLoss_;
    simsignal_t rlcPacketLoss_;
    simsignal_t rlcPduPacketLoss_;
    simsignal_t rlcDelay_;
    simsignal_t rlcPduDelay_;
    simsignal_t rlcCellThroughput_;
    simsignal_t rlcThroughput_;
    simsignal_t rlcPduThroughput_;

    simsignal_t rlcCellPacketLossD2D_;
    simsignal_t rlcPacketLossD2D_;
    simsignal_t rlcPduPacketLossD2D_;
    simsignal_t rlcDelayD2D_;
    simsignal_t rlcPduDelayD2D_;
    simsignal_t rlcCellThroughputD2D_;
    simsignal_t rlcThroughputD2D_;
    simsignal_t rlcPduThroughputD2D_;

  private:
    /// Reception buffer
    UmFragbuf fragbuf_;

    /// Timer
    TMultiTimer timer_;

    /// Timeout for above timer
    double timeout_;

    /// reference to the serving cell (for statistics purposes)
    cModule* nodeB_;

    /// reference to the ue (for statistics purposes)
    cModule* ue_;
};

#endif

