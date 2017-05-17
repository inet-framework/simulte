//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTERLCAMPDU_H_
#define _LTE_LTERLCAMPDU_H_

#include "stack/rlc/am/packet/LteRlcAmPdu_m.h"

class LteRlcAmPdu : public LteRlcAmPdu_Base
{
    std::vector<bool> bitmap_;

  public:

    LteRlcAmPdu(const char *name = NULL) :
        LteRlcAmPdu_Base(name)
    {
    }
    LteRlcAmPdu(const LteRlcAmPdu& other) :
        LteRlcAmPdu_Base(other)
    {
        operator=(other);
    }

    LteRlcAmPdu& operator=(const LteRlcAmPdu& other)
    {
        LteRlcAmPdu_Base::operator=(other);
	bitmap_ = other.bitmap_;
        return *this;
    }

    virtual LteRlcAmPdu* dup()
    {
        return new LteRlcAmPdu(*this);
    }

    virtual void setBitmapArraySize(unsigned int size);
    virtual unsigned int getBitmapArraySize() const;
    virtual bool getBitmap(unsigned int k) const;
    virtual void setBitmap(unsigned int k, bool bitmap_var);
    virtual void setBitmapVec(std::vector<bool> bitmap_vec);
    virtual std::vector<bool> getBitmapVec();
    //sequence check functions
    virtual bool isWhole();
    virtual bool isFirst();
    virtual bool isMiddle();
    virtual bool isLast();
};

Register_Class(LteRlcAmPdu);

#endif

