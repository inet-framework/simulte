//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef LTE_D2DMODESELECTIONBESTCQI_H_
#define LTE_D2DMODESELECTIONBESTCQI_H_

#include "stack/d2dModeSelection/D2DModeSelectionBase.h"

//
// D2DModeSelectionBestCqi
//
// For each D2D-capable flow, select the mode having the best CQI
//
class D2DModeSelectionBestCqi : public D2DModeSelectionBase
{

protected:

    // run the mode selection algorithm
    virtual void doModeSelection();

public:
    D2DModeSelectionBestCqi() {}
    virtual ~D2DModeSelectionBestCqi() {}

    virtual void initialize(int stage);
    virtual void doModeSwitchAtHandover(MacNodeId nodeId, bool handoverCompleted);
};

#endif /* LTE_D2DMODESELECTIONBESTCQI_H_ */
