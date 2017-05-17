//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "apps/vod/VoDUDPServer.h"

Define_Module(VoDUDPServer);

VoDUDPServer::VoDUDPServer()
{
}
VoDUDPServer::~VoDUDPServer()
{
}

void VoDUDPServer::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage != INITSTAGE_APPLICATION_LAYER)
        return;
    EV << "VoD Server initialize: stage " << stage << endl;
    serverPort = par("localPort");
    inputFileName = par("vod_trace_file").stringValue();
    traceType = par("traceType").stringValue();
    fps = par("fps");
    double one = 1.0;
    TIME_SLOT = one / fps;
    numStreams = 0;

    // set up UDP socket
    socket.setOutputGate(gate("udpOut"));
    socket.bind(serverPort);

    if (!inputFileName.empty())
    {
        // Check whether string is empty
        if (traceType == "SVC")
        {
            infile.open(inputFileName.c_str(), ios::in);
            if (infile.bad()) /* Or file is bad */
                throw cRuntimeError("Error while opening input file (File not found or incorrect type)");

            infile.seekg(0, ios::beg);
            long int i = 0;
            while (!infile.eof())
            {
                svcPacket tmp;
                tmp.index = i;
                infile >> tmp.memoryAdd >> tmp.length >> tmp.lid >> tmp.tid >> tmp.qid >>
                    tmp.frameType >> tmp.isDiscardable >> tmp.isTruncatable >> tmp.frameNumber
                    >> tmp.timestamp >> tmp.isControl;
                svcTrace_.push_back(tmp);
                i++;
            }
            svcPacket tmp;
            tmp.index = LONG_MAX;
            svcTrace_.push_back(tmp);
        }
        else
        {
            tracerec t;
            struct stat buf;
            FILE *fp;

            /* only open/read the file once (could be shared by multiple
             * SourceModel's
             */

            if (stat(inputFileName.c_str(), (struct stat *) &buf))
                throw cRuntimeError("Error while opening input file (File not found or incorrect type)");

            nrec_ = buf.st_size / sizeof(tracerec);
            unsigned nrecplus = nrec_ * sizeof(tracerec);
            unsigned bufst = buf.st_size;
            //      if ((unsigned)(nrec_ * sizeof(tracerec)) != buf.st_size) {
            if (nrecplus != bufst)
                throw cRuntimeError("bad file size in %s", inputFileName.c_str());

            trace_ = new tracerec[nrec_];

            if ((fp = fopen(inputFileName.c_str(), "rb")) == NULL)
                throw cRuntimeError("can't open file %s", inputFileName.c_str());

            for (unsigned int i = 0; i < nrec_; i++)
            {
                if (fread((char *) &t, sizeof(tracerec), 1, fp) != 1)
                    throw cRuntimeError("read failed");
                trace_[i].trec_time = ntohl(t.trec_time);
                trace_[i].trec_size = ntohl(t.trec_size);
            }
            fclose(fp);
        }
    }

    /* Initialize parameters after the initialize() method */
    EV << "VoD Server initialize: Trace: " << inputFileName << " trace type " << traceType << endl;
    cMessage* timer = new cMessage("Timer");
    double start = par("startTime");
    double offset = (double) start + simTime().dbl();
    scheduleAt(offset, timer);
}

void VoDUDPServer::finish()
{
    if (infile.is_open())
        infile.close();
}

void VoDUDPServer::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "Timer"))
        {
            clientsPort = par("destPort");
            //     vclientsPort = cStringTokenizer(clientsPort).asIntVector();

            clientsStartStreamTime = par("clientsStartStreamTime").doubleValue();
            //vclientsStartStreamTime = cStringTokenizer(clientsStartStreamTime).asDoubleVector();

            clientsReqTime = par("clientsReqTime");
            vclientsReqTime = cStringTokenizer(clientsReqTime).asDoubleVector();

            int size = 0;

            const char *destAddrs = par("destAddresses");
            cStringTokenizer tokenizer(destAddrs);
            const char *token;
            while ((token = tokenizer.nextToken()) != NULL)
            {
                clientAddr.push_back(L3AddressResolver().resolve(token));
                size++;
            }

            /* Register video streams*/

            for (int i = 0; i < size; i++)
            {
                M1Message* M1 = new M1Message();
                M1->setClientAddr(clientAddr[i]);
                M1->setClientPort(clientsPort);
                double npkt;
                npkt = clientsStartStreamTime;
                M1->setNumPkSent((int) (npkt * fps));

                numStreams++;
                EV << "VoD Server self message: Dest IP: " << clientAddr[i] << " port: " << clientsPort << " start stream: " << (int)(npkt* fps) << endl;
//                    scheduleAt(simTime() + vclientsReqTime[i], M1);
                scheduleAt(simTime(), M1);
            }

            delete msg;
            return;
        }
        else if (traceType == "SVC")
            handleSVCMessage(msg);
        else
            handleNS2Message(msg);
    }
    else
        delete msg;
}

void VoDUDPServer::handleNS2Message(cMessage *msg)
{
    M1Message* msgNew = check_and_cast<M1Message*>(msg);
    long numPkSentApp = msgNew->getNumPkSent();

    /* File is not finished yet */
    int length;//, interTime;

    int seq_num = numPkSentApp;
    //interTime = trace_[numPkSentApp % nrec_].trec_time;
    length = trace_[numPkSentApp % nrec_].trec_size;

    VoDPacket* frame = new VoDPacket("VoDPacket");
    frame->setFrameSeqNum(seq_num);
    frame->setTimestamp(simTime());
    frame->setByteLength(length);
    frame->setFrameLength(length); /* Seq_num plus frame length plus payload */
    frame->setTid(0);
    frame->setQid(0);
    socket.sendTo(frame, msgNew->getClientAddr(), msgNew->getClientPort());

    numPkSentApp++;
    msgNew->setNumPkSent(numPkSentApp);

    EV << "VoD Server Sent New Packet: Dest IP: " << msgNew -> getClientAddr() << " port: " << msgNew -> getClientPort() << endl;
    scheduleAt(simTime() + TIME_SLOT, msgNew);
}

void VoDUDPServer::handleSVCMessage(cMessage *msg)
{
    M1Message* msgNew = (M1Message*) msg;
    long numPkSentApp = msgNew->getNumPkSent();
    if (svcTrace_[numPkSentApp].index == LONG_MAX)
    {
        /* End of file, send finish packet */
        cPacket* fm = new cPacket("VoDFinishPacket");
        socket.sendTo(fm, msgNew->getClientAddr(), msgNew->getClientPort());
        return;
    }
    else
    {
        int seq_num = numPkSentApp;
        int currentFrame = svcTrace_[numPkSentApp].frameNumber;

        VoDPacket* frame = new VoDPacket("VoDPacket");
        frame->setFrameSeqNum(seq_num);
        frame->setTimestamp(simTime());
        frame->setByteLength(svcTrace_[numPkSentApp].length);
        frame->setTid(svcTrace_[numPkSentApp].tid);
        frame->setQid(svcTrace_[numPkSentApp].qid);
        frame->setFrameLength(svcTrace_[numPkSentApp].length + 2 * sizeof(int)); /* Seq_num plus frame length plus payload */
        socket.sendTo(frame, msgNew->getClientAddr(), msgNew->getClientPort());
        numPkSentApp++;
        while (1)
        {
            /* Get infos about the frame from file */

            if (svcTrace_[numPkSentApp].index == LONG_MAX)
                break;

            int seq_num = numPkSentApp;
            if (svcTrace_[numPkSentApp].frameNumber != currentFrame)
                break; // Finish sending packets belonging to the current frame

            VoDPacket* frame = new VoDPacket("VoDPacket");
            frame->setTid(svcTrace_[numPkSentApp].tid);
            frame->setQid(svcTrace_[numPkSentApp].qid);
            frame->setFrameSeqNum(seq_num);
            frame->setTimestamp(simTime());
            frame->setByteLength(svcTrace_[numPkSentApp].length);
            frame->setFrameLength(svcTrace_[numPkSentApp].length + 2 * sizeof(int)); /* Seq_num plus frame length plus payload */
            socket.sendTo(frame, msgNew->getClientAddr(), msgNew->getClientPort());
            EV << " VoDUDPServer::handleSVCMessage sending frame " << seq_num << std::endl;
            numPkSentApp++;
        }
        msgNew->setNumPkSent(numPkSentApp);
        scheduleAt(simTime() + TIME_SLOT, msgNew);
    }
}

