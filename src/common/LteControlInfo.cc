//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "common/LteControlInfo.h"
#include "stack/mac/amc/UserTxParams.h"

using namespace inet;

UserControlInfo::~UserControlInfo()
{
    if (userTxParams != nullptr)
    {
        delete userTxParams;
        userTxParams = nullptr;
    }
}

UserControlInfo::UserControlInfo() :
    UserControlInfo_Base()
{
    userTxParams = nullptr;
    grantedBlocks.clear();
}

UserControlInfo& UserControlInfo::operator=(const UserControlInfo& other)
{
    if (&other == this)
        return *this;

    if (other.userTxParams != nullptr)
    {
        const UserTxParams* txParams = check_and_cast<const UserTxParams*>(other.userTxParams);
        this->userTxParams = txParams->dup();
    }
    else
    {
        this->userTxParams = nullptr;
    }
    this->grantedBlocks = other.grantedBlocks;
    this->senderCoord = other.senderCoord;
    UserControlInfo_Base::operator=(other);
    return *this;
}

void UserControlInfo::setCoord(const inet::Coord& coord)
{
    senderCoord = coord;
}

void UserControlInfo::setUserTxParams(const UserTxParams *newParams)
{
    if(userTxParams != nullptr){
        delete userTxParams;
    }
    userTxParams = newParams;
}


inet::Coord UserControlInfo::getCoord() const
{
    return senderCoord;
}
