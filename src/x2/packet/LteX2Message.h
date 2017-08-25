//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEX2MESSAGE_H_
#define _LTE_LTEX2MESSAGE_H_

#include "x2/packet/LteX2Message_m.h"
#include "common/LteCommon.h"
#include "x2/packet/X2InformationElement.h"

using namespace omnetpp;

// add here new X2 message types
enum LteX2MessageType
{
    X2_COMP_MSG, X2_HANDOVER_CONTROL_MSG, X2_HANDOVER_DATA_MSG, X2_UNKNOWN_MSG
};

/**
 * @class LteX2Message
 * @brief Base class for X2 messages
 *
 * Class derived from base class contained
 * in msg declaration: adds the Information Elements list
 *
 * Create new X2 Messages by deriving this class
 */
class LteX2Message : public LteX2Message_Base
{
  protected:

    /// type of the X2 message
    LteX2MessageType type_;

    /// List of X2 IEs
    X2InformationElementsList ieList_;

    /// Size of the X2 message
    int64_t msgLength_;

  public:

    /**
     * Constructor
     */
    LteX2Message(const char* name = NULL, int kind = 0) :
        LteX2Message_Base(name, kind)
    {
        type_ = X2_UNKNOWN_MSG;
        ieList_.clear();
        msgLength_ = 0;
    }

    /*
     * Copy constructors
     * FIXME Copy constructors do not preserve ownership
     * Here I should iterate on the list and set all ownerships
     */
    LteX2Message(const LteX2Message& other) :
        LteX2Message_Base()
    {
        operator=(other);
    }

    LteX2Message& operator=(const LteX2Message& other)
    {
        if (&other == this)
            return *this;
        LteX2Message_Base::operator=(other);
        type_ = other.type_;
        ieList_ = other.ieList_;
        msgLength_ = other.msgLength_;
        return *this;
    }

    virtual LteX2Message *dup() const
    {
        return new LteX2Message(*this);
    }

    virtual ~LteX2Message()
    {
        while(!ieList_.empty())
        {
            ieList_.pop_front();
        }
    }

    // getter/setter methods for the type field
    void setType(LteX2MessageType type) { type_ = type; }
    LteX2MessageType getType() { return type_; }

    /**
     * pushIe() stores a IE inside the
     * X2 IE list in back position and update msg length
     *
     * @param ie IE to store
     */
    virtual void pushIe(X2InformationElement* ie)
    {
        ieList_.push_back(ie);
        msgLength_ += ie->getLength();
    }

    /**
     * popIe() pops a IE from front of
     * the IE list and returns it
     *
     * @return popped IE
     */
    virtual X2InformationElement* popIe()
    {
        X2InformationElement* ie = ieList_.front();
        ieList_.pop_front();
        msgLength_ -= ie->getLength();
        return ie;
    }

    /**
     * hasIe() verifies if there are other
     * IEs inside the ie list
     *
     * @return true if list is not empty, false otherwise
     */
    virtual bool hasIe() const
    {
        return (!ieList_.empty());
    }

    int64_t getByteLength() const
    {
        return msgLength_;
    }

    int64_t getBitLength() const
    {
        return (getByteLength() * 8);
    }
};

Register_Class(LteX2Message);

#endif

