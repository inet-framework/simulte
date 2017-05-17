//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEPDCPENTITY_H_
#define _LTE_LTEPDCPENTITY_H_

#include "common/LteCommon.h"
#include "common/LteControlInfo.h"

/**
 * @class LtePdcpEntity
 * @brief Entity for PDCP Layer
 *
 * This is the PDCP entity of LTE Stack.
 *
 * PDCP entity performs the following tasks:
 * - mantain numbering of one logical connection
 *
 */
class LtePdcpEntity
{
    // next sequence number to be assigned
    unsigned int sequenceNumber_;

  public:

    LtePdcpEntity();
    virtual ~LtePdcpEntity();

    // returns the current sno and increment the next sno
    unsigned int nextSequenceNumber();

    // set the value of the next sequence number
    void setNextSequenceNumber(unsigned int nextSno) { sequenceNumber_ = nextSno; }

    // reset numbering
    void resetSequenceNumber() { sequenceNumber_ = 0; }
};

#endif
