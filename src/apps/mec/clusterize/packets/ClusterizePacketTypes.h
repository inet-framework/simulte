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



#ifndef APPS_VEHICULAR_MEC_CLUSTERIZE_PACKETS_CLUSTERIZEPACKETTYPES_H_
#define APPS_VEHICULAR_MEC_CLUSTERIZE_PACKETS_CLUSTERIZEPACKETTYPES_H_

//
//  defining type values for ClusterizePacket
//

#define START_CLUSTERIZE "ClusterizeStart"
#define STOP_CLUSTERIZE "ClusterizeStop"

#define ACK_START_CLUSTERIZE "ClusterizeStartAck"
#define ACK_STOP_CLUSTERIZE "ClusterizeStopAck"

#define INFO_CLUSTERIZE "ClusterizeInfo"
#define CONFIG_CLUSTERIZE "ClusterizeConfig"

// defining app and service names fro ME CLUSTERIZE SERVICE

#define ME_CLUSTERIZE_APP_MODULE_TYPE "lte.apps.mec.clusterize.MEClusterizeApp"
#define ME_CLUSTERIZE_APP_MODULE_NAME "MEClusterizeApp"
#define ME_CLUSTERIZE_SERVICE_MODULE_NAME "MEPlatooningService"

//
//  defining Tx Modes for ClusterizeConfigPacket
//
#define V2V_UNICAST_TX_MODE "V2V_UNICAST"
#define V2V_MULTICAST_TX_MODE "V2V_MULTICAST"
#define DOWNLINK_UNICAST_TX_MODE "DOWNLINK_UNICAST"
#define DOWNLINK_MULTICAST_TX_MODE "DOWNLINK_UNICAST"
#define HYBRID_TX_MODE "HYBRID"


#endif /* APPS_VEHICULAR_MEC_CLUSTERIZE_PACKETS_CLUSTERIZEPACKETTYPES_H_ */
