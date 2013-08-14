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

#ifndef _LTE_AMCPILOTAUTO_H_
#define _LTE_AMCPILOTAUTO_H_

#include "AmcPilot.h"

/**
 * @class AmcPilotAuto
 * @brief AMC auto pilot
 *
 * Provides a simple online assigment of transmission mode.
 * The best per-UE effective rate available is selected to assign logical bands.
 * This mode is intended for use with localization unaware schedulers (legacy).
 *
 * This AMC AUTO mode always selects single antenna port (port 0) as tx mode,
 * so there is only 1 codeword, mapped on 1 layer (SISO).
 * Users are sorted by CQI and added to user list for all LBs with
 * the respective RI, CQI and PMI.
 */
class AmcPilotAuto : public AmcPilot
{
  public:

    /**
     * Constructor
     * @param amc LteAmc owner module
     */
    AmcPilotAuto(LteAmc *amc) :
        AmcPilot(amc)
    {
        name_ = "Auto";
    }
    /**
     * Assign logical bands for given nodeId and direction
     * @param id The mobile node ID.
     * @param dir The link direction.
     * @return The user transmission parameters computed.
     */
    const UserTxParams& computeTxParams(MacNodeId id, const Direction dir);
    //Used with TMS pilot
    void updateActiveUsers(ActiveSet aUser, Direction dir)
    {
        return;
    }
};

#endif
