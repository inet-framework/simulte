//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERLCUM_H_
#define _LTE_LTERLCUM_H_

#include <omnetpp.h>
#include "LteCommon.h"
#include "LteControlInfo.h"
#include "LteRlcSdu_m.h"
#include "UmTxQueue.h"
#include "UmRxQueue.h"

class UmTxQueue;
class UmRxQueue;

/**
 * @class LteRlcUm
 * @brief UM Module
 *
 * This is the UM Module of RLC.
 * It implements the unacknowledged mode (UM):
 *
 * - Unacknowledged mode (UM):
 *   This mode is used for data traffic. Packets arriving on
 *   this port have been already assigned a CID.
 *   UM implements fragmentation and reassembly of packets.
 *   To perform this task there is a TXBuffer module for
 *   every CID = <NODE_ID,LCID>. Fragments are created by the
 *   sender and reassembly is performed at the receiver by
 *   simply returning him the original packet.
 *   Traffic on this port is then forwarded on ports
 *
 *   UM mode attaches an header to the packet. The size
 *   of this header is fixed to 2 bytes.
 *
 */
class LteRlcUm : public cSimpleModule
{
  public:
    LteRlcUm()
    {
    }
    virtual ~LteRlcUm()
    {
    }

    /**
     * sendFragmented() is invoked by the TXBuffer as a direct method
     * call and used to forward fragments to lower layers. This is needed
     * since the TXBuffer himself has no output gates
     *
     * @param pkt packet to forward
     */
    void sendFragmented(cPacket *pkt);

    /**
     * sendDefragmented() is invoked by the RXBuffer as a direct method
     * call and used to forward fragments to upper layers. This is needed
     * since the RXBuffer himself has no output gates
     *
     * @param pkt packet to forward
     */
    void sendDefragmented(cPacket *pkt);

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    virtual void deleteQueues(MacNodeId nodeId);

    /**
     * sendToLowerLayer() is identical to sendFragmented(), but it is invoked
     * by TxEntity instead of TxBuffer (hence it is for RlcUmRealistic) and
     * it is a virtual function (for example, RlcUmRealistic needs to
     * redefine the behaviour of this function)
     *
     * @param pkt packet to forward
     */
    virtual void sendToLowerLayer(cPacket *pkt);

  protected:

    cGate* up_[2];
    cGate* down_[2];

    /**
     * Initialize watches
     */
    virtual void initialize();

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(cMessage *msg);

    virtual void finish()
    {
    }

  private:
    /**
     * getTxBuffer() is used by the sender to gather the TXBuffer
     * for that CID. If TXBuffer was already present, a reference
     * is returned, otherwise a new TXBuffer is created,
     * added to the tx_buffers map and a reference is returned aswell.
     *
     * @param lcid Logical Connection ID
     * @param nodeId MAC Node Id
     * @return pointer to the TXBuffer for that CID
     *
     */
    UmTxQueue* getTxBuffer(MacNodeId nodeId, LogicalCid lcid);

    /**
     * getRxBuffer() is used by the receiver to gather the RXBuffer
     * for that CID. If RXBuffer was already present, a reference
     * is returned, otherwise a new RXBuffer is created,
     * added to the rx_buffers map and a reference is returned aswell.
     *
     * @param lcid Logical Connection ID
     * @param nodeId MAC Node Id
     * @return pointer to the RXBuffer for that CID
     *
     */
    UmRxQueue* getRxBuffer(MacNodeId nodeId, LogicalCid lcid);

    /**
     * handler for traffic coming
     * from the upper layer (PDCP)
     *
     * handleUpperMessage() performs the following tasks:
     * - Adds the RLC-UM header to the packet, containing
     *   the CID, the Traffic Type and the Sequence Number
     *   of the packet (extracted from the IP Datagram)
     * - Search (or add) the proper TXBuffer, depending
     *   on the packet CID
     * - Calls the TXBuffer, that from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    virtual void handleUpperMessage(cPacket *pkt);

    /**
     * UM Mode
     *
     * handler for traffic coming from
     * lower layer (DTCH, MTCH, MCCH).
     *
     * handleLowerMessage() performs the following task:
     *
     * - Search (or add) the proper RXBuffer, depending
     *   on the packet CID
     * - Calls the RXBuffer, that from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    virtual void handleLowerMessage(cPacket *pkt);

    /*
     * Data structures
     */

    /**
     * The buffers map associate each CID with
     * a TX/RX Buffer , identified by its ID
     */
    typedef std::map<MacCid, UmTxQueue*> UmTxBuffers;
    typedef std::map<MacCid, UmRxQueue*> UmRxBuffers;
    UmTxBuffers txBuffers_;
    UmRxBuffers rxBuffers_;
};

#endif
