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

#ifndef _LTE_LTERLCDEFS_H_
#define _LTE_LTERLCDEFS_H_

#include "LteControlInfo.h"
#include "LteRlcPdu_m.h"
#include "LteRlcSdu_m.h"
#include "UmFragbuf.h"

/*!
 * LTE RLC AM Types
 */
enum LteAmType
{
    //! Data packet
    DATA = 0,
    //****** control packets ********
    //! ACK
    ACK = 1,
    //! Move Receiver Window
    MRW = 2,
    //! Move Receiver Window ACK
    MRW_ACK = 3,
//! BITMAP
//BITMAP = 4
};

/*
 * RLC AM Window descriptor
 */

struct RlcFragDesc
{
    /*!
     * Main SDU size (bytes) - the size of  the SDU to be fragmented
     */
    int sduSize_;

    /*!
     * Fragmentation Unit (bytes) - the size of fragments the SDU will be divided into
     */
    int fragUnit_;

    /*!
     * the number of fragments the SDU will be partitioned into
     *
     */
    int totalFragments_;

    /*!
     * the fragments of current SDU already added to transmission window
     */
    int fragCounter_;

    /*!
     * the first fragment SN of current SDU
     */
    int firstSn_;

    RlcFragDesc()
    {
        fragUnit_ = 0;
        resetFragmentation();
    }
    /*
     * Configures the fragmentation descriptor for working on SDU of size sduSize
     */

    void startFragmentation(unsigned int sduSize, unsigned int firstFragment)
    {
        totalFragments_ = ceil((double) sduSize / (double) fragUnit_);
        fragCounter_ = 0;
        firstSn_ = firstFragment;
    }

    /*
     * resets the fragmentation descriptor for working on SDU of size sduSize - frag unit is left untouched.
     */

    void resetFragmentation()
    {
        sduSize_ = 0;
        totalFragments_ = 0;
        fragCounter_ = 0;
        firstSn_ = 0;
    }

    /*
     * adds a fragment to created ones. if last is added, returns true
     */

    bool addFragment()
    {
        fragCounter_++;
        if (fragCounter_ >= totalFragments_)
            return true;
        else
            return false;
    }
};

struct RlcWindowDesc
{
  public:
    //! Sequence number of the first PDU in the TxWindow
    unsigned int firstSeqNum_;
    //! Sequence number of current PDU in the TxWindow
    unsigned int seqNum_;
    //! Size of the transmission window
    unsigned int windowSize_;

    RlcWindowDesc()
    {
        seqNum_ = 0;
        firstSeqNum_ = 0;
        windowSize_ = 0;
    }
};

/*!
 * Move Receiver Window command descriptor
 */
struct MrwDesc
{
    //! MRW current Sequence Number
    unsigned int mrwSeqNum_;
    //! Last MRW Sequence Number
    unsigned int lastMrw_;

    MrwDesc()
    {
        mrwSeqNum_ = 0;
        lastMrw_ = 0;
    }
};

enum RlcAmTimerType
{
    PDU_T = 0, MRW_T = 1, BUFFER_T = 2, BUFFERSTATUS_T = 3
};

#endif
