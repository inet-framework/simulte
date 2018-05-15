//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.

package lte.apps.d2dMultihop.statistics;

//
// MultihopD2DStatistics module
//
// This module is only used to gather statistics related to events generated
// by the Multihop D2D apps. It is accessed via method calls.
//
simple MultihopD2DStatistics {
  
  parameters:

    @signal[d2dMultihopEventDeliveryRatio];
    @statistic[d2dMultihopEventDeliveryRatio](title="Avg delivery ratio within the area"; unit=""; source="d2dMultihopEventDeliveryRatio"; record=mean,sum,min,max);   
    @signal[d2dMultihopEventDelay];
    @statistic[d2dMultihopEventDelay](title="Avg delay of broadcasting within the area"; unit=""; source="d2dMultihopEventDelay"; record=mean,sum,min,max);    
    @signal[d2dMultihopEventDelay95Per];
    @statistic[d2dMultihopEventDelay95Per](title="95th percentile of the delay of broadcasting within the area"; unit=""; source="d2dMultihopEventDelay95Per"; record=mean,sum,min,max);    
    @signal[d2dMultihopEventSentMsg];
    @statistic[d2dMultihopEventSentMsg](title="Avg transmissions for broadcasting a message"; unit=""; source="d2dMultihopEventSentMsg"; record=mean,sum,min,max);  
    @signal[d2dMultihopEventTrickleSuppressedMsg];
    @statistic[d2dMultihopEventTrickleSuppressedMsg](title="Avg suppressed transmissions for broadcasting a message"; unit=""; source="d2dMultihopEventTrickleSuppressedMsg"; record=mean,sum,min,max);  
    @signal[d2dMultihopEventRcvdDupMsg];
    @statistic[d2dMultihopEventRcvdDupMsg](title="Avg num of duplicates within the area"; unit=""; source="d2dMultihopEventRcvdDupMsg"; record=mean,sum,min,max);   
     
    @display("i=block/cogwheel");
}