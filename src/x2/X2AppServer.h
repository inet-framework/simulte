//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __X2APPSERVER_H_
#define __X2APPSERVER_H_

#include "inet/applications/sctpapp/SCTPServer.h"
#include "common/LteCommon.h"
#include "corenetwork/binder/LteBinder.h"


/**
 * Implements the X2AppServer simple module. See the NED file for more info.
 */
class X2AppServer : public SCTPServer
{
        // reference to the gate
        cGate* x2ManagerIn_;

    protected:
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        void handleTimer(cMessage *msg);
        void generateAndSend(cPacket* pkt);
};

#endif


