//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEMACPDU_H_
#define _LTE_LTEMACPDU_H_

#include "stack/mac/packet/LteMacPdu_m.h"
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"

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
    cPacketQueue* sduList_;

    /// List of MAC CEs
    MacControlElementsList ceList_;

    /// Length of the PDU
    inet::int64 macPduLength_;

    /**
     * ID of the MAC PDU
     * at the creation, the ID is set equal to cMessage::msgid. When the message is duplicated,
     * the new message gets a different msgid. However, we need that multiple copies of the
     * same PDU have the same PDU ID (for example, multiple copies of a broadcast transmission)
     */
    inet::int64 macPduId_;

  public:

    /**
     * Constructor
     */
    LteMacPdu(const char* name = NULL, int kind = 0) :
        LteMacPdu_Base(name, kind)
    {
        macPduLength_ = 0;
        sduList_ = new cPacketQueue("SDU List");
        take(sduList_);
        macPduId_ = cMessage::getId();
    }

    /*
     * Copy constructors
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
        macPduLength_ = other.macPduLength_;
        macPduId_ = other.macPduId_;

        sduList_ = other.sduList_->dup();
        take(sduList_);

        // duplicate MacControlElementsList (includes BSRs)
        ceList_ = std::list<MacControlElement*> ();
        MacControlElementsList otherCeList = other.ceList_;
        MacControlElementsList::iterator cit;
        for (cit = otherCeList.begin(); cit != otherCeList.end(); cit++){
            MacBsr* bsr = dynamic_cast<MacBsr *> (*cit);
            if(bsr)
            {
                ceList_.push_back(new MacBsr(*bsr));
            }
            else
            {
                ceList_.push_back(new MacControlElement(**cit));
            }
        }

        // duplicate control info - if it exists
        cObject* ci = other.getControlInfo();
        if(ci){
            UserControlInfo * uci = dynamic_cast<UserControlInfo *> (other.getControlInfo());

            if (uci) {
                setControlInfo(uci->dup());
            } else {
                throw cRuntimeError("LteMacPdu.h::Unknown type of control info!");
            }
        }

        // duplication of the SDU queue duplicates all packets but not
        // the ControlInfo - iterate over all packets and restore ControlInfo if necessary
        cPacketQueue::Iterator iterOther(*other.sduList_);
        for(cPacketQueue::Iterator iter(*sduList_); !iter.end(); iter++){
            cPacket *p1 = (cPacket *) *iter;
            cPacket *p2 = (cPacket *) *iterOther;
            if(p1->getControlInfo() == NULL && p2->getControlInfo() != NULL){
                FlowControlInfo * fci = dynamic_cast<FlowControlInfo *> (p2->getControlInfo());
                if(fci){
                    p1->setControlInfo(new FlowControlInfo(*fci));
                } else {
                    throw cRuntimeError("LteMacPdu.h::Unknown type of control info in SDU list!");
                }
            }

            iterOther++;
        }

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
            << sduList_->getLength() << " SDUs and " << ceList_.size() << " CEs"
            << " with size " << getByteLength();
        s = ss.str();
        return s;
    }

    /**
     * Destructor
     */
    virtual ~LteMacPdu()
    {
        // delete the SDU queue
        // (since it is derived of cPacketQueue, it will automatically delete all contained SDUs)

        // ASSERT(sduList_->getOwner() == this); // should not throw an exception in a destructor
        drop(sduList_);
        delete sduList_;

        MacControlElementsList::iterator cit;
        for (cit = ceList_.begin(); cit != ceList_.end(); cit++){
            delete *cit;
        }

        // remove and delete control UserControlInfo - if it exists
        cObject * ci = removeControlInfo();
        if(ci){
            delete ci;
        }

    }

    virtual void setSduArraySize(unsigned int size)
    {
        ASSERT(false);
    }

    virtual unsigned int getSduArraySize() const
    {
        return sduList_->getLength();
    }
    virtual cPacket& getSdu(unsigned int k)
    {
        return *sduList_->get(k);
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
        // sduList_ will take ownership
        drop(pkt);
        sduList_->insert(pkt);
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
        cPacket* pkt = sduList_->pop();
        macPduLength_ -= pkt->getByteLength();
        take(pkt);
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
        return (!sduList_->isEmpty());
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
    inet::int64 getByteLength() const
    {
        return macPduLength_ + getHeaderLength();
    }

    inet::int64 getBitLength() const
    {
        return (getByteLength() * 8);
    }

    long getId() const
    {
        return macPduId_;
    }
};

Register_Class(LteMacPdu);

#endif

