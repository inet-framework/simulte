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

#ifndef _LTE_VOIPSENDER_H_
#define _LTE_VOIPSENDER_H_

#include <string.h>
#include <omnetpp.h>

#include "UDPSocket.h"
#include "IPvXAddressResolver.h"
#include "VoipPacket_m.h"

class VoIPSender : public cSimpleModule
{
    UDPSocket socket;
    //has the sender been initialized?
    bool initialized_;

    //source
    simtime_t durTalk_;
    simtime_t durSil_;
    double scaleTalk_;
    double shapeTalk_;
    double scaleSil_;
    double shapeSil_;
    bool isTalk_;
    cMessage* selfSource_;
    //sender
    int iDtalk_;
    int nframes_;
    int iDframe_;
    int nframesTmp_;
    int size_;
    simtime_t sampling_time;

    bool silences_;


    simsignal_t voIPGeneratedThroughtput_;
    // ----------------------------

    cMessage *selfSender_;

    simtime_t timestamp_;
    int localPort_;
    int destPort_;
    IPvXAddress destAddress_;

    void talkspurt(simtime_t dur);
    void selectPeriodTime();
    void sendVoIPPacket();

  public:
    ~VoIPSender();
    VoIPSender();

  protected:

    virtual int numInitStages() const { return 4; }
    void initialize(int stage);
    void handleMessage(cMessage *msg);
};

#endif

