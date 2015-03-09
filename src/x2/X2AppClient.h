//
//                           SimuLTE
// Copyright (C) 2015 Antonio Virdis, Giovanni Nardini, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __X2APPCLIENT_H_
#define __X2APPCLIENT_H_

#include "SCTPClient.h"
#include "LteCommon.h"

class SCTPAssociation;

/**
 * Implements the X2AppClient simple module. See the NED file for more info.
 */
class X2AppClient : public SCTPClient
{
    // reference to the gates
    cGate* x2ManagerOut_;

  protected:

    void initialize(int stage);
    virtual int numInitStages() const { return 5; }
    void socketEstablished(int32 connId, void *yourPtr, uint64 buffer);
    void socketDataArrived(int32 connId, void *yourPtr, cPacket *msg, bool urgent);
};

#endif

