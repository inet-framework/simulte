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

#ifndef __SIMULTE_MEPLATOONINGSERVICE_H_
#define __SIMULTE_MEPLATOONINGSERVICE_H_

//SERVICES AND UTILITIES
#include "../../MEPlatform/GeneralServices/RadioNetworkInformation.h"
#include "MEClusterizeService.h"
#include "SimpleControllers.h"

/**
 * MEPlatooningService see MEPlatooningService.ned
 *
 *      implementing compute() with the following platoon formation algorithm:
 *              1) for each vehicle update its position to this instant by forecasting its mobility
 *              2) for each vehicle update Radio Network Informations (txPower and cqi)                 //not used
 *              3) for each vehicle compute its vector of adjacent vehicles, among all other vehicles (the ones inside the area defined by parameters)
 *              4) for each vehicle select the best follower among its adjacent vehicles
 *              5) build the clusters entry following the chains starting from vehicle leaders and moving on its followers
 *              6) for each cluster run the controller associated to each member: leader uses SimpleVelocityController while members use SimpleDistanceController
 */

class MEPlatooningService : public MEClusterizeService
{
    //general service
    RadioNetworkInformation* rni;
    //---------------------------------------
    //algorithm parameters
    //area shape for computing car adjacences
    std::string shape;
    //direction
    double directionDelimiterThreshold;
    //area dimensions
    double proximityThreshold;
    double roadLaneSize;
    double triangleAngle;
    //cluster mobility
    double desiredVelocity;
    double criticalDistance;
    double MAX_ACCELERATION;
    double MIN_ACCELERATION;
    //---------------------------------------
    //controllers (from matlab sisotool state space representation)
    std::map<int, SimpleVelocityController> cars_velocity_controllers;
    std::map<int, SimpleDistanceController> cars_distance_controllers;

    GeneralSpeedController leaderController;
    SafePlatooningController followerController;

    protected:
        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);
        //----------------------------------------------------------------------------------------------------------------------
        virtual void compute();
            //cleaning structures
            void resetCarFlagsAndControls();
            void resetClusters();
            //update, to this instant, car positions and speeds using received informations
            void updatePositionsAndSpeeds();
            //getting Radio Network Informations (i.e. txPower and CQI)
            void updateRniInfo();
            //computing candidate Platoon of vehicles
            void computePlatoon(std::string shape);
                //compute vehicle adjacences according the area shape
                void computeTriangleAdjacencies(std::map<int, std::vector<int>> &adjacencies);
                    //checking vehicles inside the area defined by the shape
                    bool isInTriangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C);
                void computeRectangleAdjacencies(std::map<int, std::vector<int>> &adjacencies);
                    //checking vehicles inside the area defined by the shape
                    bool isInRectangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C, inet::Coord D);
                //select the best follower vehicle (according to the distance)
                void selectFollowers(std::map<int, std::vector<int>> &adjacencies);
                //updating/building the clusters map with follower vehicle informations
                void updateClusters();
            //computing accelerations for each platoon member, for each platoon, to converge at desired speed and inter-vehicle distances
            void computePlatoonAccelerations();
        //----------------------------------------------------------------------------------------------------------------------
        //overriding the MEClusterizeService::handleClusterizeStop to delete the controllers map entry
        virtual void handleClusterizeStop(ClusterizePacket*);
};

#endif
