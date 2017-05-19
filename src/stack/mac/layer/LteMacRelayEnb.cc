//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/layer/LteMacRelayEnb.h"

Define_Module(LteMacRelayEnb);

LteMacRelayEnb::LteMacRelayEnb()
{
    // TODO Auto-generated constructor stub
    nodeType_ = RELAY;
}
LteMacRelayEnb::~LteMacRelayEnb()
{
    // TODO Auto-generated destructor stub
}

LteDeployer* LteMacRelayEnb::getDeployer()
{
    MacNodeId masterId = getAncestorPar("masterId");
    OmnetId masterOmnetId = binder_->getOmnetId(masterId);
    return check_and_cast<LteDeployer *>(getSimulation()->getModule(masterOmnetId)->getSubmodule("deployer"));
}

int LteMacRelayEnb::getNumAntennas()
{
    return 1;
}
