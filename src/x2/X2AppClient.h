//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __X2APPCLIENT_H_
#define __X2APPCLIENT_H_

#include "inet/applications/sctpapp/SCTPClient.h"
#include "common/LteCommon.h"

class SCTPAssociation;

/**
 * Implements the X2AppClient simple module. See the NED file for more info.
 */
class X2AppClient : public inet::SCTPClient
{
    // reference to the gates
    cGate* x2ManagerOut_;

  protected:

    void initialize(int stage);
    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void socketEstablished(int32_t connId, void *yourPtr, unsigned long int buffer);
    void socketDataArrived(int32_t connId, void *yourPtr, cPacket *msg, bool urgent);
};

#endif

