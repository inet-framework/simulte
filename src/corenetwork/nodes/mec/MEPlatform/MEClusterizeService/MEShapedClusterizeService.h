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


#ifndef __SIMULTE_MESHAPEDCLUSTERIZESERVICE_H_
#define __SIMULTE_MESHAPEDCLUSTERIZESERVICE_H_

#include "MEClusterizeService.h"
#include "../../MEPlatform/GeneralServices/RadioNetworkInformation.h"


class MEShapedClusterizeService : public MEClusterizeService{


    double directionDelimiterThreshold;         // radiant: threshold within two cars are going in the same direction
    double proximityThreshold;                  // meter: threshold within two cars can communicate v2v

    double roadLaneSize;                        // meter: for rectangular platooning
    double triangleAngle;                       // radiant: for triangel platooning

    RadioNetworkInformation* rni;

    public:
        MEShapedClusterizeService();
        virtual ~MEShapedClusterizeService();

    protected:
        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);

        virtual void compute();

        void computePlatoon(std::string shape);

        void computeTriangleAdiacences(std::map<int, std::vector<int>> &adiacences);
        void computeRectangleAdiacences(std::map<int, std::vector<int>> &adiacences);
        void selectFollowers(std::map<int, std::vector<int>> &adiacences);
        void updateClusters();

        bool isInTriangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C);
        bool isInRectangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C, inet::Coord D);

        void resetCarFlagsAndControls();
        void resetClusters();
};

#endif
