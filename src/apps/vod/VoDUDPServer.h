//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_VODUDPSRV_H_
#define _LTE_VODUDPSRV_H_

#include <fstream>

#include <omnetpp.h>

#include <inet/transportlayer/contract/udp/UdpControlInfo.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include <platdep/sockets.h>
#include "common/LteCommon.h"
#include "apps/vod/VoDUDPStruct.h"
#include "apps/vod/VoDPacket_m.h"
#include "apps/vod/M1Message_m.h"

class SIMULTE_API VoDUDPServer : public omnetpp::cSimpleModule
{
  protected:
      inet::UdpSocket socket;
    /* Server parameters */

    int serverPort;
    std::ifstream infile;
    std::string inputFileName;
    int fps;
    std::string traceType;
    std::fstream outfile;
    double TIME_SLOT;

    const char * clientsIP;
    int clientsPort;
    double clientsStartStreamTime;
    const char * clientsReqTime;

    std::vector<std::string> vclientsIP;

    std::vector<int> vclientsPort;
    std::vector<double> vclientsStartStreamTime;
    std::vector<double> vclientsReqTime;
    std::vector<inet::L3Address> clientAddr;

    /* Statistics */

    unsigned int numStreams;  // number of video streams served
    unsigned long numPkSent;  // total number of packets sent

    struct tracerec
    {
        uint32_t trec_time;
        uint32_t trec_size;
    };
    struct svcPacket
    {
        int tid;
        int lid;
        int qid;
        int length;
        int frameNumber;
        int timestamp;
        int currentFrame;
        std::string memoryAdd;
        std::string isDiscardable;
        std::string isTruncatable;
        std::string isControl;
        std::string frameType;
        long int index;
    };
    unsigned int nrec_;

    tracerec* trace_;

    std::vector<svcPacket> svcTrace_;

  public:
    VoDUDPServer();
    virtual ~VoDUDPServer();

  protected:

    void initialize(int stage);
    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void finish();
    virtual void handleMessage(omnetpp::cMessage*);
    virtual void handleNS2Message(omnetpp::cMessage*);
    virtual void handleSVCMessage(omnetpp::cMessage*);
};

#endif
