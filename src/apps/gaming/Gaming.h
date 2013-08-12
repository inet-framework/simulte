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


#ifndef GAMING_H_
#define GAMING_H_
#include <string.h>
#include <omnetpp.h>

#include "UDPBasicApp.h"

class Gaming : public UDPBasicApp {
public:
    Gaming();
    virtual ~Gaming();
    virtual cPacket *createPacket();
};

#endif /* GAMING_H_ */
