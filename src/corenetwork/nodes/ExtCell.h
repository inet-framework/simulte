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


#ifndef EXTCELL_H_
#define EXTCELL_H_

#include "Coord.h"


class ExtCell {

    Coord position_;
    double txPower_;
    //Pattern pattern_; // enum { HEAVY ; LOW ; MED ; UNIFORM }

public:
    /**
     * Creates and external cell
     * @param pwr transmission power of the eNb within the extern cell
     * @param i external cell index
     * @param num number of external cells (used for cell deployment over the playground
     * @param distance from the "main" eNb
     */
    ExtCell(double pwr , int i , int num , int distance);
    virtual ~ExtCell();

    Coord getPosition() { return position_; }
    double getTxPower() { return txPower_;     }

    /**
     * evaluates traffic pattern
     * @return true if a transmission is in progress
     */
    bool transmitting();
};

typedef std::vector<ExtCell*> ExtCellList;

#endif /* EXTCELL_H_ */
