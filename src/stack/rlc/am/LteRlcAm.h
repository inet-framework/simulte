//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERLCAM_H_
#define _LTE_LTERLCAM_H_

#include <omnetpp.h>
#include "common/LteCommon.h"

class AmTxQueue;
class AmRxQueue;

/**
 * @class LteRlcAm
 * @brief AM Module
 *
 * This is the AM Module of RLC.
 * It implements the acknowledged mode (AM):
 *
 * TODO
 */
class LteRlcAm : public cSimpleModule
{
  protected:

    /*
     * Data structures
     */

    typedef std::map<MacCid, AmTxQueue*> AmTxBuffers;
    typedef std::map<MacCid, AmRxQueue*> AmRxBuffers;

    /**
     * The buffers map associate each CID with
     * a TX/RX Buffer , identified by its ID
     */

    AmTxBuffers txBuffers_;
    AmRxBuffers rxBuffers_;

    cGate* up_[2];
    cGate* down_[2];

  public:
    LteRlcAm()
    {
    }
    virtual ~LteRlcAm()
    {
    }

  protected:

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(cMessage *msg);

    virtual void initialize();
    virtual void finish()
    {
    }

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    void deleteQueues(MacNodeId nodeId);

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
    AmTxQueue* getTxBuffer(MacNodeId nodeId, LogicalCid lcid);

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
    AmRxQueue* getRxBuffer(MacNodeId nodeId, LogicalCid lcid);

    /**
     * handler for traffic coming
     * from the upper layer (PDCP)
     *
     * handleUpperMessage() performs the following tasks:
     * - Adds the RLC-Am header to the packet, containing
     *   the CID, the Traffic Type and the Sequence NAmber
     *   of the packet (extracted from the IP Datagram)
     * - Search (or add) the proper TXBuffer, depending
     *   on the packet CID
     * - Calls the TXBuffer, that from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    void handleUpperMessage(cPacket *pkt);

    /**
     * Am Mode
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
    void handleLowerMessage(cPacket *pkt);

  public:
    /**
     * handler for control messages coming
     * from receiver AM entities
     *
     *   routeControlMessage() performs the following task:
     * - Search the proper TXBuffer, depending
     *   on the packet CID and deliver the control packet to it
     *
     * @param pkt packet to process
     */
    void routeControlMessage(cPacket *pkt);
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
};

#endif
