

package lte.apps.burst;

import inet.applications.contract.IUDPApp;


simple BurstReceiver like IUDPApp
{
    parameters:
        int localPort = default(3000);

        @signal[burstRcvdPkt];
        @statistic[burstRcvdPkt](title="Received packet ID"; unit=""; source="burstRcvdPkt"; record=vector);
        @signal[burstPktDelay];
        @statistic[burstPktDelay](title="Delay of received packets"; unit="s"; source="burstPktDelay"; record=mean,vector);
        
        @display("i=block/source");
    gates:
        output udpOut;
        input udpIn;
}

