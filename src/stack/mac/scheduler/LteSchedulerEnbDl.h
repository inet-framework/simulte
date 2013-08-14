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

#ifndef _LTE_LTE_SCHEDULER_ENB_DL_H_
#define _LTE_LTE_SCHEDULER_ENB_DL_H_

#include "LteSchedulerEnb.h"
#include "LteCommon.h"
#include "LteAmc.h"
#include "UserTxParams.h"

/**
 * @class LteSchedulerEnbDl
 *
 * LTE eNB downlink scheduler.
 */
class LteSchedulerEnbDl : public LteSchedulerEnb
{
    // XXX debug: to call grant from mac
    friend class LteMacEnb;

    /**
     * TODO
     * Implementare funzioni ancora non implementate : {}
     */

  protected:

    //---------------------------------------------

    /**
     * Checks Harq Descriptors and return the first free codeword.
     *
     * @param id
     * @param cw
     * @return
     */
    bool checkEligibility(MacNodeId id, Codeword& cw);

  public:

    /**
     * Default Constructor.
     */
    LteSchedulerEnbDl()
    {
    }
  protected:
    /**
     * Updates current schedule list with HARQ retransmissions.
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxschedule();

    /**
     * Schedules retransmission for the Harq Process of the given UE on a set of logical bands.
     * Each band has also assigned a band limit amount of bytes: no more than the specified
     * amount will be served on the given band for the acid.
     *
     * @param nodeId The node ID
     * @param cw The codeword used to serve the acid process
     * @param bands A vector of logical bands
     * @param acid The ACID
     * @return The allocated bytes. 0 if retransmission was not possible
     */
    virtual unsigned int schedulePerAcidRtx(MacNodeId nodeId, Codeword cw, unsigned char acid,
        std::vector<BandLimit>* bandLim = NULL, Remote antenna = MACRO, bool limitBl = false);
};

#endif // _LTE_LTE_SCHEDULER_ENB_DL_H_
