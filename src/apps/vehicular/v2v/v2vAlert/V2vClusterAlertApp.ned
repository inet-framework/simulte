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

package lte.apps.vehicular.v2v.v2vAlert;
import lte.apps.vehicular.v2v.V2vClusterBaseApp ;

//
// TODO auto-generated module
//
simple V2vClusterAlertApp extends V2vClusterBaseApp
{
	parameters:
	    @class("V2vClusterAlertApp");
	    
	    //sender side signals       
        @signal[v2vClusterAlertSentMsg];
        @statistic[v2vClusterAlertSentMsg](title="V2vClusterAlertSentMsg messages sent"; unit=""; source="v2vClusterAlertSentMsg"; record=sum,vector);

        //receiver side signals
        @signal[v2vClusterAlertDelay];
        @statistic[v2vClusterAlertDelay](title="V2vClusterAlertDelay Message Delay"; unit="s"; source="v2vClusterAlertDelay"; record=mean,vector);
           
        @signal[v2vClusterAlertRcvdMsg]; 
        @statistic[v2vClusterAlertRcvdMsg](title="V2vClusterAlertRcvdMsg Messages Received"; unit="s"; source="v2vClusterAlertRcvdMsg"; record=sum,vector);
}