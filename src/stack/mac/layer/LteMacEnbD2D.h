//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEMACENBD2D_H_
#define _LTE_LTEMACENBD2D_H_

#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2DMirror.h"

typedef std::map<MacNodeId, LteHarqBufferRxD2DMirror*> HarqRxBuffersMirror;

class LteMacEnbD2D : public LteMacEnb
{
  protected:

    /*
     * Map associating a nodeId with the corresponding RX H-ARQ buffer.
     * Used in eNB for D2D communications. The key value of the map is
     * the *receiver* of the D2D flow
     */
    HarqRxBuffersMirror harqRxBuffersD2DMirror_;

    // if true, use the preconfigured TX params for transmission, else use that signaled by the eNB
    bool usePreconfiguredTxParams_;
    UserTxParams* preconfiguredTxParams_;

    /**
     * Reads MAC parameters for eNb and performs initialization.
     */
    virtual void initialize(int stage);

    /**
     * creates scheduling grants (one for each nodeId) according to the Schedule List.
     * It sends them to the  lower layer
     */
    virtual void sendGrants(LteMacScheduleList* scheduleList);

    /**
     * bufferizeBsr() works much alike bufferizePacket()
     * but only saves the BSR in the corresponding virtual
     * buffer, eventually creating it if a queue for that
     * cid does not exists yet.
     *
     * @param bsr bsr to store
     * @param cid connection id for this bsr
     */
    void bufferizeBsr(MacBsr* bsr, MacCid cid);

    /**
     * macPduUnmake() extracts SDUs from a received MAC
     * PDU and sends them to the upper layer.
     *
     * On ENB it also extracts the BSR Control Element
     * and stores it in the BSR buffer (for the cid from
     * which packet was received)
     *
     * @param pkt container packet
     */
    virtual void macPduUnmake(cPacket* pkt);

    /**
     * macHandleFeedbackPkt is called every time a feedback pkt arrives on MAC
     */
    virtual void macHandleFeedbackPkt(cPacket* pkt);

  public:

    LteMacEnbD2D();
    virtual ~LteMacEnbD2D();

    virtual bool isD2DCapable()
    {
        return true;
    }

    // update the status of the "mirror" RX-Harq Buffer for this node
    void storeRxHarqBufferMirror(MacNodeId id, LteHarqBufferRxD2DMirror* mirbuff);
    // get the reference to the "mirror" buffers
    HarqRxBuffersMirror* getRxHarqBufferMirror();
    // delete the "mirror" RX-Harq Buffer for this node (useful at mode switch)
    void deleteRxHarqBufferMirror(MacNodeId id);
    // send the D2D Mode Switch signal to the transmitter of the given flow
    void sendModeSwitchNotification(MacNodeId srcId, MacNodeId dst, LteD2DMode oldMode, LteD2DMode newMode);
};

#endif
