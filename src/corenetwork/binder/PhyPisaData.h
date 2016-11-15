//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_PHYPISADATA_H_
#define _LTE_PHYPISADATA_H_

#include <string.h>
#include <vector>

using namespace omnetpp;

class PhyPisaData
{
    double lambdaTable_[10000][3];
    double blerCurves_[3][15][49];
    std::vector<double> channel_;
    public:
    PhyPisaData();
    virtual ~PhyPisaData();
    double getBler(int i, int j, int k){if (j==0) return 1; else return blerCurves_[i][j][k-1];}
    double getLambda(int i, int j){return lambdaTable_[i][j];}
    int nTxMode(){return 3;}
    int nMcs(){return 15;}
    int maxSnr(){return 49;}
    int maxChannel(){return 10000;}
    int maxChannel2(){return 1000;}
    double getChannel(unsigned int i);
};

#endif
