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

#include "MEIceAlertService.h"

Define_Module(MEIceAlertService);

void MEIceAlertService::initialize(int stage)
{
    EV << "MEIceAlertService::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    // setting the MEIceAlertService gate sizes
    int maxMEApps = 0;
    cModule* mePlatform = getParentModule();
    if(mePlatform != NULL){
        cModule* meHost = mePlatform->getParentModule();
        if(meHost->hasPar("maxMEApps"))
                maxMEApps = meHost->par("maxMEApps").longValue();
        else
            throw cRuntimeError("MEIceAlertService::initialize - \tFATAL! Error when getting meHost.maxMEApps parameter!");

        this->setGateSize("meAppOut", maxMEApps);
        this->setGateSize("meAppIn", maxMEApps);
    }
    else{
        EV << "MEIceAlertService::initialize - ERROR getting mePlatform cModule!" << endl;
        throw cRuntimeError("MEIceAlertService::initialize - \tFATAL! Error when getting getParentModule()");
    }

    //retrieving parameters
    dangerEdgeA = inet::Coord(par("dangerEdgeAx").doubleValue(), par("dangerEdgeAy").doubleValue(), par("dangerEdgeAz").doubleValue());
    dangerEdgeB = inet::Coord(par("dangerEdgeBx").doubleValue(), par("dangerEdgeBy").doubleValue(), par("dangerEdgeBz").doubleValue());
    dangerEdgeC = inet::Coord(par("dangerEdgeCx").doubleValue(), par("dangerEdgeCy").doubleValue(), par("dangerEdgeCz").doubleValue());
    dangerEdgeD = inet::Coord(par("dangerEdgeDx").doubleValue(), par("dangerEdgeDy").doubleValue(), par("dangerEdgeDz").doubleValue());

    //drawing the Danger Area
    cPolygonFigure *polygon = new cPolygonFigure("polygon");
    std::vector<cFigure::Point> points;
    points.push_back(cFigure::Point(dangerEdgeA.x, dangerEdgeA.y));
    points.push_back(cFigure::Point(dangerEdgeB.x, dangerEdgeB.y));
    points.push_back(cFigure::Point(dangerEdgeC.x, dangerEdgeC.y));
    points.push_back(cFigure::Point(dangerEdgeD.x, dangerEdgeD.y));
    polygon->setPoints(points);
    polygon->setLineColor(cFigure::RED);
    polygon->setLineWidth(2);
    getSimulation()->getSystemModule()->getCanvas()->addFigure(polygon);
}

void MEIceAlertService::handleMessage(cMessage *msg)
{
    EV << "MEIceAlertService::handleMessage - \n";

    IceAlertPacket* pkt = check_and_cast<IceAlertPacket*>(msg);
    if (pkt == 0)
        throw cRuntimeError("MEIceAlertService::handleMessage - \tFATAL! Error when casting to IceAlertPacket");

    if(!strcmp(pkt->getType(), INFO_UEAPP))         handleInfoUEIceAlertApp(pkt);

    delete pkt;
}

void MEIceAlertService::handleInfoUEIceAlertApp(IceAlertPacket* pkt){

    EV << "MEIceAlertService::handleInfoUEIceAlertApp - Received " << pkt->getType() << " type IceAlertPacket from " << pkt->getSourceAddress() << endl;

    inet::Coord uePosition(pkt->getPositionX(), pkt->getPositionY(), pkt->getPositionZ());

    if(isInQuadrilateral(uePosition, dangerEdgeA, dangerEdgeB, dangerEdgeC, dangerEdgeD)){

        IceAlertPacket* packet = new IceAlertPacket();
        packet->setType(INFO_MEAPP);

        send(packet, "meAppOut", pkt->getArrivalGate()->getIndex());

        EV << "MEIceAlertService::handleInfoUEIceAlertApp - "<< pkt->getSourceAddress() << " is in Danger Area! Sending the " << INFO_MEAPP << " type IceAlertPacket!" << endl;
    }
    else
        EV << "MEIceAlertService::handleInfoUEIceAlertApp - "<< pkt->getSourceAddress() << " is not in Danger Area!" << endl;
}

bool MEIceAlertService::isInTriangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C){

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

bool MEIceAlertService::isInQuadrilateral(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C, inet::Coord D)
{
      return isInTriangle(P, A, B, C) || isInTriangle(P, A, C, D);
}
