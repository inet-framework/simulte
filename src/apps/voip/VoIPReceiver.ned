// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.apps.voip;

import inet.applications.contract.IUDPApp;


simple VoIPReceiver like IUDPApp
{
    parameters:
        int localPort = default(3000);
        int emodel_Ie_ = default(5);
        int emodel_Bpl_ = default(10);
        int emodel_A_ = default(5);
        double emodel_Ro_ = default(93.2);
        double playout_delay @unit("s") = default(0s);
        int dim_buffer = default(20);
        double sampling_time @unit("s") = default(0.02s);

		@signal[voIPFrameLoss];
        @statistic[voIPFrameLoss](title="VoIP Frame Loss"; unit="ratio"; source="voIPFrameLoss"; record=mean);
        @signal[voIPFrameDelay];
        @statistic[voIPFrameDelay](title="VoIP Frame Delay"; unit="s"; source="voIPFrameDelay"; record=mean,);
        @signal[voIPPlayoutDelay];
        @statistic[voIPPlayoutDelay](title="VoIP Playout Delay"; unit="s"; source="voIPPlayoutDelay"; record=mean,);
        @signal[voIPPlayoutLoss];
        @statistic[voIPPlayoutLoss](title="VoIP Playout Loss"; unit="ratio"; source="voIPPlayoutLoss"; record=mean);
        @signal[voIPJitter];
        @statistic[voIPJitter](title="VoIP Playout Loss"; unit="s"; source="voIPJitter"; record=mean);
        @signal[voIPMos];
        @statistic[voIPMos](title="VoIP Mos Signal"; unit="MOS"; source="voIPMos"; record=mean);
        @signal[voIPTaildropLoss];
        @statistic[voIPTaildropLoss](title="VoIP Tail Drop Loss"; unit="ratio"; source="voIPTaildropLoss"; record=mean);
        
        @signal[voipReceivedThroughtput];
        @statistic[voipReceivedThroughtput](title="Throughput received at the application level"; unit="Bps"; source="voipReceivedThroughtput"; record=timeavg,mean,vector,sum);
        @display("i=block/source");
    gates:
        output udpOut;
        input udpIn;
}

