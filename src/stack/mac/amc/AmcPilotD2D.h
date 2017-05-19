//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_AMCPILOTD2D_H_
#define _LTE_AMCPILOTD2D_H_

#include "stack/mac/amc/AmcPilot.h"

/**
 * @class AmcPilotD2D
 * @brief AMC pilot for D2D communication
 */
class AmcPilotD2D : public AmcPilot
{
    bool usePreconfiguredTxParams_;
    UserTxParams* preconfiguredTxParams_;

  public:

    /**
     * Constructor
     * @param amc LteAmc owner module
     */
    AmcPilotD2D(LteAmc *amc) :
        AmcPilot(amc)
    {
        name_ = "D2D";
        mode_ = MIN_CQI;
        usePreconfiguredTxParams_ = false;
        preconfiguredTxParams_ = NULL;
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

    void setPreconfiguredTxParams(Cqi cqi);

    // TODO reimplement these functions
    virtual std::vector<Cqi>  getMultiBandCqi(MacNodeId id, const Direction dir){}
    virtual void setUsableBands(MacNodeId id , UsableBands usableBands){}
    virtual UsableBands* getUsableBands(MacNodeId id){}
};

#endif
