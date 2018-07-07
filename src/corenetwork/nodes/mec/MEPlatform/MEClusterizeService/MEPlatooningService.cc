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


MEPlatooningService::MEPlatooningService() {

}

MEPlatooningService::~MEPlatooningService() {
}

void MEPlatooningService::initialize(int stage)
{
    EV << "MEPlatooningService::initialize - stage " << stage << endl;

    MEClusterizeService::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    proximityThreshold = par("proximityThreshold").doubleValue();                               //meter
    directionDelimiterThreshold = par("directionDelimiterThreshold").doubleValue();                //radiant
    roadLaneSize = par("roadLaneSize").doubleValue();                                           //meter
    triangleAngle = par("triangleAngle").doubleValue();                                             //radiant

    desiredVelocity = par("desiredVelocity").doubleValue();                                     //meter per second
    desiredDistance = par("desiredDistance").doubleValue();                                     //meter

    EV << "MEPlatooningService::initialize - proximityThreshold: " << proximityThreshold << " - directionDelimiterThreshold: " << directionDelimiterThreshold << endl;

    //shape for clustering
    shape = par("shape").stringValue();

    rni = check_and_cast<RadioNetworkInformation*>(mePlatform->getSubmodule("rniService"));
}
/*
 * #########################################################################################################################################
 */

void MEPlatooningService::compute(){

    //cleaning cluster-info and control/flags in data structures: clusters and cars

    EV << "\nMEPlatooningService::compute - resetting flags and controls\n" << endl;
    resetCarFlagsAndControls();

    EV << "\nMEPlatooningService::compute - resetting clusters\n" << endl;
    resetClusters();

    //updating rni-info in data structures cars
    //EV << "\nMEPlatooningService::compute - updating RNI infos\n" << endl;
    //updateRniInfo();

    //interpolate positions
    EV << "\nMEPlatooningService::compute - updating car positions\n" << endl;
    updatePositions();

    //executing algorithm
    EV << "\nMEPlatooningService::compute - computing platoons\n" << endl;
    computePlatoon(shape);  //shape is "rectangle" or "triangle"

    //executing controllers
    EV << "\nMEPlatooningService::compute - computing car accelerations\n" << endl;
    computePlatoonAccelerations();
}

/*
 * #########################################################################################################################################
 */

void MEPlatooningService::computePlatoon(std::string shape){

    EV << "MEPlatooningService::computePlatoon - starting clustering algorithm" << endl;

    std::map<int, std::vector<int>> adiacences;
    // Build Adiacences
    if(!shape.compare("triangle")){
        EV << "MEPlatooningService::computePlatoon - computing Triangle Adjacences..." << endl;
        computeTriangleAdiacences(adiacences);
    }
    else if(!shape.compare("rectangle")){
        EV << "MEPlatooningService::computePlatoon - computing Rectangle Adjacences..." << endl;
        computeRectangleAdiacences(adiacences);
    }
    else
        throw cRuntimeError("MEPlatooningService::computePlatoon - \tFATAL! Error in the shape value!");

    // Select Best Follower
    EV << "MEPlatooningService::computePlatoon - selecting Followers...\n";
    selectFollowers(adiacences);

    // Scan the v2vConfig to update the platoonList filed!
    EV << "MEPlatooningService::computePlatoon - Updating Clusters...\n";
    updateClusters();

    //TESTING PRINTS------------------------------------------------------------------------------------------------------------------------
    /*
    EV << "\nMEPlatooningService::computePlaaton - FOLLOWERS:\n\n";
    std::map<int, car>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++){

          EV << it->second.simbolicAddress << "\tfollowed by:\t" << it->second.follower << "\t[following:\t" << it->second.following << "] ";
          EV << "\t\tposition: " << it->second.position << " angle:" << it->second.angularPosition.alpha << endl;
    }
    */
    EV << "\nMEPlatooningService::computePlatoon - CLUSTERS:\n\n";
    std::map<int, cluster>::iterator cit;
    for(cit = clusters.begin(); cit != clusters.end(); cit++){

            EV << "Cluster #" << cit->second.id << " (" << cit->second.color << ") : " << cit->second.membersList << "\n";

            EV << "cluster members keys: ";
            for(std::vector<int>::iterator i = cit->second.members.begin(); i != cit->second.members.end(); i++)
                EV << *i << " ";
            EV << endl << endl;
    }

    //--------------------------------------------------------------------------------------------------------------------------------------
}

void MEPlatooningService::computeTriangleAdiacences(std::map<int, std::vector<int>> &adiacences){

    std::map<int, car>::iterator it, it2;
    for(it = cars.begin(); it != cars.end(); it++){

        inet::Coord A(it->second.position);
        double a = it->second.angularPosition.alpha;
        inet::Coord B( A.x + proximityThreshold*cos(M_PI+a-triangleAngle) , A.y + proximityThreshold*sin(M_PI+a-triangleAngle) );
        inet::Coord C( A.x + proximityThreshold*cos(M_PI+a+triangleAngle) , A.y + proximityThreshold*sin(M_PI+a+triangleAngle) );

        for(it2 = cars.begin(); it2 != cars.end(); it2++){

            //cars going in the same direction
            if( (it != it2) && (abs(it->second.angularPosition.alpha - it2->second.angularPosition.alpha) <= directionDelimiterThreshold)){

                inet::Coord P(it2->second.position);
                //adding to the vector of adiacences of car it
                if(isInTriangle(P,A,B,C)){
                    adiacences[it->first].push_back(it2->first);
                }
            }
        }
    }
}

void MEPlatooningService::computeRectangleAdiacences(std::map<int, std::vector<int>> &adiacences){

    std::map<int, car>::iterator it, it2;
    for(it = cars.begin(); it != cars.end(); it++){

        double a = it->second.angularPosition.alpha;
        inet::Coord pos(it->second.position);
        inet::Coord A( pos.x + (roadLaneSize/2)*cos(a+M_PI/2), pos.y + (roadLaneSize/2)*sin(a+M_PI/2) );        // left-right side          A-------B
        inet::Coord B( pos.x + (roadLaneSize/2)*cos(a-M_PI/2), pos.y + (roadLaneSize/2)*sin(a-M_PI/2) );        // left-right side          -       -
        inet::Coord C( B.x + proximityThreshold*cos(M_PI+a), B.y + proximityThreshold*sin(M_PI+a) );            // behind side              -       -
        inet::Coord D( A.x + proximityThreshold*cos(M_PI+a), A.y + proximityThreshold*sin(M_PI+a) );            // behind side              D-------C

        for(it2 = cars.begin(); it2 != cars.end(); it2++){

            //cars going in the same direction
            if( (it != it2) && (abs(it->second.angularPosition.alpha - it2->second.angularPosition.alpha) <= directionDelimiterThreshold)){

                inet::Coord P(it2->second.position);
                //adding to the vector of adjacencies of car it
                if(isInRectangle(P, A, B, C, D)){
                    //EV << "inside!";
                    adiacences[it->first].push_back(it2->first);
                }
            }
        }
    }
}

void MEPlatooningService::selectFollowers(std::map<int, std::vector<int>> &adiacences){

    std::map<int, std::vector<int>>::iterator it;
    bool found;
    for(it = adiacences.begin(); it != adiacences.end(); it++){

        found = false;
        std::vector<int>::iterator it2 = it->second.begin();
        int follower = *it2;

        for( it2; it2 != it->second.end(); it2++){

            if(!cars[*it2].isFollower){
                // update follower if it2 car has distance lesser than the follower one!
                if(cars[it->first].position.distance(cars[*it2].position) <= cars[it->first].position.distance(cars[follower].position)){
                    follower = *it2;
                    found = true;
                }
            }
        }
        if(found){
            cars[it->first].follower = cars[follower].simbolicAddress;
            cars[it->first].followerKey = follower;

            cars[follower].following = cars[it->first].simbolicAddress;
            cars[follower].followingKey = it->first;
            cars[follower].isFollower = true;
        }
    }
}

void MEPlatooningService::updateClusters(){

    int colorSize = colors.size();
    // Scan the v2vConfig to update the platoonList filed!
    std::map<int, car>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++){

        if(it->second.followingKey == -1){

            std::stringstream platoonList;
            int k = it->first;
            int clusterID = (it->second).id;
            (it->second).isLeader = true;

            // update clusters
            while(k != -1){

                clusters[clusterID].members.push_back(k);
                cars[k].clusterID = clusterID;

                // UPDATING THE TX_MODE for ClusterizeConfigPacket (INFO_MEAPP) message propagation!
                if(!strcmp(preconfiguredTxMode.c_str(), HYBRID_TX_MODE))
                {
                    //compute the optimal mode
                    cars[k].txMode = DOWNLINK_UNICAST_TX_MODE; // for now DOWNLINK_UNICAST by default!
                                                                                                        //TODO: adding the txMode computation! in case of HYBRID TX MODE!
                                                                                                        //compute the best txMode according to some policy --> CQI or TxPower.. for each cluster!
                }else
                {
                    //use pre-configured mode
                    cars[k].txMode = preconfiguredTxMode.c_str();
                }

                platoonList << cars[k].simbolicAddress << " -> ";
                k = cars[k].followerKey;
            }
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
      if(u>=0 && v>=0 && u+v<=1)
      {
          //EV << "inside!";
          return true;
      }
      else{
          //EV << "outside!";
          return false;
      }
}

bool MEPlatooningService::isInRectangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C, inet::Coord D)
{
      return isInTriangle(P, A, B, C) || isInTriangle(P, A, C, D);
      // left-right side          A-------B
      // left-right side          -       -
      // behind side              -       -
      // behind side              D-------C
}


void MEPlatooningService::updatePositions(){

    //updating car positions based on the last position & timestamp + velocity * elapsed_time + acceleration * elapsed_time^2
    EV << "MEPlatooningService::updatePositions\n";

    double now = simTime().dbl();

    std::map<int, car>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++){

        double time_gap = now - it->second.timestamp.dbl();

        it->second.position.x = it->second.position.x + it->second.speed.x*time_gap + it->second.acceleration*cos(it->second.angularPosition.alpha)*time_gap*time_gap;
        it->second.position.y = it->second.position.y + it->second.speed.y*time_gap + it->second.acceleration*sin(it->second.angularPosition.alpha)*time_gap*time_gap;
        //it->second.position.z = it->second.position.z + it->second.speed.z*time_gap;
    }
}

void MEPlatooningService::computePlatoonAccelerations(){

    std::map<int, cluster>::iterator cit;
       for(cit = clusters.begin(); cit != clusters.end(); cit++){

           int previous;
           for( int i : cit->second.members){
               //leader
               if(cars[i].isLeader){

                    double velocity_gap = desiredVelocity - cars[i].speed.length();

                    double acceleration = cars_velocity_controllers[i].getOutput(velocity_gap);
                    cars_velocity_controllers[i].updateNextState(velocity_gap);

                    cit->second.accelerations.push_back(acceleration);

                    EV << "MEPlatooningService::computePlatoonAccelerations - update "<< cars[i].simbolicAddress <<" (LEADER)";
                    EV << "\t position: " << cars[i].position << "\t acceleration: " << acceleration << endl;
               }
               //members
               else{

                   double distance_gap = cars[i].position.distance(cars[previous].position) -  desiredDistance;

                   double acceleration = cars_distance_controllers[i].getOutput(distance_gap);
                   cars_distance_controllers[i].updateNextState(distance_gap);

                   cit->second.accelerations.push_back(acceleration);

                   EV << "MEPlatooningService::computePlatoonAccelerations - update "<< cars[i].simbolicAddress <<" (MEMBER)";
                   EV << "\t position: " << cars[i].position <<  "\t acceleration: " << acceleration << endl;
               }
               previous = i;
           }
       }
}

/*
 * #########################################################################################################################################
 */

void MEPlatooningService::resetClusters(){

    clusters.clear();
}

void MEPlatooningService::resetCarFlagsAndControls(){

    // Scan the cars to update the flags and control fields!
    std::map<int, car>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++){

        it->second.followingKey = -1;
        it->second.followerKey = -1;
        it->second.isFollower = false;
        it->second.isLeader = false;

        it->second.following = "";
        it->second.follower = "";
    }
}

void MEPlatooningService::updateRniInfo(){

    EV << "MEPlatooningService::updateRniInfo\n";

    std::map<int, car>::iterator it;
    for(it = cars.begin(); it != cars.end(); it++){

        double txPower = rni->getUETxPower(it->second.simbolicAddress);     //check if != -1
        it->second.txPower = txPower;

        Cqi cqi = rni->getUEcqi(it->second.simbolicAddress);     //check if != -1
        it->second.cqi = cqi;

        //TESTING
        EV << "MEPlatooningService::updateRniInfo - " << it->second.simbolicAddress << " tx power: " << txPower << endl;
        EV << "MEPlatooningService::updateRniInfo - " << it->second.simbolicAddress << " cqi: " << cqi << endl;
    }
}

void MEPlatooningService::handleClusterizeStop(ClusterizePacket* pkt){

    // erasing controllers map entries
    int key = pkt->getArrivalGate()->getIndex();

    EV << "MEPlatooningService::handleClusterizeStop - Erasing Controller" << endl;

    if(!cars_velocity_controllers.empty() && cars_velocity_controllers.find(key) != cars_velocity_controllers.end()){

        EV << "MEPlatooningService::handleClusterizeStop - Erasing cars_velocity_controllers[" << key << "]" << endl;
        //erasing the map cars_velocity_controllers entry
        //
        std::map<int, SimpleVelocityController>::iterator it;
        it = cars_velocity_controllers.find(key);
        if (it != cars_velocity_controllers.end()){
            cars_velocity_controllers.erase (it);
        }
    }
    else if(!cars_distance_controllers.empty() && cars_distance_controllers.find(key) != cars_distance_controllers.end()){

        EV << "MEPlatooningService::handleClusterizeStop - Erasing cars_distance_controllers[" << key << "]" << endl;
        //erasing the map cars_distance_controllers entry
        //
        std::map<int, SimpleDistanceController>::iterator it;
        it = cars_distance_controllers.find(key);
        if (it != cars_distance_controllers.end()){
            cars_distance_controllers.erase (it);
        }
    }

    MEClusterizeService::handleClusterizeStop(pkt);
}
