//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//  @author Angelo Buono
//

package lte.apps.vehicular.v2v.v2vSpeedControl;
import lte.apps.vehicular.v2v.V2vClusterBaseApp ;

//
// TODO auto-generated module
//
simple V2vClusterSpeedControlApp extends V2vClusterBaseApp
{
	parameters:
    	@class("V2vClusterSpeedControlApp");
    	        
        //sender side signals       
        @signal[v2vClusterSpeedControlSentMsg];
        @statistic[v2vClusterSpeedControlSentMsg](title="V2vClusterSpeedControlSentMsg messages sent"; unit=""; source="v2vClusterSpeedControlSentMsg"; record=sum,vector);

        //receiver side signals
        @signal[v2vClusterSpeedControlDelay];
        @statistic[v2vClusterSpeedControlDelay](title="V2vClusterSpeedControlDelay Message Delay"; unit="s"; source="v2vClusterSpeedControlDelay"; record=mean,vector);
           
        @signal[v2vClusterSpeedControlRcvdMsg]; 
        @statistic[v2vClusterSpeedControlRcvdMsg](title="V2vClusterSpeedControlRcvdMsg Messages Received"; unit="s"; source="v2vClusterSpeedControlRcvdMsg"; record=sum,vector);
              
        @display("i=block/source");
}
