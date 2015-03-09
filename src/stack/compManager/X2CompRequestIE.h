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

#ifndef _LTE_X2COMPREQUESTIE_H_
#define _LTE_X2COMPREQUESTIE_H_

#include "X2InformationElement.h"

//
// X2CompRequestIE
//
class X2CompRequestIE : public X2InformationElement
{
  protected:

    // number of "ideally" required blocks to satisfy the queue of all UEs
    unsigned int numBlocks_;

  public:
    X2CompRequestIE()
    {
        type_ = COMP_REQUEST_IE;
        length_ = sizeof(unsigned int);
    }
    X2CompRequestIE(const X2CompRequestIE& other) :
        X2InformationElement()
    {
        operator=(other);
    }

    X2CompRequestIE& operator=(const X2CompRequestIE& other)
    {
        if (&other == this)
            return *this;
        X2InformationElement::operator=(other);
        return *this;
    }
    virtual X2CompRequestIE *dup() const
    {
        return new X2CompRequestIE(*this);
    }
    virtual ~X2CompRequestIE() {}

    // getter/setter methods
    void setNumBlocks(unsigned int numBlocks) { numBlocks_ = numBlocks; }
    unsigned int getNumBlocks() { return numBlocks_; }

};

#endif
