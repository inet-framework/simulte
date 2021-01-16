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

#include <inet/applications/sctpapp/SctpClient.h>
#include "common/LteCommon.h"

class SctpAssociation;

/**
 * Implements the X2AppClient simple module. See the NED file for more info.
 */
class SIMULTE_API X2AppClient : public inet::SctpClient
{
    // reference to the gates
    omnetpp::cGate* x2ManagerOut_;

  protected:

    void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void socketEstablished(inet::SctpSocket *socket, unsigned long int buffer) override;
    void socketDataArrived(inet::SctpSocket *socket, inet::Packet *msg, bool urgent) override;
};

#endif

