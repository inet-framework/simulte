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

#include <omnetpp.h>
#include "MEClusterizeService.h"
#include "../../MEPlatform/GeneralServices/RadioNetworkInformation.h"

#include "SimpleControllers.h"

class MEPlatooningService : public MEClusterizeService{

    double directionDelimiterThreshold;         // radiant: threshold within two cars are going in the same direction
    double proximityThreshold;                  // meter: threshold within two cars can communicate v2v

    double roadLaneSize;                        // meter: for rectangular platooning
    double triangleAngle;                       // radiant: for triangel platooning

    double desiredVelocity;
    double desiredDistance;

    std::string shape;

    RadioNetworkInformation* rni;

    //controllers
    std::map<int, SimpleVelocityController> cars_velocity_controllers;
    std::map<int, SimpleDistanceController> cars_distance_controllers;

    public:
        MEPlatooningService();
        virtual ~MEPlatooningService();

    protected:
        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);

        virtual void compute();

        //find vehicles that are able to form a platoon
        void computePlatoon(std::string shape);

        //platoon computation
        void computeTriangleAdiacences(std::map<int, std::vector<int>> &adiacences);
        void computeRectangleAdiacences(std::map<int, std::vector<int>> &adiacences);
        void selectFollowers(std::map<int, std::vector<int>> &adiacences);
        void updateClusters();

        //adjacences computation
        bool isInTriangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C);
        bool isInRectangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C, inet::Coord D);

        //compute accelerations for each vehicle in a platoon to converge at desired speed and inter-vehicle distances
        void computePlatoonAccelerations();

        //cleaning structures
        void resetCarFlagsAndControls();
        void resetClusters();

        //interpolate positions by using last update on position and speed and the time passed
        void interpolatePositions();

        //using the RNI service
        void updateRniInfo();

        //overriding to delete the controllers map entry
        virtual void handleClusterizeStop(ClusterizePacket*);
};

#endif
