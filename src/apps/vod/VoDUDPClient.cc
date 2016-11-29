//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//
//

#include "apps/vod/VoDUDPClient.h"

using namespace std;

Define_Module(VoDUDPClient);

void VoDUDPClient::initialize(int stage)
{
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    /* Get parameters from INI file */
    EV << "VoD Client initialize: stage " << stage << endl;

    stringstream ss;
    ss << getId();
    string dir = "mkdir ./Framework/clients/client" + ss.str() + "/";
    string trace = "mkdir ./Framework/clients/client" + ss.str() + "/trace/";

    chdir("../../../VoDProject");
    system(dir.c_str());
    system(trace.c_str());

    string nsOutput = "./Framework/clients/client" + ss.str() + "/nsout.txt";
    outfile.open(nsOutput.c_str(), ios::out);

    if (outfile.bad()) /* File is bad */
        throw cRuntimeError("Error while opening output file (File not found or incorrect type)");

    totalRcvdBytes_ = 0;

    cMessage* timer = new cMessage("Timer");
    chdir("../workspace/lte/simulations/dynamicnetwork");
    scheduleAt(simTime(), timer);
    tptLayer0_ = registerSignal("VoDTptLayer0");
    tptLayer1_ = registerSignal("VoDTptLayer1");
    tptLayer2_ = registerSignal("VoDTptLayer2");
    tptLayer3_ = registerSignal("VoDTptLayer3");

    delayLayer0_ = registerSignal("VoDDelayLayer0");
    delayLayer1_ = registerSignal("VoDDelayLayer1");
    delayLayer2_ = registerSignal("VoDDelayLayer2");
    delayLayer3_ = registerSignal("VoDDelayLayer3");
}

void VoDUDPClient::finish()
{
    chdir("../../../VoDProject");
    outfile.close();
    string startMetrics = par("startMetrics").stringValue();

    /* Parameters to be sended to ana.sh */

    string inputFileName = par("vod_trace_file").stringValue();
    string bsePath = par("bsePath").stringValue();
    string origVideoYuv = par("origVideoYuv").stringValue();
    string origVideoSvc = par("origVideoSvc").stringValue();
    string decPath = par("decPath").stringValue();
    string avipluginPath = par("avipluginPath").stringValue();
    int playbackSize = par("playbackSize");
    string traceType = par("traceType");
    int numPktPerFrame = par("numPktPerFrame");
    int numFrame = par("numFrame");

    if ((!strcmp(startMetrics.c_str(), "on")))
    {
        stringstream ss, nf, pb, npktf;
        ss << getId();
        pb << playbackSize;
        npktf << numPktPerFrame;
        nf << numFrame;

        string createOutput = "./Framework/creatensoutput.py " + inputFileName + " " + ss.str();

        string anaPar = "./Framework/ana.sh " + inputFileName + " " + bsePath + " " + origVideoYuv
            + " " + origVideoSvc + " " + decPath + " " + pb.str()
            + " " + avipluginPath + " " + ss.str() + " " + nf.str();

        // Bin FPS NumPkt frame configurabili
        string plot = "./Framework/plot.py 25 ./Framework/clients/client" + ss.str() +
            "/output/nsoutput.txt-plos ./Framework/clients/client" + ss.str() +
            "/output/nsoutput.txt-psnr 25 " + nf.str() + " " + npktf.str() + " " + ss.str();

        fstream f;
        string apri = "client" + ss.str() + ".sh";
        f.open(apri.c_str(), ios::out);
        f << "#!/bin/bash" << endl;
        f << createOutput << " && " << anaPar << " && " << plot << endl;
        f.close();
        //system(createOutput.c_str());
        //system(anaPar.c_str());
        //system(plot.c_str());
    }
    else
    {
        if (traceType == "ns2")
        {
            stringstream ss, pb, np;
            ss << getId();
            pb << playbackSize;
            double startStreamTime = par("startStreamTime");
            int npkt = (int) startStreamTime;
            np << npkt;
            string par = "./Framework/createns2output.py " + inputFileName + " " + pb.str() + " " + ss.str() + " " + np.str();
            system(par.c_str());
        }
    }
    chdir("../workspace/lte/simulations/dynamicnetwork");
}

void VoDUDPClient::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage())
    {
        int localPort = par("localPort");
        socket.setOutputGate(gate("udpOut"));
        socket.bind(localPort);
        delete msg;
    }
    else if (!strcmp(msg->getName(), "VoDPacket"))
        receiveStream((VoDPacket*) (msg));
    else
        delete msg;
}

void VoDUDPClient::receiveStream(VoDPacket *msg)
{
    int seqNum = msg->getFrameSeqNum();
    simtime_t sendingTime = msg->getTimestamp();
    int frameLength = msg->getFrameLength();
    simtime_t delay = simTime() - sendingTime;
    int layer = msg->getQid();

    totalRcvdBytes_ += msg->getByteLength();
    double tputSample = (double)totalRcvdBytes_ / (simTime() - getSimulation()->getWarmupPeriod());
    if (layer == 0)
    {
        emit(tptLayer0_, tputSample);
        emit(delayLayer0_, delay.dbl());
    }
    else if (layer == 1)
    {
        emit(tptLayer1_, tputSample);
        emit(delayLayer1_, delay.dbl());
    }
    else if (layer == 2)
    {
        emit(tptLayer2_, tputSample);
        emit(delayLayer2_, delay.dbl());
    }
    else if (layer == 3)
    {
        emit(tptLayer3_, tputSample);
        emit(delayLayer3_, delay.dbl());
    }
    //    outfile << seqNum << "\t" << frameLength << "\t" << delay << endl;

    delete msg;
}
