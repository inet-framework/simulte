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

#include "LteControlInfo.h"
#include "UserTxParams.h"

UserControlInfo::~UserControlInfo()
{
    if (userTxParams != 0)
        delete userTxParams;
}

UserControlInfo::UserControlInfo() :
    UserControlInfo_Base()
{
    userTxParams = 0;
    grantedBlocks.clear();
}

UserControlInfo& UserControlInfo::operator=(const UserControlInfo& other)
{
    if (&other == this)
        return *this;

    if (other.userTxParams != NULL)
    {
        const UserTxParams* txParams = check_and_cast<const UserTxParams*>(other.userTxParams);
        this->userTxParams = txParams->dup();
    }
    else
    {
        this->userTxParams = 0;
    }
    this->grantedBlocks = other.grantedBlocks;
    this->senderCoord = other.senderCoord;
    UserControlInfo_Base::operator=(other);
    return *this;
}

void UserControlInfo::setCoord(const Coord& coord)
{
    senderCoord = coord;
}

Coord UserControlInfo::getCoord() const
{
    return senderCoord;
}
