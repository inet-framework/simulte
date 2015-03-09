//
//                           SimuLTE
// Copyright (C) 2015 Antonio Virdis, Giovanni Nardini, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_X2COMPREPLYIE_H_
#define _LTE_X2COMPREPLYIE_H_

#include "X2InformationElement.h"

enum CompRbStatus
{
    AVAILABLE_RB, NOT_AVAILABLE_RB
};

//
// X2CompReplyIE
//
class X2CompReplyIE : public X2InformationElement
{
  protected:

    // for each block, it denotes whether the eNB can use that block
    std::vector<CompRbStatus> allowedBlocksMap_;

  public:
    X2CompReplyIE()
    {
        type_ = COMP_REPLY_IE;
        length_ = 0;
    }
    X2CompReplyIE(const X2CompReplyIE& other) :
        X2InformationElement()
    {
        operator=(other);
    }

    X2CompReplyIE& operator=(const X2CompReplyIE& other)
    {
        if (&other == this)
            return *this;
        X2InformationElement::operator=(other);
        return *this;
    }
    virtual X2CompReplyIE *dup() const
    {
        return new X2CompReplyIE(*this);
    }
    virtual ~X2CompReplyIE() {}

    // getter/setter methods
    void setAllowedBlocksMap(std::vector<CompRbStatus>& map)
    {
        allowedBlocksMap_ = map;
        length_ += allowedBlocksMap_.size() * sizeof(CompRbStatus);
    }
    std::vector<CompRbStatus>& getAllowedBlocksMap() { return allowedBlocksMap_; }
};

#endif
