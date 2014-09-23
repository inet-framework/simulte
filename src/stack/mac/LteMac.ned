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


package lte.stack.mac;

// 
// Interface for the MAC layer of LTE Stack.
//
moduleinterface LteMac {
    parameters:
        @display("i=block/mac");
   
    gates:
        input RLC_to_MAC;    // RLC to MAC
        output MAC_to_RLC;    // MAC to RLC
        input PHY_to_MAC;    // PHY to MAC
        output MAC_to_PHY;    // MAC to PHY
}

// 
// Base module for the MAC layer of LTE Stack.
//
simple LteMacBase like LteMac {
    parameters:
        @display("i=block/mac");

        //# Mac Queues
        int queueSize @unit(B) = default(2MiB);              // MAC Buffers queue size
        
        //# Mac Scheduler
        int    maxBytesPerTti @unit(B) = default(3000KiB);          // Schedulable Bytes
        
        //# Mac MIB
        
        bool muMimo = default(true);
        
                
        //# H-ARQ
        int harqProcesses = default(8);
        int maxHarqRtx = default(4);
         
        //#
        //# Statistic recording: end2end delay and throughput at the mac layer
        //#
        @statistic[macDelayDl](title="Delay at the MAC layer UL"; unit="s"; source="macDelayDl"; record=lteAvg);
        @statistic[macThroughputDl](title="Throughput at the MAC layer DL"; unit="Bps"; source="macThroughputDl"; record=lteRate);
        @statistic[macDelayUl](title="Delay at the MAC layer UL"; unit="s"; source="macDelayUl"; record=lteAvg);
        @statistic[macThroughputUl](title="Throughput at the MAC layer UL"; unit="Bps"; source="macThroughputUl"; record=lteRate);
        @statistic[macCellThroughputUl](title="Cell Throughput at the MAC layer UL"; unit="Bps"; source="macCellThroughputUl"; record=lteRate);
        @statistic[macCellThroughputDl](title="Cell Throughput at the MAC layer DL"; unit="Bps"; source="macCellThroughputDl"; record=lteRate);
        @statistic[macCellPacketLossDl](title="Mac Cell Packet Loss"; unit=""; source="macCellPacketLossDl"; record=mean);
        @statistic[macCellPacketLossUl](title="Mac Cell Packet Loss"; unit=""; source="macCellPacketLossUl"; record=mean);
        @statistic[macPacketLossUl](title="Mac Packet Loss"; unit=""; source="macPacketLossUl"; record=mean);
        @statistic[macPacketLossDl](title="Mac Packet Loss"; unit=""; source="macPacketLossDl"; record=mean);
        @statistic[macBufferOverFlowDl](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowDl"; record=lteRate);
        @statistic[macBufferOverFlowUl](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowUl"; record=lteRate);
        @statistic[harqErrorRateUl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRateUl"; record=mean);
        @statistic[harqErrorRateDl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRateDl"; record=mean);
        @statistic[harqErrorRate_1st_Ul](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_1st_Ul"; record=mean);
        @statistic[harqErrorRate_1st_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_1st_Dl"; record=mean);
        @statistic[harqErrorRate_2nd_Ul](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_2nd_Ul"; record=mean);
        @statistic[harqErrorRate_2nd_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_2nd_Dl"; record=mean);
        @statistic[harqErrorRate_3rd_Ul](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_3rd_Ul"; record=mean);
        @statistic[harqErrorRate_3rd_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_3rd_Dl"; record=mean);
        @statistic[harqErrorRate_4th_Ul](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_4th_Ul"; record=mean);
        @statistic[harqErrorRate_4th_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_4th_Dl"; record=mean);
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[receivedPacketFromLowerLayer](source="receivedPacketFromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[sentPacketToUpperLayer](source="sentPacketToUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);


		@statistic[measuredItbs](title="TBS index"; unit=""; source="measuredItbs"; record=mean,vector);
		@statistic[measuredItbs_lte](title="TBS index"; unit=""; source="measuredItbs_lte"; record=lteAvg);

    gates:
        //# 
        //# Gates connecting RLC and MAC Layers
        //# 
        
        input RLC_to_MAC;    // RLC to MAC
        output MAC_to_RLC;    // MAC to RLC
        
        //# 
        //# Gates connecting MAC and PHY Layers
        //# 
        
        input PHY_to_MAC;    // PHY to MAC
        output MAC_to_PHY;    // MAC to PHY
}

//
// User Equipment MAC layer of LTE stack
//
simple LteMacUe extends LteMacBase {
    @class("LteMacUe");
}

//
// eNodeB MAC layer of LTE stack
//
simple LteMacEnb extends LteMacBase {
    
    parameters:
        
        @class("LteMacEnb");    
        
        volatile xml optSolution = default(xmldoc("solution.sol"));
        
        //#
        //# AMC Parameters
        //#
        
        // AMC Mode:  "auto", "piloted", "multi", "das"
        string amcMode = default("AUTO");
        
        //Min disance used to pair two MuMimo UE
        int muMimoMinDistance =default(50);
        
        // resource allocation type ("distributed" or "localized")
        string rbAllocationType = default("distributed");

        // summary feedback confidence function lower bound
        double summaryLowerBound @unit(s) = default(5ms);

        // summary feedback confidence function upper bound
        double summaryUpperBound @unit(s) = default(20ms);

        // FeedBack Historical Base capacity in DL (number of stored feedback samples per UE) 
        int fbhbCapacityDl = default(5);

        // FeedBack Historical Base capacity in UL (number of stored feedback samples per UE)
        int fbhbCapacityUl = default(5);

        // wideband PMI generation parameter (0.0 means "use the mean value" )        
        double pmiWeight = default(0.0);

        // wideband CQI generation parameter (0.0 means "use the mean value" )
        double cqiWeight = default(0.0);

        // AMC k CQI: used in DAS AMC mode
        int kCqi = default(0);
       
    
        //#
        //# eNb Scheduler Parameters
        //#    
        // Scheduling discipline. See LteCommon.h for discipline meaning.
        string schedulingDisciplineDl = default("MAXCI");
        string schedulingDisciplineUl = default("MAXCI");

        // Grant type DL
        string grantTypeConversationalDl = default("FITALL");
        string grantTypeStreamingDl      = default("FITALL");
        string grantTypeInteractiveDl    = default("FITALL");
        string grantTypeBackgroundDl     = default("FITALL");
        
        // Grant Size Dl in bytes (-1 means max grant, i.e. 4GB)
        int grantSizeConversationalDl = default(-1);
        int grantSizeStreamingDl      = default(-1);
        int grantSizeInteractiveDl    = default(-1);
        int grantSizeBackgroundDl     = default(-1);

        // Grant type Ul
        string grantTypeConversationalUl = default("FITALL");
        string grantTypeStreamingUl      = default("FITALL");
        string grantTypeInteractiveUl    = default("FITALL");
        string grantTypeBackgroundUl     = default("FITALL");

        // Grant Size Ul in bytes (-1 means max grant, i.e. 4GB)
        int grantSizeConversationalUl = default(-1);
        int grantSizeStreamingUl      = default(-1);
        int grantSizeInteractiveUl    = default(-1);
        int grantSizeBackgroundUl     = default(-1);
        
        // Proportional Fair parameters
        double pfAlpha    = default(0.95);
        
        // LTE Advanced Scheduler general parameters - DL
        int lteAallocationRbsDl = default(1);
        int lteAhistorySizeDL = default(20);
        int lteAgainHistoryThCONVERSATIONALDL = default(15);
        int lteAgainHistoryThSTREAMINGDL = default(15);
        int lteAgainHistoryThINTERACTIVEDL = default(15);
        int lteAgainHistoryThBACKGROUNDDL = default(15);
        
        // LTE Advanced Scheduler general parameters - UL 
        int lteAallocationRbsUl = default(1);
        int lteAhistorySizeUL = default(20);
        int lteAgainHistoryThUL = default(5);
            
        // LTE Advanced Scheduler QOS Parameters - DL
        double lteAdeadlineCONVERSATIONALDL @unit(s) = default(200ms);
        double lteAdeadlineSTREAMINGDL @unit(s) = default(1000ms);
        double lteAdeadlineINTERACTIVEDL @unit(s) = default(50ms);
        double lteAdeadlineBACKGROUNDDL @unit(s) = default(-1ms);
        
        double lteAslackTermCONVERSATIONALDL @unit(s) = default(50ms);
        double lteAslackTermSTREAMINGDL @unit(s) = default(100ms);
        double lteAslackTermINTERACTIVEDL @unit(s) = default(10ms);
        double lteAslackTermBACKGROUNDDL @unit(s) = default(0ms);
        
        int lteAmaxUrgentBurstCONVERSATIONALDL @unit(byte) = default(0);
        int lteAmaxUrgentBurstSTREAMINGDL @unit(byte)     = default(0);
        int lteAmaxUrgentBurstINTERACTIVEDL @unit(byte)    = default(0);
        int lteAmaxUrgentBurstBACKGROUNDDL   @unit(byte)  = default(200B);
        
        int lteAmaxFairnessBurstCONVERSATIONALDL @unit(byte) = default(0);
        int lteAmaxFairnessBurstSTREAMINGDL @unit(byte)     = default(0);
        int lteAmaxFairnessBurstINTERACTIVEDL @unit(byte)    = default(0);
        int lteAmaxFairnessBurstBACKGROUNDDL   @unit(byte)  = default(200B);
        
        // LTE Advanced Scheduler QOS Parameters - UL
        double lteAdeadlineCONVERSATIONALUL @unit(s) = default(250ms);
        double lteAdeadlineSTREAMINGUL @unit(s) = default(1500ms);
        double lteAdeadlineINTERACTIVEUL @unit(s) = default(2500ms);
        double lteAdeadlineBACKGROUNDUL @unit(s) = default(250ms);
        
        double lteAslackTermCONVERSATIONALUL @unit(s) = default(50ms);
        double lteAslackTermSTREAMINGUL @unit(s) = default(500ms);
        double lteAslackTermINTERACTIVEUL @unit(s) = default(500ms);
        double lteAslackTermBACKGROUNDUL @unit(s) = default(0ms);
        
        int lteAmaxBurstCONVERSATIONALUL @unit(byte) = default(0);
        int lteAmaxBurstSTREAMINGUL     @unit(byte) = default(0);
        int lteAmaxBurstINTERACTIVEUL   @unit(byte) = default(0);
        int lteAmaxBurstBACKGROUNDUL    @unit(byte) = default(0);
        
        // LTE Advanced Scheduler PF Version - DL
        bool pfTmsAwareDL = default(false);
        // LTE Advanced Scheduler PF Version - UL
        bool pfTmsAwareUL = default(false);
        
        // algorithm activation thresholds. 
        int layer0th= default (10000);
        int layer1th= default (8000);
        int layer2th= default (5000);
        int layer3th= default (2000);
        

        
        // D2 class of service percentages
        double goldUsers = default(0.2);
        double silverUsers = default(0.4);
        
        
        // Power Model Parameters for MBSFN frame
        // minimum depleted power (W)
        double zeroLevelMBSFNDL @unit(W)= default(260W); //504
        // minimum depleted power (W)
        double zeroLevelMBSFNUL @unit(W)= default(260W); 
        // idle state depleted power (W)
        double idleLevelMBSFNDL @unit(W)= default(150W); //336
        // idle state depleted power (W)
        double idleLevelMBSFNUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitMBSFNDL @unit(W)= default(3.76W); //6.72
        // per-block depletion unit (W)
        double powerUnitMBSFNUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerMBSFNDL @unit(W)= default(448W); //840
        // maximumum depletable power (W)
        double maxPowerMBSFNUL @unit(W)= default(448W);
        
        // Power Model Parameters for PAGING FRAMES
        // minimum depleted power (W)
        double zeroLevelPAGINGDL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelPAGINGUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelPAGINGDL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelPAGINGUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitPAGINGDL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitPAGINGUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerPAGINGDL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerPAGINGUL @unit(W)= default(448W);
        
        // Power Model Parameters for NORMAL frames
        // minimum depleted power (W)
        double zeroLevelNORMAL_FRAME_TYPEDL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelNORMAL_FRAME_TYPEUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelNORMAL_FRAME_TYPEDL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelNORMAL_FRAME_TYPEUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitNORMAL_FRAME_TYPEDL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitNORMAL_FRAME_TYPEUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerNORMAL_FRAME_TYPEDL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerNORMAL_FRAME_TYPEUL @unit(W)= default(448W);
        
        // Power Model Parameters for BROADCAST frames
        // minimum depleted power (W)
        double zeroLevelBROADCASTDL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelBROADCASTUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelBROADCASTDL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelBROADCASTUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitBROADCASTDL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitBROADCASTUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerBROADCASTDL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerBROADCASTUL @unit(W)= default(448W);
        
        // Power Model Parameters for SYNCRO frames
        // minimum depleted power (W)
        double zeroLevelSYNCRODL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelSYNCROUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelSYNCRODL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelSYNCROUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitSYNCRODL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitSYNCROUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerSYNCRODL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerSYNCROUL @unit(W)= default(448W);
        
        double prfFixedSYNCRO @unit(W)= default(5.543W);
        double prfRBSYNCRO @unit(W)= default(0.689W);
        
        double prfFixedMBSFN @unit(W)=default(4.4444W);
        double prfRBMBSFN @unit(W)=default(0.711W);
        
        double prfFixedNORMAL_FRAME_TYPE @unit(W)= default(3.810W);
        double prfRBNORMAL_FRAME_TYPE @unit(W)= default(0.724W);
        
        double prfFixedPAGING @unit(W)= default(3.810W);
        double prfRBPAGING @unit(W)= default(0.724W);
        
        double prfFixedBROADCAST @unit(W)= default(4.4W);
        double prfRBBROADCAST @unit(W)= default(0.712W);
        
        
        //# Statistics for Lte Scheduler Enb
        @statistic[tmsTxDiv](title="how many time TMS has coiche TxDIV respect to the other"; unit="ratio"; source="tmsTxDiv"; record=lteAvg);
        @statistic[tmsSMux](title="how many time TMS has coiche SMUX respect to the other"; unit="ratio"; source="tmsSMux"; record=lteAvg);
        @statistic[tmsMuMimo](title="how many time TMS has coiche MuMIMO respect to the other"; unit="ratio"; source="tmsMuMimo"; record=lteAvg);
        
        //#
        //# Statistics for Lte Scheduler Enb
        @statistic[cellBlocksUtilizationDl](title="LTE Cell Blocks Utilization Dl"; unit="blocks"; source="cellBlocksUtilizationDl"; record=lteAvg);
        @statistic[cellBlocksUtilizationUl](title="LTE Cell Blocks Utilization Ul"; unit="blocks"; source="cellBlocksUtilizationUl"; record=lteAvg);
        @statistic[avgServedBlocksDl](title="LTE Avg Served Blocks Dl"; unit="blocks"; source="avgServedBlocksDl"; record=lteAvg);
        @statistic[avgServedBlocksUl](title="LTE Avg Served Blocks Ul"; unit="blocks"; source="avgServedBlocksUl"; record=lteAvg);
        @statistic[depletedPowerDl](title="LTE ENodeB depleted power Dl"; unit="watts"; source="depletedPowerDl"; record=lteAvg);
        @statistic[depletedPowerUl](title="LTE ENodeB depleted power Ul"; unit="watts"; source="depletedPowerUl"; record=lteAvg);
        @statistic[activatedFrames](title="D1 algo, ratio of activated frame"; unit="ratio"; source="activetedFrames"; record=lteAvg);
        @statistic[sleepFrames](title="D1 algo, ratio of sleep frame"; unit="ratio"; source="sleepFrames"; record=lteAvg);
        @statistic[wastedFrames](title="D1 algo, ratio of activated frame with no traffic to serve"; unit="ratio"; source="wastedFrames"; record=lteAvg);
        
        @statistic[prf_0](unit="ratio"; source="prf_0"; record=lteAvg);
        @statistic[prf_1a](unit="ratio"; source="prf_1a"; record=lteAvg);
        @statistic[prf_2a](  unit="ratio"; source="prf_2a"; record=lteAvg);
        @statistic[prf_3a](  unit="ratio"; source="prf_3a"; record=lteAvg);
        @statistic[prf_1b](  unit="ratio"; source="prf_1b"; record=lteAvg);
        @statistic[prf_2b](  unit="ratio"; source="prf_2b"; record=lteAvg);
        @statistic[prf_3b](  unit="ratio"; source="prf_3b"; record=lteAvg);
        @statistic[prf_1c](  unit="ratio"; source="prf_1c"; record=lteAvg);
        @statistic[prf_2c](  unit="ratio"; source="prf_2c"; record=lteAvg);
        @statistic[prf_3c](  unit="ratio"; source="prf_3c"; record=lteAvg);
        @statistic[prf_4](  unit="ratio"; source="prf_4"; record=lteAvg);
        @statistic[prf_5](  unit="ratio"; source="prf_5"; record=lteAvg);
        @statistic[prf_6a](  unit="ratio"; source="prf_6a"; record=lteAvg);
        @statistic[prf_7a](  unit="ratio"; source="prf_7a"; record=lteAvg);
        @statistic[prf_8a](  unit="ratio"; source="prf_8a"; record=lteAvg);
        @statistic[prf_6b](  unit="ratio"; source="prf_6b"; record=lteAvg);
        @statistic[prf_7b](  unit="ratio"; source="prf_7b"; record=lteAvg);
        @statistic[prf_8b](  unit="ratio"; source="prf_8b"; record=lteAvg);
        @statistic[prf_6c](  unit="ratio"; source="prf_6c"; record=lteAvg);
        @statistic[prf_7c](  unit="ratio"; source="prf_7c"; record=lteAvg);
        @statistic[prf_8c](  unit="ratio"; source="prf_8c"; record=lteAvg);
        @statistic[prf_9](  unit="ratio"; source="prf_9"; record=lteAvg);
        
        @statistic[rb_0](  unit="ratio"; source="rb_0"; record=lteAvg);
        @statistic[rb_1a](  unit="ratio"; source="rb_1a"; record=lteAvg);
        @statistic[rb_2a](  unit="ratio"; source="rb_2a"; record=lteAvg);
        @statistic[rb_3a](  unit="ratio"; source="rb_3a"; record=lteAvg);
        @statistic[rb_1b](  unit="ratio"; source="rb_1b"; record=lteAvg);
        @statistic[rb_2b](  unit="ratio"; source="rb_2b"; record=lteAvg);
        @statistic[rb_3b](  unit="ratio"; source="rb_3b"; record=lteAvg);
        @statistic[rb_1c](  unit="ratio"; source="rb_1c"; record=lteAvg);
        @statistic[rb_2c](  unit="ratio"; source="rb_2c"; record=lteAvg);
        @statistic[rb_3c](  unit="ratio"; source="rb_3c"; record=lteAvg);
        @statistic[rb_4](  unit="ratio"; source="rb_4"; record=lteAvg);
        @statistic[rb_5](  unit="ratio"; source="rb_5"; record=lteAvg);
        @statistic[rb_6a](  unit="ratio"; source="rb_6a"; record=lteAvg);
        @statistic[rb_7a](  unit="ratio"; source="rb_7a"; record=lteAvg);
        @statistic[rb_8a](  unit="ratio"; source="rb_8a"; record=lteAvg);
        @statistic[rb_6b](  unit="ratio"; source="rb_6b"; record=lteAvg);
        @statistic[rb_7b](  unit="ratio"; source="rb_7b"; record=lteAvg);
        @statistic[rb_8b](  unit="ratio"; source="rb_8b"; record=lteAvg);
        @statistic[rb_6c](  unit="ratio"; source="rb_6c"; record=lteAvg);
        @statistic[rb_7c](  unit="ratio"; source="rb_7c"; record=lteAvg);
        @statistic[rb_8c](  unit="ratio"; source="rb_8c"; record=lteAvg);
        @statistic[rb_9](  unit="ratio"; source="rb_9"; record=lteAvg);
        
        @statistic[cqiDlSiso0](title="Average cqi siso band 0"; unit="cqi"; source="cqiDlSiso0"; record=lteAvg);
        @statistic[cqiDlSiso1](title="Average cqi siso band 1"; unit="cqi"; source="cqiDlSiso1"; record=lteAvg);
        @statistic[cqiDlSiso2](title="Average cqi siso band 2"; unit="cqi"; source="cqiDlSiso2"; record=lteAvg);
        @statistic[cqiDlSiso3](title="Average cqi siso band 3"; unit="cqi"; source="cqiDlSiso3"; record=lteAvg);
        @statistic[cqiDlSiso4](title="Average cqi siso band 4"; unit="cqi"; source="cqiDlSiso4"; record=lteAvg);
    
        @statistic[cqiDlSpmux0](title="Average cqi Spmux band 0"; unit="cqi"; source="cqiDlSpmux0"; record=lteAvg);
        @statistic[cqiDlSpmux1](title="Average cqi Spmux band 1"; unit="cqi"; source="cqiDlSpmux1"; record=lteAvg);
        @statistic[cqiDlSpmux2](title="Average cqi Spmux band 2"; unit="cqi"; source="cqiDlSpmux2"; record=lteAvg);
        @statistic[cqiDlSpmux3](title="Average cqi Spmux band 3"; unit="cqi"; source="cqiDlSpmux3"; record=lteAvg);
        @statistic[cqiDlSpmux4](title="Average cqi Spmux band 4"; unit="cqi"; source="cqiDlSpmux4"; record=lteAvg);
        
        @statistic[cqiDlTxDiv0](title="Average cqi TxDiv band 0"; unit="cqi"; source="cqiDlTxDiv0"; record=lteAvg);
        @statistic[cqiDlTxDiv1](title="Average cqi TxDiv band 1"; unit="cqi"; source="cqiDlTxDiv1"; record=lteAvg);
        @statistic[cqiDlTxDiv2](title="Average cqi TxDiv band 2"; unit="cqi"; source="cqiDlTxDiv2"; record=lteAvg);
        @statistic[cqiDlTxDiv3](title="Average cqi TxDiv band 3"; unit="cqi"; source="cqiDlTxDiv3"; record=lteAvg);
        @statistic[cqiDlTxDiv4](title="Average cqi TxDiv band 4"; unit="cqi"; source="cqiDlTxDiv4"; record=lteAvg);
        
        @statistic[cqiDlMuMimo0](title="Average cqi MuMimo band 0"; unit="cqi"; source="cqiDlMuMimo0"; record=lteAvg);
        @statistic[cqiDlMuMimo1](title="Average cqi MuMimo band 1"; unit="cqi"; source="cqiDlMuMimo1"; record=lteAvg);
        @statistic[cqiDlMuMimo2](title="Average cqi MuMimo band 2"; unit="cqi"; source="cqiDlMuMimo2"; record=lteAvg);
        @statistic[cqiDlMuMimo3](title="Average cqi MuMimo band 3"; unit="cqi"; source="cqiDlMuMimo3"; record=lteAvg);
        @statistic[cqiDlMuMimo4](title="Average cqi MuMimo band 4"; unit="cqi"; source="cqiDlMuMimo4"; record=lteAvg);    
}    
//
// Relay (UE side) MAC layer of LTE stack
// Connection: eNodeB <--> Relay
//
simple LteMacRelayUe extends LteMacUe 
{
    @class("LteMacRelayUe");
}

//
// Relay (eNB side) MAC layer of LTE stack
// Connection: Relay <--> UE
//
simple LteMacRelayEnb extends LteMacEnb 
{
    @class("LteMacRelayEnb");
}
