//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEMACSDUREQUEST_H_
#define _LTE_LTEMACSDUREQUEST_H_

#include "stack/mac/packet/LteMacSduRequest_m.h"
#include "common/LteCommon.h"

/**
 * @class LteMacSduRequest
 * @brief Lte MAC SDU Request message
 *
 * Class derived from base class contained
 * in msg declaration: adds UE ID and Logical Connection ID
 */
class LteMacSduRequest : public LteMacSduRequest_Base
{
  protected:
    /// ID of the destination UE associated with the request
    MacNodeId ueId_;

    /// Logical Connection identifier associated with the request
    LogicalCid lcid_;

  public:

    /**
     * Constructor
     */
    LteMacSduRequest(const char* name = NULL, int kind = 0) :
        LteMacSduRequest_Base(name, kind)
    {
    }

    /**
     * Destructor
     */
    virtual ~LteMacSduRequest()
    {
    }

    LteMacSduRequest(const LteMacSduRequest& other) :
        LteMacSduRequest_Base()
    {
        operator=(other);
    }

    LteMacSduRequest& operator=(const LteMacSduRequest& other)
    {
        if (&other == this)
            return *this;
        LteMacSduRequest_Base::operator=(other);
        return *this;
    }

    virtual LteMacSduRequest *dup() const
    {
        return new LteMacSduRequest(*this);
    }
    MacNodeId getUeId() { return ueId_; }
    void setUeId(MacNodeId ueId) { ueId_ = ueId; }

    LogicalCid getLcid() { return lcid_; }
    void setLcid(LogicalCid lcid) { lcid_ = lcid; }
};

Register_Class(LteMacSduRequest);

#endif

