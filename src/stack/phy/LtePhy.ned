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


package lte.stack.phy;

// 
// Interface for the LtePhy module of PHY Layer
//
moduleinterface LtePhy {
    parameters:
        @display("i=block/wrxtx");
        bool usePropagationDelay;
        xml channelModel;
        
    gates:
        input upperGateIn;
        output upperGateOut;
        input radioIn;
}

// 
// Base LtePhy module of PHY Layer
//
simple LtePhyBase like LtePhy {
    parameters:
        @display("i=block/wrxtx");
        bool usePropagationDelay = default(true);                     // Should transmission delay be simulated?
        xml channelModel;
        double ueTxPower =default(26);
        double eNodeBTxPower =default(46);
        double relayTxPower =default(20);
        double microTxPower =default(30);
        string txDirection = default("OMNI");
        double txAngle = default(0);       
             
        //# CQI statistics
        @statistic[averageCqiDl](title="Average Cqi reported in DL"; unit="cqi"; source="averageCqiDl"; record=lteAvg);
        @statistic[averageCqiDlvect](title="Average Cqi reported in DL"; unit="cqi"; source="averageCqiDlvect"; record=vector);
        @statistic[averageCqiUl](title="Average Cqi reported in UL"; unit="cqi"; source="averageCqiUl"; record=lteAvg);
        @statistic[averageCqiUlvect](title="Average Cqi reported in UL"; unit="cqi"; source="averageCqiUlvect"; record=vector);
    
        
    gates:
        input upperGateIn;       // from upper layer
        output upperGateOut;    // to upper layer
        input radioIn;           // gate to receive message via sendDirect
}

// 
// User Equipment LtePhy module of PHY Layer
//
simple LtePhyUe extends LtePhyBase {
     parameters:
         @class("LtePhyUe");
         // switch for handover messages handling on UEs
         bool enableHandover = default(false);
}

// 
// eNodeB LtePhy module of PHY Layer
//
simple LtePhyEnb extends LtePhyBase {
    @class("LtePhyEnb");
    xml feedbackComputation;
}

// 
// Relay LtePhy module of PHY Layer
//
simple LtePhyRelay extends LtePhyBase {
    @class("LtePhyRelay");
}


