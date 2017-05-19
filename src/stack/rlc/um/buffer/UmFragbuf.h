//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_UMFRAGBUF_H_
#define _LTE_UMFRAGBUF_H_

#include <omnetpp.h>
#include "common/LteControlInfo.h"

/**
 * @class UmFragbuf
 * @brief Fragments buffer for UM
 *
 * This class is the fragment buffer for UM.
 * The fragments are stored inside a map that binds each
 * Main packet ID (sequence number, obtained looking
 * inside fragments) with a structure containing a vector of boolean
 * that realizes a bitmap, indicating which fragments have been received.
 *
 * The buffer can be accessed to:
 * - insert fragments.
 * - check if all fragments for a given Main packet have been gathered
 * - remove all fragments for a certain Main packet
 */
class UmFragbuf
{
  public:
    UmFragbuf()
    {
        debugCounter_ = 0;
    }
    virtual ~UmFragbuf()
    {
        // iterate over all fragments and delete corresponding LteInfo
        for (auto it=frags_.begin(); it!=frags_.end(); ++it){
            delete it->second.lteInfo_;
        }
        frags_.clear();
    }

    /**
     * insert() is called by the RXBuffer when a new fragment
     * has been received. It adds the received packet to the receiver Buffer
     * through the following procedure:
     *
     * - If in the receiver buffer there isn't a vector for the
     *   Main Packet with this ID, insert a new pair <pktID,vector> where
     *   the vector is set to false for all entries of this Main Packet.
     * - Else just set the bit in the vector for the corresponding fragment
     *   Sequence number
     *
     * @param pktID Sequence number of Main Packet
     * @param totFrag Total number of fragments
     * @param fragSno Sequence number of the fragment
     * @param fragSize Size of the fragment
     * @return true if this was the first insertion, false otherwise
     */
    bool insert(unsigned int pktId, unsigned int totFrag,
        unsigned int fragSno, unsigned int fragSize, FlowControlInfo* info);

    /**
     * check() is called by the RXBuffer to check if all
     * fragments have been received.
     *
     * @param pktID Sequence number of Main Packet
     * @return true if all fragments have been received, false otherwise.
     */
    bool check(unsigned int pktId);

    /**
     * remove() is called by the RXBuffer only when all
     * fragments have been received. It removes the fragments
     * from the buffer and returns the original packet size.
     *
     * @param pktID Sequence number of Main Packet
     * @return Main Packet Size
     */
    int remove(unsigned int pktId);

    /**
     * operator << prints the content of the fragments
     * buffer using the following format:
     * Packet: <SequenceNumber> has <totalFragments> fragments
     *
     * @param stream output stream
     * @param rxBuffer pointer to the buffer
     * @return output stream
     */
    friend std::ostream &operator << (std::ostream &stream, const UmFragbuf* rxBuffer);
    /**
     * Get LteInfo for statistics purpose
     */
    FlowControlInfo* getLteInfo(unsigned int pktId);

  private:

    /**
     * \struct MainPktInfo
     * \brief Original Packet infos gathered from fragments
     *
     * This structure is updated for each fragment received
     * for a given main packet and contains:
     * - The size of the original packet
     *   (updated for every received fragment)
     * - The number of fragments left to gather
     * - a vector of booleans that tells us what
     *   fragments have been received
     */
    struct MainPktInfo_
    {
        std::vector<bool> bitmap_;
        int fragsleft_;
        unsigned int size_;
        FlowControlInfo* lteInfo_;
    };

    /**
     * frags is a map associating each Main Packet
     * (identified by its ID) with the informations
     * gathered about it
     */
    std::map<unsigned int, MainPktInfo_> frags_;

    unsigned int debugCounter_;
};

#endif
