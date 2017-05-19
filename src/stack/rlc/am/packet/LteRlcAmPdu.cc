// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

#include "stack/rlc/am/packet/LteRlcAmPdu.h"

void
LteRlcAmPdu::setBitmapArraySize(unsigned int size)
{
    this->bitmap_.resize(size);
}

unsigned int
LteRlcAmPdu::getBitmapArraySize() const
{
    return this->bitmap_.size();
}

bool
LteRlcAmPdu::getBitmap(unsigned int k) const
    {
    return this->bitmap_.at(k);
}

void
LteRlcAmPdu::setBitmap(unsigned int k, bool bitmap_)
{
    this->bitmap_[k] = bitmap_;
}

void
LteRlcAmPdu::setBitmapVec(std::vector<bool> bitmap_vec)
{
    this->bitmap_ = bitmap_vec;
}

std::vector<bool>
LteRlcAmPdu::getBitmapVec()
{
    return this->bitmap_;
}

bool
LteRlcAmPdu::isWhole()
{
    return (firstSn == lastSn);
}

bool
LteRlcAmPdu::isFirst()
{
    return (firstSn == snoFragment);
}

bool
LteRlcAmPdu::isMiddle()
{
    return ((!isFirst()) && (!isLast()));
}

bool
LteRlcAmPdu::isLast()
{
    return (lastSn == snoFragment);
}
