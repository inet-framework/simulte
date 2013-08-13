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

#include "LteAirFrame.h"

void LteAirFrame::addRemoteUnitPhyDataVector(RemoteUnitPhyData data)
{
    remoteUnitPhyDataVector.push_back(data);
}
RemoteUnitPhyDataVector LteAirFrame::getRemoteUnitPhyDataVector()
{
    return remoteUnitPhyDataVector;
}
