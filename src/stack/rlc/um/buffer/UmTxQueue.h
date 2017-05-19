//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_UMTXBUFFER_H_
#define _LTE_UMTXBUFFER_H_

#include <omnetpp.h>
#include "stack/rlc/um/LteRlcUm.h"
#include "stack/rlc/LteRlcDefs.h"

/**
 * @class UmTxQueue
 * @brief Transmission buffer for UM
 *
 * This module is used to fragment packets in UM mode
 * at the RLC layer of the LTE stack. It operates in the
 * following way:
 *
 * - The sender uses a direct method call to the fragment()
 *   function, that takes ownership of the original packet.
 *   After this, the main fragment is created and the original
 *   packet is encapsulated inside it.
 *
 * - Other fragments are then created and sent using the dup() function,
 *   so that the encapsulated packet is not duplicated but reference counted
 *   (only one copy of the main packet exists).
 *
 * - All fragments are then sent back to the sender (via direct method call),
 *   so he can forward packets to lower layers. They are
 *   encapsulated as RLC fragment packets.
 *   stores it and creates as many fragments as needed.
 *
 * The size of fragments can be modified using the setFragmentSize() function
 */
class UmTxQueue : public cSimpleModule
{
  public:
    UmTxQueue()
    {
    }
    virtual ~UmTxQueue()
    {
    }

    /**
     * fragment() is called by the sender via direct method call.
     * It performs the following tasks:
     * - takes ownership of the packet
     * - creates the main fragments and encapsulates
     *   original packet inside it.
     * - creates other fragments using dup()
     * - Sends fragments to the caller, using another
     *   direct method call.
     *
     * @param pkt Packet to fragment
     */
    void fragment(cPacket* pkt);

    /**
     * getFragmentSize() returns the size of a fragment.
     * This is needed since size can be changed dynamically
     *
     * @return size of a fragment
     */
    int getFragmentSize();

    /**
     * setFragmentSize() sets the size of a fragment.
     * This is needed since size can be changed dynamically
     *
     * @param size of a fragment
     */
    void setFragmentSize(int fragmentSize);

  protected:

    /**
     * Initialize fragmentSize and
     * watches
     */
    virtual void initialize();

  private:
    /// Size of the fragments
    int fragmentSize_;
};

#endif
