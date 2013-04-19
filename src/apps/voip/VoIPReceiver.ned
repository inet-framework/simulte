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

package lte.apps.voip;

import inet.applications.IUDPApp;


simple VoIPReceiver like IUDPApp
{
    parameters:
        int localPort = default(3000);
        int emodel_Ie_ = default(5);
        int emodel_Bpl_ = default(10);
        int emodel_A_ = default(5);
        double emodel_Ro_ = default(93.2);
        double playout_delay = default(0);
        int dim_buffer = default(20);
        double sampling_time = default(0.02);

        @statistic[VoIPFrameLoss](title="VoIP Frame Loss"; unit="ratio"; source="VoIPFrameLoss"; record=lteAvg);
        @statistic[VoIPFrameDelay](title="VoIP Frame Delay"; unit="s"; source="VoIPFrameDelay"; record=lteAvg);
        @statistic[VoIPPlayoutDelay](title="VoIP Playout Delay"; unit="s"; source="VoIPPlayoutDelay"; record=lteAvg);
        @statistic[VoIPPlayoutLoss](title="VoIP Playout Loss"; unit="ratio"; source="VoIPPlayoutLoss"; record=lteAvg);
        @statistic[VoIPMosSignal](title="VoIP Mos Signal"; unit="MOS"; source="VoIPMosSignal"; record=lteAvg);
        @statistic[VoIPTaildropLoss](title="VoIP Tail Drop Loss"; unit="ratio"; source="VoIPTaildropLoss"; record=lteAvg);

        @display("i=block/source");
    gates:
        output udpOut;
        input udpIn;
}

