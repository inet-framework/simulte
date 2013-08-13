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

void UserControlInfo::setCoord(const Coord& coord)
{
    senderCoord = coord;
}

Coord UserControlInfo::getCoord() const
{
    return senderCoord;
}
