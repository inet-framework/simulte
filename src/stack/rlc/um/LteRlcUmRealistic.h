//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERLCUMREALISTIC_H_
#define _LTE_LTERLCUMREALISTIC_H_

#include <omnetpp.h>
#include "LteRlcUm.h"
#include "UmTxEntity.h"
#include "UmRxEntity.h"
#include "LteRlcDataPdu.h"
#include "LteMacBase.h"

class UmTxEntity;
class UmRxEntity;

/**
 * @class LteRlcUmRealistic
 * @brief UM Module
 *
 * This is the UM Module of RLC.
 * It implements the unacknowledged mode (UM):
 *
 * - Unacknowledged mode (UM):
 *   This mode is used for data traffic. Packets arriving on
 *   this port have been already assigned a CID.
 *   UM implements fragmentation and reassembly of packets.
 *   To perform this task there is a TxEntity module for
 *   every CID = <NODE_ID,LCID>. RLC PDUs are created by the
 *   sender and reassembly is performed at the receiver by
 *   simply returning him the original packet.
 *   Traffic on this port is then forwarded on ports
 *
 *   UM mode attaches an header to the packet. The size
 *   of this header is fixed to 2 bytes.
 *
 */
class LteRlcUmRealistic : public LteRlcUm
{
  public:
    LteRlcUmRealistic()
    {
    }
    virtual ~LteRlcUmRealistic()
    {
    }

    /**
     * deleteQueues() must be called on handover
     * to delete queues for a given user
     *
     * @param nodeId Id of the node whose queues are deleted
     */
    virtual void deleteQueues(MacNodeId nodeId);

  protected:

    /**
     * Initialize watches
     */
    virtual void initialize();

    virtual void finish()
    {
    }

    /**
     * getTxBuffer() is used by the sender to gather the TXBuffer
     * for that CID. If TXBuffer was already present, a reference
     * is returned, otherwise a new TXBuffer is created,
     * added to the tx_buffers map and a reference is returned aswell.
     *
     * @param lteInfo flow-related info
     * @return pointer to the TXBuffer for the CID of the flow
     *
     */
    UmTxEntity* getTxBuffer(FlowControlInfo* lteInfo);

    /**
     * getRxBuffer() is used by the receiver to gather the RXBuffer
     * for that CID. If RXBuffer was already present, a reference
     * is returned, otherwise a new RXBuffer is created,
     * added to the rx_buffers map and a reference is returned aswell.
     *
     * @param lteInfo flow-related info
     * @return pointer to the RXBuffer for that CID
     *
     */
    UmRxEntity* getRxBuffer(FlowControlInfo* lteInfo);

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
     * The entities map associate each CID with
     * a TX/RX Entity , identified by its ID
     */
    typedef std::map<MacCid, UmTxEntity*> UmTxEntities;
    typedef std::map<MacCid, UmRxEntity*> UmRxEntities;
    UmTxEntities txEntities_;
    UmRxEntities rxEntities_;
};

#endif
