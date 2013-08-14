//
//                           SimuLTE
// Copyright (C) 2012 Antonio Virdis, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEMACPDU_H_
#define _LTE_LTEMACPDU_H_

#include "LteMacPdu_m.h"
#include "LteCommon.h"

/**
 * @class LteMacPdu
 * @brief Lte MAC Pdu
 *
 * Class derived from base class contained
 * in msg declaration: adds the sdu and control elements list
 * TODO: Add Control Elements
 */
class LteMacPdu : public LteMacPdu_Base
{
  protected:
    /// List Of MAC SDUs
    MacSduList sduList_;

    /// List of MAC CEs
    MacControlElementsList ceList_;

    /// Length of the PDU
    int64 macPduLength_;

  public:

    /**
     * Constructor
     */
    LteMacPdu(const char* name = NULL, int kind = 0) :
        LteMacPdu_Base(name, kind)
    {
        macPduLength_ = 0;
    }

    /*
     * Copy constructors
     * FIXME Copy constructors do not preserve ownership
     * Here I should iterate on the list and set all ownerships
     */

    LteMacPdu(const LteMacPdu& other) :
        LteMacPdu_Base()
    {
        operator=(other);
    }

    LteMacPdu& operator=(const LteMacPdu& other)
    {
        if (&other == this)
            return *this;
        LteMacPdu_Base::operator=(other);
        sduList_ = other.sduList_;
        ceList_ = other.ceList_;
        return *this;
    }

    virtual LteMacPdu *dup() const
    {
        return new LteMacPdu(*this);
    }

    /**
     * info() prints a one line description of this object
     */
    std::string info() const
    {
        std::stringstream ss;
        std::string s;
        ss << (std::string) getName() << " containing "
            << sduList_.size() << " SDUs and " << ceList_.size() << " CEs"
            << " with size " << getByteLength();
        s = ss.str();
        return s;
    }

    /**
     * Destructor
     */
    virtual ~LteMacPdu()
    {
        // Needs to delete all contained packets
        MacSduList::iterator sit;
        for (sit = sduList_.begin(); sit != sduList_.end(); sit++)
            dropAndDelete(*sit);

        MacControlElementsList::iterator cit;
        for (cit = ceList_.begin(); cit != ceList_.end(); cit++)
            delete *cit;
    }

    virtual void setSduArraySize(unsigned int size)
    {
        ASSERT(false);
    }
    virtual unsigned int getSduArraySize() const
    {
        return sduList_.size();
    }
    virtual cPacket& getSdu(unsigned int k)
    {
        MacSduList::iterator it = sduList_.begin();
        advance(it, k);
        return **it;
    }
    virtual void setSdu(unsigned int k, const cPacket& sdu)
    {
        ASSERT(false);
    }

    /**
     * pushSdu() gets ownership of the packet
     * and stores it inside the mac sdu list
     * in back position
     *
     * @param pkt packet to store
     */
    virtual void pushSdu(cPacket* pkt)
    {
        take(pkt);
        macPduLength_ += pkt->getByteLength();
        sduList_.push_back(pkt);
    }

    /**
     * popSdu() pops a packet from front of
     * the sdu list and drops ownership before
     * returning it
     *
     * @return popped packet
     */
    virtual cPacket* popSdu()
    {
        cPacket* pkt = sduList_.front();
        sduList_.pop_front();
        macPduLength_ -= pkt->getByteLength();
        drop(pkt);
        return pkt;
    }

    /**
     * hasSdu() verifies if there are other
     * SDUs inside the sdu list
     *
     * @return true if list is not empty, false otherwise
     */
    virtual bool hasSdu() const
    {
        return (!sduList_.empty());
    }

    /**
     * pushCe() stores a CE inside the
     * MAC CE list in back position
     *
     * @param pkt CE to store
     */
    virtual void pushCe(MacControlElement* ce)
    {
        ceList_.push_back(ce);
    }

    /**
     * popCe() pops a CE from front of
     * the CE list and returns it
     *
     * @return popped CE
     */
    virtual MacControlElement* popCe()
    {
        MacControlElement* ce = ceList_.front();
        ceList_.pop_front();
        return ce;
    }

    /**
     * hasCe() verifies if there are other
     * CEs inside the ce list
     *
     * @return true if list is not empty, false otherwise
     */
    virtual bool hasCe() const
    {
        return (!ceList_.empty());
    }

    /**
     *
     */
    int64 getByteLength() const
    {
        return macPduLength_ + getHeaderLength();
    }

    int64 getBitLength() const
    {
        return (getByteLength() * 8);
    }
};

Register_Class(LteMacPdu);

#endif

