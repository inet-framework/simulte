

package lte.apps.cbr;

import inet.applications.contract.IUDPApp;


simple CbrReceiver like IUDPApp
{
    parameters:
        int localPort = default(3000);

        @signal[cbrFrameDelaySignal](type="simtime_t");
        @signal[cbrJitterSignal](type="simtime_t");
        @signal[cbrFrameLossSignal](type="double");
        @signal[cbrReceivedThroughtputSignal];
        
        
        @statistic[cbrFrameDelay](title="Cbr Frame Delay"; unit="s"; source="cbrFrameDelaySignal"; record=mean,vector);
        @statistic[cbrJitter](title="Cbr Playout Loss"; unit="s"; source="cbrJitterSignal"; record=mean,vector);
        @statistic[cbrFrameLoss](title="Cbr Frame Loss"; unit="ratio"; source="cbrFrameLossSignal"; record=mean,vector);
        
        @statistic[cbrReceivedThroughtput](title="Throughput received at the application level"; unit="Bps"; source="cbrReceivedThroughtputSignal"; record=timeavg,mean,vector,sum);
        @display("i=block/source");
    gates:
        output udpOut;
        input udpIn;
}

