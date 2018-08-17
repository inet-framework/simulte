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

#include <cmath>
#include <algorithm>
#include "MEPlatooningService.h"

Define_Module(MEPlatooningService);

void MEPlatooningService::initialize(int stage)
{
    EV << "MEPlatooningService::initialize - stage " << stage << endl;
    MEClusterizeService::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;
    //---------------------------------------
    //general service
    rni = check_and_cast<RadioNetworkInformation*>(mePlatform->getSubmodule("rniService"));
    //---------------------------------------
    //shape for clustering
    shape = par("shape").stringValue();
    //---------------------------------------
    //direction
    directionDelimiterThreshold = par("directionDelimiterThreshold").doubleValue(); //radiant
    //---------------------------------------
    //area dimensions
    proximityThreshold = par("proximityThreshold").doubleValue();                   //meter
    directionDelimiterThreshold = par("directionDelimiterThreshold").doubleValue(); //radiant
    roadLaneSize = par("roadLaneSize").doubleValue();                               //meter
    triangleAngle = par("triangleAngle").doubleValue();                             //radiant
    //---------------------------------------
    //cluster mobility
    desiredVelocity = par("desiredVelocity").doubleValue();                         //meter per second
    criticalDistance = par("criticalDistance").doubleValue();                         //meter
    MIN_ACCELERATION = par("minAcceleration").doubleValue();
    MAX_ACCELERATION = par("maxAcceleration").doubleValue();

    //controller initialization
    followerController = SafePlatooningController(period_.dbl(), MIN_ACCELERATION, MAX_ACCELERATION, criticalDistance);
    leaderController = GeneralSpeedController(period_.dbl(), MIN_ACCELERATION, MAX_ACCELERATION);
}

/*
 * #########################################################################################################################################
 */
void MEPlatooningService::compute()
{
    //update variable storing freshness
    lastRun = simTime();

    EV << "\nMEPlatooningService::compute - resetting flags and controls\n" << endl;
    resetCarFlagsAndControls();

    EV << "\nMEPlatooningService::compute - resetting clusters\n" << endl;
    resetClusters();

    //EV << "\nMEPlatooningService::compute - updating RNI infos\n" << endl;                                                //RNI not USED
    //updateRniInfo();

    EV << "\nMEPlatooningService::compute - updating car positions and speeds\n" << endl;
    updatePositionsAndSpeeds();

    EV << "\nMEPlatooningService::compute - computing platoons\n" << endl;
    computePlatoon(shape);  //shape is "rectangle" or "triangle"

    EV << "\nMEPlatooningService::compute - computing car accelerations\n" << endl;
    computePlatoonAccelerations();
}

/*
 * #########################################################################################################################################
 */

void MEPlatooningService::computePlatoon(std::string shape){

    std::map<int, std::vector<int>> adjacencies;
    if(!shape.compare("triangle"))
    {
        EV << "MEPlatooningService::computePlatoon - computing Triangle Adjacences..." << endl;
        computeTriangleAdjacencies(adjacencies);
    }
    else if(!shape.compare("rectangle"))
    {
        EV << "MEPlatooningService::computePlatoon - computing Rectangle adjacencies..." << endl;
        computeRectangleAdjacencies(adjacencies);
    }
    else
        throw cRuntimeError("MEPlatooningService::computePlatoon - \tFATAL! Error in the shape value!");

    EV << "MEPlatooningService::computePlatoon - selecting Followers...\n";
    selectFollowers(adjacencies);

    EV << "MEPlatooningService::computePlatoon - Updating Clusters...\n";
    updateClusters();

    //-----------------------------------------------------------TESTING PRINTS-------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------------------------------------
    EV << "\nMEPlatooningService::computePlaaton - FOLLOWERS:\n\n";
    std::map<int, car>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++)
    {
          EV << it->second.symbolicAddress << "\t followed by:\t" << it->second.follower << "\t[following:\t" << it->second.following << "] ";
          EV << "\t position: " << it->second.position << " speed: " << it->second.speed << " angle:" << it->second.angularPosition.alpha << " time-stamp: " << it->second.timestamp << endl;
    }
    EV << "\nMEPlatooningService::computePlatoon - CLUSTERS:\n\n";
    std::map<int, cluster>::iterator cit;
    for(cit = clusters.begin(); cit != clusters.end(); cit++)
    {
            EV << "Cluster #" << cit->second.id << " (" << cit->second.color << ") : " << cit->second.membersList << "\n";
            EV << "cluster members keys: ";
            for(std::vector<int>::iterator i = cit->second.members.begin(); i != cit->second.members.end(); i++)
                EV << *i << " ";
            EV << endl << endl;
    }
    //--------------------------------------------------------------------------------------------------------------------------------------
}

void MEPlatooningService::computeTriangleAdjacencies(std::map<int, std::vector<int>> &adjacencies){

    std::map<int, car>::iterator it, it2;
    for(it = cars.begin(); it != cars.end(); it++)
    {
        inet::Coord A(it->second.position);
        double a = it->second.angularPosition.alpha;
        inet::Coord B( A.x + proximityThreshold*cos(M_PI+a-triangleAngle) , A.y + proximityThreshold*sin(M_PI+a-triangleAngle) );
        inet::Coord C( A.x + proximityThreshold*cos(M_PI+a+triangleAngle) , A.y + proximityThreshold*sin(M_PI+a+triangleAngle) );

        for(it2 = cars.begin(); it2 != cars.end(); it2++)
        {
            //cars going in the same direction
            if( (it != it2) && (abs(it->second.angularPosition.alpha - it2->second.angularPosition.alpha) <= directionDelimiterThreshold))
            {
                inet::Coord P(it2->second.position);
                //adding to the vector of Adjacencies of car it
                if(isInTriangle(P,A,B,C))   adjacencies[it->first].push_back(it2->first);
            }
        }
    }
}

void MEPlatooningService::computeRectangleAdjacencies(std::map<int, std::vector<int>> &adjacencies){

    std::map<int, car>::iterator it, it2;
    for(it = cars.begin(); it != cars.end(); it++)
    {
        double a = it->second.angularPosition.alpha;
        inet::Coord pos(it->second.position);
        inet::Coord A( pos.x + (roadLaneSize/2)*cos(a+M_PI/2), pos.y + (roadLaneSize/2)*sin(a+M_PI/2) );        // left-right side          A-------B
        inet::Coord B( pos.x + (roadLaneSize/2)*cos(a-M_PI/2), pos.y + (roadLaneSize/2)*sin(a-M_PI/2) );        // left-right side          -       -
        inet::Coord C( B.x + proximityThreshold*cos(M_PI+a), B.y + proximityThreshold*sin(M_PI+a) );            // behind side              -       -
        inet::Coord D( A.x + proximityThreshold*cos(M_PI+a), A.y + proximityThreshold*sin(M_PI+a) );            // behind side              D-------C

        for(it2 = cars.begin(); it2 != cars.end(); it2++)
        {
            //cars going in the same direction
            if( (it != it2) && (abs(it->second.angularPosition.alpha - it2->second.angularPosition.alpha) <= directionDelimiterThreshold))
            {
                inet::Coord P(it2->second.position);
                //adding to the vector of adjacencies of car it
                if(isInRectangle(P, A, B, C, D))    adjacencies[it->first].push_back(it2->first);
            }
        }
    }
}

void MEPlatooningService::selectFollowers(std::map<int, std::vector<int>> &adjacencies){

    std::map<int, std::vector<int>>::iterator it;
    bool found;
    for(it = adjacencies.begin(); it != adjacencies.end(); it++)
    {
        found = false;
        std::vector<int>::iterator it2 = it->second.begin();
        int follower = *it2;
        //finding the closer follower to it2 car among adjacent cars that are not yet followers
        for(; it2 != it->second.end(); it2++)
        {
            if(!cars[*it2].isFollower){
                // update follower if it2 car has distance lesser than the follower one!
                if(cars[it->first].position.distance(cars[*it2].position) <= cars[it->first].position.distance(cars[follower].position))
                {
                    follower = *it2;
                    found = true;
                }
            }
        }
        if(found)
        {
            //update control informations in cars entries
            cars[it->first].follower = cars[follower].symbolicAddress;
            cars[it->first].followerKey = follower;
            cars[follower].following = cars[it->first].symbolicAddress;
            cars[follower].followingKey = it->first;
            cars[follower].isFollower = true;
        }
    }
}

void MEPlatooningService::updateClusters()
{
    int colorSize = colors.size();

    std::map<int, car>::iterator it;
    //building clusters: finding the cluster leaders and moving on the followers
    for(it = cars.begin(); it != cars.end(); it++)
    {
        if(it->second.followingKey == -1)
        {
            //leader is found
            (it->second).isLeader = true;

            std::stringstream platoonList;
            int clusterID = (it->second).id;
            int k = it->first;
            //moving on followers
            while(k != -1)
            {
                clusters[clusterID].members.push_back(k);
                cars[k].clusterID = clusterID;

                //updating txMode for INFO_MEAPP ClusterizeConfigPacket message propagation
                //HYBRID_TX_MODE
                if(!strcmp(preconfiguredTxMode.c_str(), HYBRID_TX_MODE))
                {
                    //compute the optimal mode
                    cars[k].txMode = DOWNLINK_UNICAST_TX_MODE; // for now DOWNLINK_UNICAST by default!
                                                                                                        //TODO: adding the txMode computation! in case of HYBRID TX MODE!
                                                                                                        //compute the best txMode according to some policy --> CQI or TxPower..TODO
                }
                //STATIC TX_MODE: preconfigured
                else
                {
                    cars[k].txMode = preconfiguredTxMode.c_str();
                }

                platoonList << cars[k].symbolicAddress << " -> ";
                k = cars[k].followerKey;
            }
            //update the clusters entry
            clusters[clusterID].membersList = platoonList.str();
            clusters[clusterID].id = clusterID;
            clusters[clusterID].color = colors.at( (rand() + clusterID) % colorSize);      //rand() changes the cluster color at every computation!
        }
    }
}

bool MEPlatooningService::isInTriangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C){

      //considering all points relative to A
      inet::Coord v0 = B-A;   // B w.r.t A
      inet::Coord v1 = C-A;   // C w.r.t A
      inet::Coord v2 = P-A;   // P w.r.t A

      // writing v2 = u*v0 + v*v1 (linear combination in the Plane defined by vectors v0 and v1)
      // and finding u and v (scalar)
      double den = ((v0*v0) * (v1*v1)) - ((v0*v1) * (v1*v0));

      double u = ( ((v1*v1) * (v2*v0)) - ((v1*v0) * (v2*v1)) ) / den;
      double v = ( ((v0*v0) * (v2*v1)) - ((v0*v1) * (v2*v0)) ) / den;

      // checking if coefficients u and v are constrained in [0-1], that means inside the triangle ABC
      if(u>=0 && v>=0 && u+v<=1)    return true;

      else    return false;
}

bool MEPlatooningService::isInRectangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C, inet::Coord D)
{
      return isInTriangle(P, A, B, C) || isInTriangle(P, A, C, D);
      // left-right side          A-------B
      // left-right side          -  -    -
      // behind side              -    -  -
      // behind side              D-------C
}


void MEPlatooningService::updatePositionsAndSpeeds(){

    //updating car positions based on the last position & timestamp + velocity * elapsed_time + acceleration * elapsed_time^2
    double now = simTime().dbl();
    std::map<int, car>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++)
    {
        double old = it->second.timestamp.dbl();
        //regulating the time_gap taking into account the previous update on MEPlatooningService!
        double time_gap = (now-old > period_.dbl())? period_.dbl(): now - old;
        //update position
        it->second.position.x += it->second.speed.x*time_gap + it->second.acceleration*cos(it->second.angularPosition.alpha)*time_gap*time_gap;
        it->second.position.y += it->second.speed.y*time_gap + it->second.acceleration*sin(it->second.angularPosition.alpha)*time_gap*time_gap;
        //update speed
        it->second.speed.x += it->second.acceleration*cos(it->second.angularPosition.alpha)*time_gap;
        it->second.speed.y += it->second.acceleration*sin(it->second.angularPosition.alpha)*time_gap;
        //testing
        //EV << "MEPlatooningService::updatePositionsAndSpeeds - " << it->second.symbolicAddress << " position: " << it->second.position << " speed: " << it->second.speed << " time-stamp: " << now << endl ;
    }
}

void MEPlatooningService::computePlatoonAccelerations(){

    std::map<int, cluster>::iterator cit;
    for(cit = clusters.begin(); cit != clusters.end(); cit++)
    {
       int previous;
       for( int i : cit->second.members)
       {
           //leader
           if(cars[i].isLeader)
           {
                //using a general formula
                double acceleration = leaderController.getAcceleration(desiredVelocity, cars[i].speed.length());

                //updating clusters entry with acceleration computed for each member
                cit->second.accelerations.push_back(acceleration);
                //update acceleration
                cars[i].acceleration = acceleration;
                //updating platoon formation info
                cit->second.distancies.push_back(0);

                //testing
                double velocity_gap = desiredVelocity - cars[i].speed.length();
                EV << "MEPlatooningService::computePlatoonAccelerations - update "<< cars[i].symbolicAddress <<" (LEADER)\t";
                EV << " [position: " << cars[i].position << "] [acceleration: " << acceleration << "] velocity_gap: " << velocity_gap << endl;
                previous = i;
           }
           //members
           else
           {
                /*
                * Scheuer, Simonin and Charpillet: reaching desired velocity and mantaining the secure distance to avoid collision!
                */

                //Collision-free Longitudinal distance controller: inputs
                double distanceToLeading = cars[i].position.distance(cars[previous].position);
                double followerSpeed = cars[i].speed.length();
                double leadingSpeed = cars[previous].speed.length();
                //Collision-free Longitudinal distance controller: output
                double acceleration = followerController.getAcceleration(distanceToLeading, followerSpeed, leadingSpeed);

                //updating clusters entry with acceleration computed for each member
                cit->second.accelerations.push_back(acceleration);
                //update acceleration
                cars[i].acceleration = acceleration;
                //updating platoon formation info
                cit->second.distancies.push_back(distanceToLeading);

                //testing
                EV << "MEPlatooningService::computePlatoonAccelerations - update "<< cars[i].symbolicAddress <<" (MEMBER) following " << cars[previous].symbolicAddress;
                EV << " [speed: " << cars[i].speed.length() <<  "] [acceleration: " << acceleration << "] distance_to previous: " << distanceToLeading << endl;
           }
           previous = i;
       }
    }

//TESTING VARING VELOCITY OF LEADER:
double now = simTime().dbl();
if(now > 130)
    desiredVelocity = 7;
}

/*
 * #########################################################################################################################################
 */
void MEPlatooningService::resetClusters()
{
    clusters.clear();
}

void MEPlatooningService::resetCarFlagsAndControls()
{
    std::map<int, car>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++)
    {
        it->second.followingKey = -1;
        it->second.followerKey = -1;
        it->second.isFollower = false;
        it->second.isLeader = false;
        it->second.following = "";
        it->second.follower = "";
    }
}

void MEPlatooningService::updateRniInfo()
{    std::map<int, car>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++)
    {
        double txPower = rni->getUETxPower(it->second.symbolicAddress);     //check if != -1
        it->second.txPower = txPower;

        Cqi cqi = rni->getUEcqi(it->second.symbolicAddress);     //check if != -1
        it->second.cqi = cqi;

        //testing
        EV << "MEPlatooningService::updateRniInfo - " << it->second.symbolicAddress << " tx power: " << txPower << endl;
        EV << "MEPlatooningService::updateRniInfo - " << it->second.symbolicAddress << " cqi: " << cqi << endl;
    }
}

void MEPlatooningService::handleClusterizeStop(ClusterizePacket* pkt)
{
    //getting controllers map entry key
    int key = pkt->getArrivalGate()->getIndex();

    //erasing the map cars_velocity_controllers entry
    if(!cars_velocity_controllers.empty() && cars_velocity_controllers.find(key) != cars_velocity_controllers.end())
    {
        EV << "MEPlatooningService::handleClusterizeStop - Erasing cars_velocity_controllers[" << key << "]" << endl;
        std::map<int, SimpleVelocityController>::iterator it;
        it = cars_velocity_controllers.find(key);
        if (it != cars_velocity_controllers.end())  cars_velocity_controllers.erase (it);

    }
    //erasing the map cars_distance_controllers entry
    else if(!cars_distance_controllers.empty() && cars_distance_controllers.find(key) != cars_distance_controllers.end())
    {
        EV << "MEPlatooningService::handleClusterizeStop - Erasing cars_distance_controllers[" << key << "]" << endl;
        std::map<int, SimpleDistanceController>::iterator it;
        it = cars_distance_controllers.find(key);
        if (it != cars_distance_controllers.end())  cars_distance_controllers.erase (it);

    }

    MEClusterizeService::handleClusterizeStop(pkt);
}
