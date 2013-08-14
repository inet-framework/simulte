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

#ifndef _LTE_LTECONTROLINFO_H_
#define _LTE_LTECONTROLINFO_H_

#include "LteControlInfo_m.h"
#include <vector>
class UserTxParams;

/**
 * @class UserControlInfo
 * @brief ControlInfo used in the Lte model
 *
 * This is the LteControlInfo Resource Block Vector
 * and Remote Antennas Set
 *
 */
class UserControlInfo : public UserControlInfo_Base
{
  protected:

    const UserTxParams* userTxParams;
    RbMap grantedBlocks;
    /** @brief The movement of the sending host.*/
    //Move senderMovement;
    /** @brief The playground position of the sending host.*/
    Coord senderCoord;

  public:

    /**
     * Constructor: base initialization
     * @param name packet name
     * @param kind packet kind
     */
    UserControlInfo();
    virtual ~UserControlInfo();
    /**
     * Copy constructor: packet copy
     * @param other source packet
     */
    UserControlInfo(const UserControlInfo& other) :
        UserControlInfo_Base()
    {
        operator=(other);
    }

    /**
     * Operator = : packet copy
     * @param other source packet
     * @return reference to this packet
     */

    UserControlInfo& operator=(const UserControlInfo& other)
    {
        if (&other == this)
            return *this;

        this->userTxParams = other.userTxParams;
        this->grantedBlocks = other.grantedBlocks;
        this->senderCoord = other.senderCoord;
        UserControlInfo_Base::operator=(other);
        return *this;
    }

    /**
     * dup() : packet duplicate
     * @return pointer to duplicate packet
     */
    virtual UserControlInfo *dup() const
    {
        return new UserControlInfo(*this);
    }

    void setUserTxParams(const UserTxParams* arg)
    {
        userTxParams = arg;
    }

    const UserTxParams* getUserTxParams() const
    {
        return userTxParams;
    }

    const unsigned int getBlocks(Remote antenna, Band b) const
        {
        return grantedBlocks.at(antenna).at(b);
    }

    void setBlocks(Remote antenna, Band b, const unsigned int blocks)
    {
        grantedBlocks[antenna][b] = blocks;
    }

    const RbMap& getGrantedBlocks() const
    {
        return grantedBlocks;
    }

    void setGrantedBlocks(const RbMap& rbMap)
    {
        grantedBlocks = rbMap;
    }

    // struct used to request a feedback computation by nodeB
    FeedbackRequest feedbackReq;
    void setCoord(const Coord& coord);
    Coord getCoord() const;
};

Register_Class(UserControlInfo);

#endif

