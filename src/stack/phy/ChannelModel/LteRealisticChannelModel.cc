//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
// 

#include "LteRealisticChannelModel.h"

#include "../../../corenetwork/lteCellInfo/LteCellInfo.h"
#include "stack/phy/packet/LteAirFrame.h"
#include "corenetwork/binder/LteBinder.h"
#include "stack/mac/amc/UserTxParams.h"
#include "common/LteCommon.h"
#include "corenetwork/nodes/ExtCell.h"
#include "stack/phy/layer/LtePhyUe.h"
#include "stack/mac/layer/LteMacEnbD2D.h"

// attenuation value to be returned if max. distance of a scenario has been violated
// and tolerating the maximum distance violation is enabled
#define ATT_MAXDISTVIOLATED 1000

Define_Module(LteRealisticChannelModel);

void LteRealisticChannelModel::initialize()
{
   scenario_ = aToDeploymentScenario(par("scenario").stringValue());
   hNodeB_ = par("nodeb_height");
   carrierFrequency_ = par("carrierFrequency");
   shadowing_ = par("shadowing");
   hBuilding_ = par("building_height");

   tolerateMaxDistViolation_ = par("tolerateMaxDistViolation");
   hUe_ = par("ue_height");

   wStreet_ = par("street_wide");

   correlationDistance_ = par("correlation_distance");
   harqReduction_ = par("harqReduction");

   lambdaMinTh_ = par("lambdaMinTh");
   lambdaMaxTh_ = par("lambdaMaxTh");
   lambdaRatioTh_ = par("lambdaRatioTh");

   antennaGainUe_ = par("antennaGainUe");
   antennaGainEnB_ = par("antennGainEnB");
   antennaGainMicro_ = par("antennGainMicro");
   thermalNoise_ = par("thermalNoise");
   cableLoss_ = par("cable_loss");
   ueNoiseFigure_ = par("ue_noise_figure");
   bsNoiseFigure_ = par("bs_noise_figure");
   useTorus_ = par("useTorus");
   dynamicLos_ = par("dynamic_los");
   fixedLos_ = par("fixed_los");

   fading_ = par("fading");
   std::string fType = par("fading_type");
   if (fType.compare("JAKES") == 0)
       fadingType_ = JAKES;
   else if (fType.compare("RAYLEIGH") == 0)
       fadingType_ = RAYLEIGH;
   else
       fadingType_ = JAKES;

   fadingPaths_ = par("fading_paths");
   enableExtCellInterference_ = par("extCell_interference");
   enableDownlinkInterference_ = par("downlink_interference");
   enableUplinkInterference_ = par("uplink_interference");
   enableD2DInterference_ = par("d2d_interference");
   delayRMS_ = par("delay_rms");

   //get binder
   binder_ = getBinder();
   //clear jakes fading map structure
   jakesFadingMap_.clear();

   // statistics
   rcvdSinr_ = registerSignal("rcvdSinr");
}


double LteRealisticChannelModel::getAttenuation(MacNodeId nodeId, Direction dir,
       Coord coord)
{
   double movement = .0;
   double speed = .0;

   //COMPUTE DISTANCE between ue and eNodeB
   double sqrDistance = phy_->getCoord().distance(coord);

   if (dir == DL) // sender is UE
       speed = computeSpeed(nodeId, phy_->getCoord());
   else
       speed = computeSpeed(nodeId, coord);

   //If traveled distance is greater than correlation distance UE could have changed its state and
   // its visibility from eNodeb, hence it is correct to recompute the los probability
   if (movement > correlationDistance_
           || losMap_.find(nodeId) == losMap_.end())
   {
       computeLosProbability(sqrDistance, nodeId);
   }

   //compute attenuation based on selected scenario and based on LOS or NLOS
   bool los = losMap_[nodeId];
   double dbp = 0;
   double attenuation = computePathLoss(sqrDistance, dbp, los);

   //    Applying shadowing only if it is enabled by configuration
   //    log-normal shadowing
   if (shadowing_)
   {
       double mean = 0;

       //Get std deviation according to los/nlos and selected scenario

       double stdDev = getStdDev(sqrDistance < dbp, nodeId);
       double time = 0;
       double space = 0;
       double att;

       // if direction is DOWNLINK it means that this module is located in UE stack than
       // the Move object associated to the UE is myMove_ varible
       // if direction is UPLINK it means that this module is located in UE stack than
       // the Move object associated to the UE is move varible

       // if shadowing for current user has never been computed
       if (lastComputedSF_.find(nodeId) == lastComputedSF_.end())
       {
           //Get the log normal shadowing with std deviation stdDev
           att = normal(mean, stdDev);

           //store the shadowing attenuation for this user and the temporal mark
           std::pair<simtime_t, double> tmp(NOW, att);
           lastComputedSF_[nodeId] = tmp;

           //If the shadowing attenuation has been computed at least one time for this user
           // and the distance traveled by the UE is greated than correlation distance
       }
       else if ((NOW - lastComputedSF_.at(nodeId).first).dbl() * speed
               > correlationDistance_)
       {

           //get the temporal mark of the last computed shadowing attenuation
           time = (NOW - lastComputedSF_.at(nodeId).first).dbl();

           //compute the traveled distance
           space = time * speed;

           //Compute shadowing with a EAW (Exponential Average Window) (step1)
           double a = exp(-0.5 * (space / correlationDistance_));

           //Get last shadowing attenuation computed
           double old = lastComputedSF_.at(nodeId).second;

           //Compute shadowing with a EAW (Exponential Average Window) (step2)
           att = a * old + sqrt(1 - pow(a, 2)) * normal(mean, stdDev);

           // Store the new computed shadowing
           std::pair<simtime_t, double> tmp(NOW, att);
           lastComputedSF_[nodeId] = tmp;

           // if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remain the same
       }
       else
       {
           att = lastComputedSF_.at(nodeId).second;
       }
       attenuation += att;
   }

   // update current user position

   //if sender is a eNodeB
   if (dir == DL)
       //store the position of user
       updatePositionHistory(nodeId, phy_->getCoord());
   else
       //sender is an UE
       updatePositionHistory(nodeId, coord);

   EV << "LteRealisticChannelModel::getAttenuation - computed attenuation at distance " << sqrDistance << " for eNb is " << attenuation << endl;

   return attenuation;
}

double LteRealisticChannelModel::getAttenuation_D2D(MacNodeId nodeId, Direction dir, Coord coord,MacNodeId node2_Id, Coord coord_2)
{
   double movement = .0;
   double speed = .0;

   //COMPUTE DISTANCE between ue1 and ue2
   //double sqrDistance = phy_->getCoord().distance(coord);
   double sqrDistance = coord.distance(coord_2);
   speed = computeSpeed(nodeId, coord);

   //If traveled distance is greater than correlation distance UE could have changed its state and
   // its visibility from eNodeb, hence it is correct to recompute the los probability
   if (movement > correlationDistance_
       || losMap_.find(nodeId) == losMap_.end())
   {
       computeLosProbability(sqrDistance, nodeId);
   }

   //compute attenuation based on selected scenario and based on LOS or NLOS
   bool los = losMap_[nodeId];
   double dbp = 0;
   double attenuation = computePathLoss(sqrDistance, dbp, los);

   //    Applying shadowing only if it is enabled by configuration
   //    log-normal shadowing
   if (shadowing_)
   {
       double mean = 0;

       //Get std deviation according to los/nlos and selected scenario

       double stdDev = getStdDev(sqrDistance < dbp, nodeId);
       double time = 0;
       double space = 0;
       double att = 0;

       // if direction is DOWNLINK it means that this module is located in UE stack than
       // the Move object associated to the UE is myMove_ varible
       // if direction is UPLINK it means that this module is located in UE stack than
       // the Move object associated to the UE is move varible

       // if shadowing for current user has never been computed
       if (lastComputedSF_.find(nodeId) == lastComputedSF_.end())
       {
           //Get the log normal shadowing with std deviation stdDev
           att = normal(mean, stdDev);

           //store the shadowing attenuation for this user and the temporal mark
           std::pair<simtime_t, double> tmp(NOW, att);
           lastComputedSF_[nodeId] = tmp;

           //If the shadowing attenuation has been computed at least one time for this user
           // and the distance traveled by the UE is greated than correlation distance
       }
       else if ((NOW - lastComputedSF_.at(nodeId).first).dbl() * speed
           > correlationDistance_)
       {
           //get the temporal mark of the last computed shadowing attenuation
           time = (NOW - lastComputedSF_.at(nodeId).first).dbl();

           //compute the traveled distance
           space = time * speed;

           //Compute shadowing with a EAW (Exponential Average Window) (step1)
           double a = exp(-0.5 * (space / correlationDistance_));

           //Get last shadowing attenuation computed
           double old = lastComputedSF_.at(nodeId).second;

           //Compute shadowing with a EAW (Exponential Average Window) (step2)
           att = a * old + sqrt(1 - pow(a, 2)) * normal(mean, stdDev);

           // Store the new computed shadowing
           std::pair<simtime_t, double> tmp(NOW, att);
           lastComputedSF_[nodeId] = tmp;

           // if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remain the same
       }
       else
       {
           att = lastComputedSF_.at(nodeId).second;
       }

       attenuation += att;
   }

   // update current user position
   updatePositionHistory(nodeId, coord);

   EV << "LteRealisticChannelModel::getAttenuation - computed attenuation at distance " << sqrDistance << " for UE2 is " << attenuation << endl;

   return attenuation;
}

void LteRealisticChannelModel::updatePositionHistory(const MacNodeId nodeId,
       const Coord coord)
{
   if (positionHistory_.find(nodeId) != positionHistory_.end())
   {
       // position already updated for this TTI.
       if (positionHistory_[nodeId].back().first == NOW)
           return;
   }

   // FIXME: possible memory leak
   positionHistory_[nodeId].push(Position(NOW, coord));

   if (positionHistory_[nodeId].size() > 2) // if we have more than a past and a current element
       // drop the oldest one
       positionHistory_[nodeId].pop();
}

double LteRealisticChannelModel::computeSpeed(const MacNodeId nodeId,
       const Coord coord)
{
   double speed = 0.0;

   if (positionHistory_.find(nodeId) == positionHistory_.end())
   {
       // no entries
       return speed;
   }
   else
   {
       //compute distance traveled from last update by UE (eNodeB position is fixed)

       if (positionHistory_[nodeId].size() == 1)
       {
           //  the only element refers to present , return 0
           return speed;
       }

       double movement = positionHistory_[nodeId].front().second.distance(coord);

       if (movement <= 0.0)
           return speed;
       else
       {
           double time = (NOW.dbl()) - (positionHistory_[nodeId].front().first.dbl());
           if (time <= 0.0) // time not updated since last speed call
               throw cRuntimeError("Multiple entries detected in position history referring to same time");
           // compute speed
           speed = (movement) / (time);
       }
   }
   return speed;
}

double computeAngle(Coord center, Coord point) {
   double relx, rely, arcoSen, angle, dist;

   // compute distance between points
   dist = point.distance(center);

   // compute distance along the axis
   relx = point.x - center.x;
   rely = point.y - center.y;

   // compute the arc sine
   arcoSen = asin(rely / dist) * 180.0 / M_PI;

   // adjust the angle depending on the quadrants
   if (relx < 0 && rely > 0) // quadrant II
       angle = 180.0 - arcoSen;
   else if (relx < 0 && rely <= 0) // quadrant III
       angle = 180.0 - arcoSen;
   else if (relx > 0 && rely < 0) // quadrant IV
       angle = 360.0 + arcoSen;
   else
       // quadrant I
       angle = arcoSen;

   //    EV << "computeAngle: angle[" << angle <<"] - arcoSen[" << arcoSen <<
   //          "] - relativePos[" << relx << "," << rely <<
   //          "] - siny[" << rely/dist << "] - senx[" << relx/dist <<
   //          "]" << endl;

   return angle;
}

double computeAngolarAttenuation(double angle) {
   double angolarAtt;
   double angolarAttMin = 25;
   // compute attenuation due to angolar position
   // see TR 36.814 V9.0.0 for more details
   angolarAtt = 12 * pow(angle / 70.0, 2);

   //  EV << "\t angolarAtt[" << angolarAtt << "]" << endl;
   // max value for angolar attenuation is 25 dB
   if (angolarAtt > angolarAttMin)
       angolarAtt = angolarAttMin;

   return angolarAtt;
}
std::vector<double> LteRealisticChannelModel::getSINR(LteAirFrame *frame, UserControlInfo* lteInfo)
{
   //get tx power
   double recvPower = lteInfo->getTxPower(); // dBm

   //Get the Resource Blocks used to transmit this packet
   RbMap rbmap = lteInfo->getGrantedBlocks();

   //get move object associated to the packet
   //this object is refereed to eNodeB if direction is DL or UE if direction is UL
   Coord coord = lteInfo->getCoord();

   // position of eNb and UE
   Coord ueCoord;
   Coord enbCoord;

   double antennaGainTx = 0.0;
   double antennaGainRx = 0.0;
   double noiseFigure = 0.0;
   double speed = 0.0;

   // true if we are computing a CQI for the DL direction
   bool cqiDl = false;

   MacNodeId ueId = 0;
   MacNodeId eNbId = 0;

   Direction dir = (Direction) lteInfo->getDirection();

   EV << "------------ GET SINR ----------------" << endl;
   //===================== PARAMETERS SETUP ============================
   /*
    * if direction is DL and this is not a feedback packet,
    * this function has been called by LteRealisticChannelModel::error() in the UE
    *
    *         DownLink error computation
    */
   if (dir == DL && (lteInfo->getFrameType() != FEEDBACKPKT))
   {
       //set noise Figure
       noiseFigure = ueNoiseFigure_; //dB
       //set antenna gain Figure
       antennaGainTx = antennaGainEnB_; //dB
       antennaGainRx = antennaGainUe_;  //dB

       // get MacId for Ue and eNb
       ueId = lteInfo->getDestId();
       eNbId = lteInfo->getSourceId();

       // get position of Ue and eNb
       ueCoord = phy_->getCoord();
       enbCoord = lteInfo->getCoord();

       speed = computeSpeed(ueId, phy_->getCoord());
   }
   /*
    * If direction is UL OR
    * if the packet is a feedback packet
    * it means that this function is called by the feedback computation module
    *
    * located in the eNodeB that compute the feedback received by the UE
    * Hence the UE macNodeId can be taken by the sourceId of the lteInfo
    * and the speed of the UE is contained by the Move object associated to the lteinfo
    */
   else // UL/DL CQI & UL error computation
   {
       // get MacId for Ue and eNb
       ueId = lteInfo->getSourceId();
       eNbId = lteInfo->getDestId();

       if (dir == DL)
       {
           //set noise Figure
           noiseFigure = ueNoiseFigure_; //dB
           //set antenna gain Figure
           antennaGainTx = antennaGainEnB_; //dB
           antennaGainRx = antennaGainUe_;  //dB

           // use the jakes map in the UE side
           cqiDl = true;
       }
       else // if( dir == UL )
       {
           // TODO check if antennaGainEnB should be added in UL direction too
           antennaGainTx = antennaGainUe_;
           antennaGainRx = antennaGainEnB_;
           noiseFigure = bsNoiseFigure_;

           // use the jakes map in the eNb side
           cqiDl = false;
       }
       speed = computeSpeed(ueId, coord);

       // get position of Ue and eNb
       ueCoord = coord;
       enbCoord = phy_->getCoord();
   }

   LteCellInfo* eNbCell = getCellInfo(eNbId);
   const char* eNbTypeString = eNbCell ? (eNbCell->getEnbType() == MACRO_ENB ? "MACRO" : "MICRO") : "NULL";

   EV << "LteRealisticChannelModel::getSINR - srcId=" << lteInfo->getSourceId()
                      << " - destId=" << lteInfo->getDestId()
                      << " - DIR=" << (( dir==DL )?"DL" : "UL")
                      << " - frameType=" << ((lteInfo->getFrameType()==FEEDBACKPKT)?"feedback":"other")
                      << endl
                      << eNbTypeString << " - txPwr " << lteInfo->getTxPower()
                      << " - ueCoord[" << ueCoord << "] - enbCoord[" << enbCoord << "] - ueId[" << ueId << "] - enbId[" << eNbId << "]" <<
                      endl;
   //=================== END PARAMETERS SETUP =======================

   //=============== PATH LOSS + SHADOWING + FADING =================
   EV << "\t using parameters - noiseFigure=" << noiseFigure << " - antennaGainTx=" << antennaGainTx << " - antennaGainRx=" << antennaGainRx <<
           " - txPwr=" << lteInfo->getTxPower() << " - for ueId=" << ueId << endl;

   // attenuation for the desired signal
   double attenuation;
   if ((lteInfo->getFrameType() == FEEDBACKPKT))
       attenuation = getAttenuation(ueId, UL, coord); // dB
   else
       attenuation = getAttenuation(ueId, dir, coord); // dB

   //compute attenuation (PATHLOSS + SHADOWING)
   recvPower -= attenuation; // (dBm-dB)=dBm

   //add antenna gain
   recvPower += antennaGainTx; // (dBm+dB)=dBm
   recvPower += antennaGainRx; // (dBm+dB)=dBm

   //sub cable loss
   recvPower -= cableLoss_; // (dBm-dB)=dBm

   //=============== ANGOLAR ATTENUATION =================
   if (dir == DL)
   {
       //get tx angle
       omnetpp::cModule* eNbModule = getSimulation()->getModule(binder_->getOmnetId(eNbId));
       LtePhyBase* ltePhy = eNbModule ?
          check_and_cast<LtePhyBase*>(eNbModule->getSubmodule("lteNic")->getSubmodule("phy")) :
          nullptr;

       if (ltePhy && ltePhy->getTxDirection() == ANISOTROPIC)
       {
           // get tx angle
           double txAngle = ltePhy->getTxAngle();

           // compute the angle between uePosition and reference axis, considering the eNb as center
           double ueAngle = computeAngle(enbCoord, ueCoord);

           // compute the reception angle between ue and eNb
           double recvAngle = fabs(txAngle - ueAngle);

           if (recvAngle > 180)
               recvAngle = 360 - recvAngle;

           // compute attenuation due to sectorial tx
           double angolarAtt = computeAngolarAttenuation(recvAngle);

           recvPower -= angolarAtt;
       }
       // else, antenna is omni-directional
   }
   //=============== END ANGOLAR ATTENUATION =================

   std::vector<double> snrVector;
   snrVector.resize(band_, 0.0);

   // compute and add interference due to fading
   // Apply fading for each band
   // if the phy layer is localized we can assume that for each logical band we have different fading attenuation
   // if the phy layer is distributed the number of logical band should be set to 1
   double fadingAttenuation = 0;

   // for each logical band
   // FIXME compute fading only for used RBs
   for (unsigned int i = 0; i < band_; i++)
   {
       fadingAttenuation = 0;
       //if fading is enabled
       if (fading_)
       {
           //Appling fading
           if (fadingType_ == RAYLEIGH)
               fadingAttenuation = rayleighFading(ueId, i);

           else if (fadingType_ == JAKES)
               fadingAttenuation = jakesFading(ueId, speed, i, cqiDl);
       }
       // add fading contribution to the received pwr
       double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

       //if txmode is multi user the tx power is dived by the number of paired user
       // in db divede by 2 means -3db
       if (lteInfo->getTxMode() == MULTI_USER)
       {
           finalRecvPower -= 3;
       }

       EV << " LteRealisticChannelModel::getSINR node " << ueId
          << ((lteInfo->getFrameType() == FEEDBACKPKT) ?
           " FEEDBACK PACKET " : " NORMAL PACKET ")
          << " band " << i << " recvPower " << recvPower
          << " direction " << dirToA(dir) << " antenna gain tx "
          << antennaGainTx << " antenna gain rx " << antennaGainRx
          << " noise figure " << noiseFigure
          << " cable loss   " << cableLoss_
          << " attenuation (pathloss + shadowing) " << attenuation
          << " speed " << speed << " thermal noise " << thermalNoise_
          << " fading attenuation " << fadingAttenuation << endl;

       snrVector[i] = finalRecvPower;
   }
   //============ END PATH LOSS + SHADOWING + FADING ===============

   /*
    * The SINR will be calculated as follows
    *
    *              Pwr
    * SINR = ---------
    *           N  +  I
    *
    * Ndb = thermalNoise_ + noiseFigure (measured in decibel)
    * I = extCellInterference + multiCellInterference
    */

   //============ MULTI CELL INTERFERENCE COMPUTATION =================
   //vector containing the sum of multicell interference for each band
   std::vector<double> multiCellInterference; // Linear value (mW)
   // prepare data structure
   multiCellInterference.resize(band_, 0);
   if (enableDownlinkInterference_ && dir == DL)
   {
       computeDownlinkInterference(eNbId, ueId, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), rbmap, &multiCellInterference);
   }
   else if (enableUplinkInterference_ && dir == UL)
   {
       computeUplinkInterference(eNbId, ueId, (lteInfo->getFrameType() == FEEDBACKPKT), rbmap, &multiCellInterference);
   }

   //============ EXTCELL INTERFERENCE COMPUTATION =================
   //vector containing the sum of ext-cell interference for each band
   std::vector<double> extCellInterference; // Linear value (mW)
   // prepare data structure
   extCellInterference.resize(band_, 0);
   if (enableExtCellInterference_ && dir == DL)
   {
       computeExtCellInterference(eNbId, ueId, ueCoord, (lteInfo->getFrameType() == FEEDBACKPKT), &extCellInterference); // dBm
   }

   //===================== SINR COMPUTATION ========================
   // compute and linearize total noise
   double totN = dBmToLinear(thermalNoise_ + noiseFigure);

   // denominator expressed in dBm as (N+extCell+multiCell)
   double den;
   EV << "LteRealisticChannelModel::getSINR - distance from my eNb=" << enbCoord.distance(ueCoord) << " - DIR=" << (( dir==DL )?"DL" : "UL") << endl;

   // add interference for each band
   for (unsigned int i = 0; i < band_; i++)
   {
       // if we are decoding a data transmission and this RB has not been used, skip it
       // TODO fix for multi-antenna case
       if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
           continue;

       //               (      mW            +  mW  +        mW            )
       den = linearToDBm(extCellInterference[i] + totN + multiCellInterference[i]);

       EV << "\t ext[" << extCellInterference[i] << "] - multi[" << multiCellInterference[i] << "] - recvPwr["
          << dBmToLinear(snrVector[i]) << "] - sinr[" << snrVector[i]-den << "]\n";

       // compute final SINR
       snrVector[i] -= den;
   }

   //if sender is an eNodeB
   if (dir == DL)
       //store the position of user
       updatePositionHistory(ueId, phy_->getCoord());
   //sender is an UE
   else
       updatePositionHistory(ueId, coord);
   return snrVector;
}

std::vector<double> LteRealisticChannelModel::getRSRP_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord)
{
   AttenuationVector::iterator it;
   // Get Tx power
   double recvPower = lteInfo_1->getD2dTxPower(); // dBm

   // Coordinate of the Sender of the Feedback packet
   Coord sourceCoord =  lteInfo_1->getCoord();

   double antennaGainTx = 0.0;
   double antennaGainRx = 0.0;
   double noiseFigure = 0.0;
   double speed = 0.0;
   // Get MacId for Ue and his peer
   MacNodeId sourceId = lteInfo_1->getSourceId();
   std::vector<double> rsrpVector;

   // True if we use the jakes map in the UE side (D2D is like DL for the receivers)
   bool cqiDl = false;
   // Get the direction
   Direction dir = (Direction) lteInfo_1->getDirection();
   dir = D2D;

   EV << "------------ GET RSRP D2D----------------" << endl;

   //===================== PARAMETERS SETUP ============================

   // D2D CQI or D2D error computation

   if( dir == UL || dir==DL)
   {
       //consistency check
        throw cRuntimeError("Direction should neither be UL or DL");
   }
   else
   {
       antennaGainTx = antennaGainRx = antennaGainUe_;
       //In D2D case the noise figure is the ueNoiseFigure_
       noiseFigure = ueNoiseFigure_;
       // use the jakes map in the UE side
       cqiDl = true;
   }
   // Compute speed
   speed = computeSpeed(sourceId, sourceCoord);

   EV << "LteRealisticChannelModel::getRSRP_D2D - srcId=" << sourceId
      << " - destId=" << destId
      << " - DIR=" << dirToA(dir)
      << " - frameType=" << ((lteInfo_1->getFrameType()==FEEDBACKPKT)?"feedback":"other")
      << endl
      << " - txPwr " << recvPower
      << " - ue1_Coord[" << sourceCoord << "] - ue2_Coord[" << destCoord << "] - ue1_Id[" << sourceId << "] - ue2_Id[" << destId << "]" <<
   endl;
   //=================== END PARAMETERS SETUP =======================

   //=============== PATH LOSS + SHADOWING + FADING =================
   EV << "\t using parameters - noiseFigure=" << noiseFigure << " - antennaGainTx=" << antennaGainTx << " - antennaGainRx=" << antennaGainRx <<
   " - txPwr=" << recvPower << " - for ueId=" << sourceId << endl;

   // attenuation for the desired signal
   double attenuation = getAttenuation_D2D(sourceId, dir, sourceCoord, destId, destCoord); // dB

   //compute attenuation (PATHLOSS + SHADOWING)
   recvPower -= attenuation; // (dBm-dB)=dBm

   //add antenna gain
   recvPower += antennaGainTx; // (dBm+dB)=dBm
   recvPower += antennaGainRx; // (dBm+dB)=dBm

   //sub cable loss
   recvPower -= cableLoss_; // (dBm-dB)=dBm

   // compute and add interference due to fading
   // Apply fading for each band
   // if the phy layer is localized we can assume that for each logical band we have different fading attenuation
   // if the phy layer is distributed the number of logical band should be set to 1
   double fadingAttenuation = 0;
   //for each logical band
   for (unsigned int i = 0; i < band_; i++)
   {
       fadingAttenuation = 0;
       //if fading is enabled
       if (fading_)
       {
           //Appling fading
           if (fadingType_ == RAYLEIGH)
               fadingAttenuation = rayleighFading(sourceId, i);

           else if (fadingType_ == JAKES)
           {
               fadingAttenuation = jakesFading(sourceId, speed, i, cqiDl);
           }
       }
       // add fading contribution to the received pwr
       double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

       //if txmode is multi user the tx power is dived by the number of paired user
       // in db divede by 2 means -3db
       if (lteInfo_1->getTxMode() == MULTI_USER)
       {
           finalRecvPower -= 3;
       }

       EV << " LteRealisticChannelModel::getRSRP_D2D node " << sourceId
          << ((lteInfo_1->getFrameType() == FEEDBACKPKT) ?
           " FEEDBACK PACKET " : " NORMAL PACKET ")
          << " band " << i << " recvPower " << recvPower
          << " direction " << dirToA(dir) << " antenna gain tx "
          << antennaGainTx << " antenna gain rx " << antennaGainRx
          << " noise figure " << noiseFigure
          << " cable loss   " << cableLoss_
          << " attenuation (pathloss + shadowing) " << attenuation
          << " speed " << speed << " thermal noise " << thermalNoise_
          << " fading attenuation " << fadingAttenuation << endl;

       // Store the calculated receive power
       rsrpVector.push_back(finalRecvPower);
   }
   //============ END PATH LOSS + SHADOWING + FADING ===============

   return rsrpVector;
}

std::vector<double> LteRealisticChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo, MacNodeId destId, Coord destCoord, MacNodeId enbId)
{
   AttenuationVector::iterator it;
   // Get Tx power
   double recvPower = lteInfo->getD2dTxPower(); // dBm

   // Get allocated RBs
   RbMap rbmap = lteInfo->getGrantedBlocks();

   // Coordinate of the Sender of the Feedback packet
   Coord sourceCoord =  lteInfo->getCoord();

   double antennaGainTx = 0.0;
   double antennaGainRx = 0.0;
   double noiseFigure = 0.0;
   double speed = 0.0;
   double extCellInterference = 0;
   // Get MacId for Ue and his peer
   MacNodeId sourceId = lteInfo->getSourceId();
   std::vector<double> snrVector;
   snrVector.resize(band_, 0.0);

   // True if we use the jakes map in the UE side (D2D is like DL for the receivers)
   bool cqiDl = true;
   // Get the direction
   Direction dir = D2D;

   EV << "------------ GET SINR D2D ----------------" << endl;

   //===================== PARAMETERS SETUP ============================

   // antenna gain is antennaGainUe for both tx and rx
   antennaGainTx = antennaGainRx = antennaGainUe_;
   // In D2D case the noise figure is the ueNoiseFigure_
   noiseFigure = ueNoiseFigure_;

   // Compute speed
   speed = computeSpeed(sourceId, sourceCoord);


   EV << "LteRealisticChannelModel::getSINR_d2d - srcId=" << sourceId
      << " - destId=" << destId
      << " - DIR=" << dirToA(dir)
      << " - frameType=" << ((lteInfo->getFrameType()==FEEDBACKPKT)?"feedback":"other")
      << endl
      << " - txPwr " << recvPower
      << " - ue1_Coord[" << sourceCoord << "] - ue2_Coord[" << destCoord << "] - ue1_Id[" << sourceId << "] - ue2_Id[" << destId << "]" <<
   endl;
   //=================== END PARAMETERS SETUP =======================

   //=============== PATH LOSS + SHADOWING + FADING =================
   EV << "\t using parameters - noiseFigure=" << noiseFigure << " - antennaGainTx=" << antennaGainTx << " - antennaGainRx=" << antennaGainRx <<
   " - txPwr=" << recvPower << " - for ueId=" << sourceId << endl;

   // attenuation for the desired signal
   double attenuation = getAttenuation_D2D(sourceId, dir, sourceCoord, destId, destCoord); // dB

   //compute attenuation (PATHLOSS + SHADOWING)
   recvPower -= attenuation; // (dBm-dB)=dBm

   //add antenna gain
   recvPower += antennaGainTx; // (dBm+dB)=dBm
   recvPower += antennaGainRx; // (dBm+dB)=dBm

   //sub cable loss
   recvPower -= cableLoss_; // (dBm-dB)=dBm

   // compute and add interference due to fading
   // Apply fading for each band
   // if the phy layer is localized we can assume that for each logical band we have different fading attenuation
   // if the phy layer is distributed the number of logical band should be set to 1
   double fadingAttenuation = 0;
   //for each logical band
   for (unsigned int i = 0; i < band_; i++)
   {
       fadingAttenuation = 0;
       //if fading is enabled
       if (fading_)
       {
           //Appling fading
           if (fadingType_ == RAYLEIGH)
               fadingAttenuation = rayleighFading(sourceId, i);

           else if (fadingType_ == JAKES)
           {
               fadingAttenuation = jakesFading(sourceId, speed, i, cqiDl);
           }
       }
       // add fading contribution to the received pwr
       double finalRecvPower = recvPower + fadingAttenuation; // (dBm+dB)=dBm

       //if txmode is multi user the tx power is dived by the number of paired user
       // in db divede by 2 means -3db
       if (lteInfo->getTxMode() == MULTI_USER)
       {
           finalRecvPower -= 3;
       }

       EV << " LteRealisticChannelModel::getSINR_d2d node " << sourceId
          << ((lteInfo->getFrameType() == FEEDBACKPKT) ?
           " FEEDBACK PACKET " : " NORMAL PACKET ")
          << " band " << i << " recvPower " << recvPower
          << " direction " << dirToA(dir) << " antenna gain tx "
          << antennaGainTx << " antenna gain rx " << antennaGainRx
          << " noise figure " << noiseFigure
          << " cable loss   " << cableLoss_
          << " attenuation (pathloss + shadowing) " << attenuation
          << " speed " << speed << " thermal noise " << thermalNoise_
          << " fading attenuation " << fadingAttenuation << endl;

       // Store the calculated receive power
       snrVector[i] = finalRecvPower;
   }
   //============ END PATH LOSS + SHADOWING + FADING ===============

   /*
    * The SINR will be calculated as follows
    *
    *              Pwr
    * SINR = ---------
    *           N  +  I
    *
    * N = thermalNoise_ + noiseFigure (measured in dBm)
    * I = extCellInterference + inCellInterference (measured in mW)
    */
   //============ D2D INTERFERENCE COMPUTATION =================
   /*
    * In calculating a D2D CQI the Interference from others UEs discriminates between calculating a CQI
    * following direction D2D_Tx--->D2D_Rx or D2D_Tx<---D2D_Rx (This happens due to the different positions of the
    * interfering UEs relative to the position of the UE for whom we are calculating the CQI). We need that the CQI
    * for the D2D_Tx is the same of the D2D_Rx(This is an help for the simulator because when the eNodeB allocates
    * resources to a D2D_Tx it must refer to quality channel of the D2D_Rx).
    * To do so here we must check if the ueId is the ID of the D2D_Tx:if it
    * is so we swap the ueId with the one of his Peer(D2D_Rx). We do the same for the coord.
    */
   //vector containing the sum of inCell interference for each band
   std::vector<double> d2dInterference; // Linear value (mW)
   // prepare data structure
   d2dInterference.resize(band_, 0);
   if (enableD2DInterference_)
   {
       computeD2DInterference(enbId, sourceId, sourceCoord, destId, destCoord, (lteInfo->getFrameType() == FEEDBACKPKT), rbmap, &d2dInterference,dir);
   }

   //===================== SINR COMPUTATION ========================
   if( enableD2DInterference_)
   {
           // compute and linearize total noise
           double totN = dBmToLinear(thermalNoise_ + noiseFigure);

           // denominator expressed in dBm as (N+extCell+inCell)
           double den;
           EV << "LteRealisticChannelModel::getSINR - distance from my Peer = " << destCoord.distance(sourceCoord) << " - DIR=" << dirToA(dir)  << endl;

           // Add interference for each band
           for (unsigned int i = 0; i < band_; i++)
           {
               // if we are decoding a data transmission and this RB has not been used, skip it
               // TODO fix for multi-antenna case
               if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
                   continue;

               //               (      mW            +  mW  +        mW            )
               den = linearToDBm(extCellInterference + totN + d2dInterference[i]);

               EV << "\t ext[" << extCellInterference << "] - in[" << d2dInterference[i] << "] - recvPwr["
                       << dBmToLinear(snrVector[i]) << "] - sinr[" << snrVector[i]-den << "]\n";

               // compute final SINR. Subtraction in dB is equivalent to linear division
               snrVector[i] -= den;
           }
   }
   // compute snr with no D2D interference
   else
   {
       for (unsigned int i = 0; i < band_; i++)
       {
           // if we are decoding a data transmission and this RB has not been used, skip it
           // TODO fix for multi-antenna case
           if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
               continue;

           /*
           std::cout<<"SNR "<<i<<" "<<snrVector[i]<<endl;
           std::cout<<"noise figure "<<i<<" "<<noiseFigure<<endl;
           std::cout<<"Termal noise "<<i<<" "<<thermalNoise_<<endl;
           */
           // compute final SINR
           snrVector[i] -=  (noiseFigure + thermalNoise_);

           EV << "LteRealisticChannelModel::getSINR_d2d - distance from my Peer = " << destCoord.distance(sourceCoord) << " - DIR=" << dirToA(dir) << " - snr[" << snrVector[i] << "]\n";
       }
   }
   //sender is an UE
   updatePositionHistory(sourceId, sourceCoord);

   return snrVector;
}

std::vector<double> LteRealisticChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord,MacNodeId enbId,const std::vector<double>& rsrpVector)
{
   std::vector<double> snrVector = rsrpVector;

   MacNodeId sourceId = lteInfo_1->getSourceId();
   Coord sourceCoord = lteInfo_1->getCoord();

   // Get allocated RBs
   RbMap rbmap = lteInfo_1->getGrantedBlocks();

   // Get the direction
   Direction dir = D2D;

   double noiseFigure = 0.0;
   double extCellInterference = 0.0;

   //In D2D case the noise figure is the ueNoiseFigure_
   noiseFigure = ueNoiseFigure_;


   EV << "------------ GET SINR D2D----------------" << endl;

   /*
    * The SINR will be calculated as follows
    *
    *              Pwr
    * SINR = ---------
    *           N  +  I
    *
    * N = thermalNoise_ + noiseFigure (measured in dBm)
    * I = extCellInterference + inCellInterference (measured in mW)
    */
   //============ IN CELL D2D INTERFERENCE COMPUTATION =================
   /*
    * In calculating a D2D CQI the Interference from others D2D UEs discriminates between calculating a CQI
    * following direction D2D_Tx--->D2D_Rx or D2D_Tx<---D2D_Rx (This happens due to the different positions of the
    * interfering UEs relative to the position of the UE for whom we are calculating the CQI). We need that the CQI
    * for the D2D_Tx is the same of the D2D_Rx(This is an help for the simulator because when the eNodeB allocates
    * resources to a D2D_Tx it must refer to quality channel of the D2D_Rx).
    * To do so here we must check if the ueId is the ID of the D2D_Tx:if it
    * is so we swap the ueId with the one of his Peer(D2D_Rx). We do the same for the coord.
    */
   //vector containing the sum of inCell interference for each band
   std::vector<double> d2dInterference; // Linear value (mW)
   // prepare data structure
   d2dInterference.resize(band_, 0);
   if (enableD2DInterference_)
   {
       computeD2DInterference(enbId, sourceId, sourceCoord, destId, destCoord, (lteInfo_1->getFrameType() == FEEDBACKPKT), rbmap, &d2dInterference,dir);
   }

   //===================== SINR COMPUTATION ========================
   if (enableD2DInterference_)
   {
           // compute and linearize total noise
           double totN = dBmToLinear(thermalNoise_ + noiseFigure);

           // denominator expressed in dBm as (N+extCell+inCell)
           double den;
           EV << "LteRealisticChannelModel::getSINR - distance from my Peer = " << destCoord.distance(sourceCoord) << " - DIR=" << dirToA(dir)  << endl;

           // Add interference for each band
           for (unsigned int i = 0; i < band_; i++)
           {
               // if we are decoding a data transmission and this RB has not been used, skip it
               // TODO fix for multi-antenna case
               if (lteInfo_1->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
                   continue;

               //               (      mW            +  mW  +        mW            )
               den = linearToDBm(extCellInterference + totN + d2dInterference[i]);

               EV << "\t ext[" << extCellInterference << "] - in[" << d2dInterference[i] << "] - recvPwr["
                       << dBmToLinear(snrVector[i]) << "] - sinr[" << snrVector[i]-den << "]\n";

               // compute final SINR. Subtraction in dB is equivalent to linear division
               snrVector[i] -= den;
           }
   }
   // compute snr with no D2D interference
   else
   {
       for (unsigned int i = 0; i < band_; i++)
       {
           // if we are decoding a data transmission and this RB has not been used, skip it
           // TODO fix for multi-antenna case
           if (lteInfo_1->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
               continue;

           // compute final SINR
           snrVector[i] -=  (noiseFigure + thermalNoise_);

           EV << "LteRealisticChannelModel::getSINR_D2D - distance from my Peer = " << destCoord.distance(sourceCoord) << " - DIR=" << dirToA(dir) << " - snr[" << snrVector[i] << "]\n";
       }
   }

   //sender is a UE
   updatePositionHistory(sourceId, sourceCoord);

   return snrVector;
}


std::vector<double> LteRealisticChannelModel::getSIR(LteAirFrame *frame,
       UserControlInfo* lteInfo)
{
   AttenuationVector::iterator it;
   //get tx power
   double recvPower = lteInfo->getTxPower();

   Coord coord = lteInfo->getCoord();

   Direction dir = (Direction) lteInfo->getDirection();

   MacNodeId id = 0;
   double speed = 0.0;

   //if direction is DL
   if (dir == DL && (lteInfo->getFrameType() != FEEDBACKPKT))
   {
       id = lteInfo->getDestId();
       speed = computeSpeed(id, phy_->getCoord());
   }

   /*
    * If direction is UL OR
    * if the packet is a feedback packet
    * it means that this function is called by the feedback computation module
    * located in the eNodeB that compute the feedback received by the UE
    * Hence the UE macNodeId can be taken by the sourceId of the lteInfo
    * and the speed of the UE is contained by the Move object associated to the lteinfo     */
   else
   {
       id = lteInfo->getSourceId();
       speed = computeSpeed(id, coord);
   }

   //Apply fading for each band
   //if the phy layer is localized we can assume that for each logical band we have different fading attenuation
   //if the phy layer is distributed the number of logical band should be set to 1
   std::vector<double> snrVector;

   double fadingAttenuation = 0;
   //for each logical band
   for (unsigned int i = 0; i < band_; i++)
   {
       fadingAttenuation = 0;
       //if fading is enabled
       if (fading_)
       {
           //Applying fading
           if (fadingType_ == RAYLEIGH)
           {
               fadingAttenuation = rayleighFading(id, i);
           }
           else if (fadingType_ == JAKES)
           {
               fadingAttenuation = jakesFading(id, speed, i, dir);
           }
       }
       // add fading contribution to the final Sinr
       double finalSnr = recvPower + fadingAttenuation;

       //if txmode is multi user the tx power is dived by the number of paired user
       // using db, to divide by 2 means -3db
       if (lteInfo->getTxMode() == MULTI_USER)
           finalSnr -= 3;

       snrVector.push_back(finalSnr);
   }

   //if sender is a eNodeB
   if (dir == DL)
       //store the position of user
       updatePositionHistory(id, phy_->getCoord());
   //sender is an UE
   else
       updatePositionHistory(id, coord);
   return snrVector;
}

double LteRealisticChannelModel::rayleighFading(MacNodeId id,
       unsigned int band)
{
   //get raylegh variable from trace file
   double temp1 = binder_->phyPisaData.getChannel(
           getCellInfo(id)->getLambda(id)->channelIndex + band);
   return linearToDb(temp1);
}

double LteRealisticChannelModel::jakesFading(MacNodeId nodeId, double speed,
       unsigned int band, bool cqiDl)
{
   /**
    * NOTE: there are two different jakes map. One on the Ue side and one on the eNb side, with different values.
    *
    * eNb side => used for CQI computation and for error-probability evaluation in UL
    * UE side  => used for error-probability evaluation in DL
    *
    * the one within eNb is referred to the UL direction
    * the one within UE is referred to the DL direction
    *
    * thus the actual map should be choosen carefully (i.e. just check the cqiDL flag)
    */
   JakesFadingMap * actualJakesMap;

   if (cqiDl) // if we are computing a DL CQI we need the Jakes Map stored on the UE side
       actualJakesMap = obtainUeJakesMap(nodeId);

   else
       actualJakesMap = &jakesFadingMap_;

   //if this is the first time that we compute fading for current user
   if (actualJakesMap->find(nodeId) == actualJakesMap->end())
   {
       //clear the map
       // FIXME: possible memory leak
       (*actualJakesMap)[nodeId].clear();

       //for each band we are going to create a jakes fading
       for (unsigned int j = 0; j < band_; j++)
       {
           //clear some structure
           JakesFadingData temp;
           temp.angleOfArrival.clear();
           temp.delaySpread.clear();

           //for each fading path
           for (int i = 0; i < fadingPaths_; i++)
           {
               //get angle of arrivals
               temp.angleOfArrival.push_back(cos(uniform(0, M_PI)));

               //get delay spread
               temp.delaySpread.push_back(exponential(delayRMS_));
           }
           //store the jakes fadint for this user
           (*actualJakesMap)[nodeId].push_back(temp);
       }
   }
   // convert carrier frequency from GHz to Hz
   double f = carrierFrequency_ * 1000000000;

   //get transmission time start (TTI =1ms)
   simtime_t t = simTime().dbl() - 0.001;

   double re_h = 0;
   double im_h = 0;

   // Compute Doppler shift.
   double doppler_shift = (speed * f) / SPEED_OF_LIGHT;

   for (int i = 0; i < fadingPaths_; i++)
   {
       // Phase shift due to Doppler => t-selectivity.
       double phi_d = actualJakesMap->at(nodeId).at(band).angleOfArrival[i] * doppler_shift;

       // Phase shift due to delay spread => f-selectivity.
       double phi_i = actualJakesMap->at(nodeId).at(band).delaySpread[i].dbl() * f;

       // Calculate resulting phase due to t-selective and f-selective fading.
       double phi = 2.00 * M_PI * (phi_d * t.dbl() - phi_i);

       // One ring model/Clarke's model plus f-selectivity according to Cavers:
       // Due to isotropic antenna gain pattern on all paths only a^2 can be received on all paths.
       // Since we are interested in attenuation a:=1, attenuation per path is then:
       double attenuation = (1.00 / sqrt(static_cast<double>(fadingPaths_)));

       // Convert to cartesian form and aggregate {Re, Im} over all fading paths.
       re_h = re_h + attenuation * cos(phi);
       im_h = im_h - attenuation * sin(phi);

       //        EV << "ID=" << nodeId << " - t[" << t << "] - dopplerShift[" << doppler_shift << "] - phiD[" <<
       //                phi_d << "] - phiI[" << phi_i << "] - phi[" << phi << "] - attenuation[" << attenuation << "] - f["
       //                << f << "] - Band[" << band << "] - cos(phi)["
       //                << cos(phi) << "]" << endl;
   }

   // Output: |H_f|^2 = absolute channel impulse response due to fading.
   // Note that this may be >1 due to constructive interference.
   return linearToDb(re_h * re_h + im_h * im_h);
}

bool LteRealisticChannelModel::isCorrupted(LteAirFrame *frame,
       UserControlInfo* lteInfo)
{
   EV << "LteRealisticChannelModel::error" << endl;

   //get codeword
   unsigned char cw = lteInfo->getCw();
   //get number of codeword
   int size = lteInfo->getUserTxParams()->readCqiVector().size();

   //get position associated to the packet
   Coord coord = lteInfo->getCoord();

   //if total number of codeword is equal to 1 the cw index should be only 0
   if (size == 1)
       cw = 0;

   //get cqi used to transmit this cw
   Cqi cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];

   MacNodeId id;
   Direction dir = (Direction) lteInfo->getDirection();

   //Get MacNodeId of UE
   if (dir == DL)
       id = lteInfo->getDestId();
   else
       id = lteInfo->getSourceId();

   // Get Number of RTX
   unsigned char nTx = lteInfo->getTxNumber();

   //consistency check
   if (nTx == 0)
       throw cRuntimeError("Transmissions counter should not be 0");

   //Get txmode
   TxMode txmode = (TxMode) lteInfo->getTxMode();

   // If rank is 1 and we used SMUX to transmit we have to corrupt this packet
   if (txmode == CL_SPATIAL_MULTIPLEXING
           || txmode == OL_SPATIAL_MULTIPLEXING)
   {
       //compare lambda min (smaller eingenvalues of channel matrix) with the threshold used to compute the rank
       if (binder_->phyPisaData.getLambda(id, 1) < lambdaMinTh_)
           return false;
   }

   // Take sinr
   std::vector<double> snrV;
   if (lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI)
   {
       MacNodeId destId = lteInfo->getDestId();
       Coord destCoord = phy_->getCoord();
       MacNodeId enbId = binder_->getNextHop(lteInfo->getSourceId());
       snrV = getSINR_D2D(frame,lteInfo,destId,destCoord,enbId);
   }
   else
   {
       snrV = getSINR(frame, lteInfo);
   }

   //Get the resource Block id used to transmist this packet
   RbMap rbmap = lteInfo->getGrantedBlocks();

   //Get txmode
   unsigned int itxmode = txModeToIndex[txmode];

   double bler = 0;
   std::vector<double> totalbler;
   double finalSuccess = 1;
   RbMap::iterator it;
   std::map<Band, unsigned int>::iterator jt;

   // for statistic purposes
   double sumSnr = 0.0;
   int usedRBs = 0;

   //for each Remote unit used to transmit the packet
   for (it = rbmap.begin(); it != rbmap.end(); ++it)
   {
       //for each logical band used to transmit the packet
       for (jt = it->second.begin(); jt != it->second.end(); ++jt)
       {
           //this Rb is not allocated
           if (jt->second == 0)
               continue;

           //check the antenna used in Das
           if ((lteInfo->getTxMode() == CL_SPATIAL_MULTIPLEXING
                   || lteInfo->getTxMode() == OL_SPATIAL_MULTIPLEXING)
                   && rbmap.size() > 1)
               //we consider only the snr associated to the LB used
               if (it->first != lteInfo->getCw())
                   continue;

           //Get the Bler
           if (cqi == 0 || cqi > 15)
               throw cRuntimeError("A packet has been transmitted with a cqi equal to 0 or greater than 15 cqi:%d txmode:%d dir:%d rb:%d cw:%d rtx:%d", cqi,lteInfo->getTxMode(),dir,jt->second,cw,nTx);

           // for statistic purposes
           sumSnr += snrV[jt->first];
           usedRBs++;

           int snr = snrV[jt->first];//XXX because jt->first is a Band (=unsigned short)
           if (snr < 0)
               return false;
           else if (snr > binder_->phyPisaData.maxSnr())
               bler = 0;
           else
               bler = binder_->phyPisaData.getBler(itxmode, cqi - 1, snr);

           EV << "\t bler computation: [itxMode=" << itxmode << "] - [cqi-1=" << cqi-1
                   << "] - [snr=" << snr << "]" << endl;

           double success = 1 - bler;
           //compute the success probability according to the number of RB used
           double successPacket = pow(success, (double)jt->second);
           // compute the success probability according to the number of LB used
           finalSuccess *= successPacket;

           EV << " LteRealisticChannelModel::error direction " << dirToA(dir)
                              << " node " << id << " remote unit " << dasToA((*it).first)
                              << " Band " << (*jt).first << " SNR " << snr << " CQI " << cqi
                              << " BLER " << bler << " success probability " << successPacket
                              << " total success probability " << finalSuccess << endl;
       }
   }
   //Compute total error probability
   double per = 1 - finalSuccess;
   //Harq Reduction
   double totalPer = per * pow(harqReduction_, nTx - 1);

   double er = uniform(0.0, 1.0);

   EV << " LteRealisticChannelModel::error direction " << dirToA(dir)
                      << " node " << id << " total ERROR probability  " << per
                      << " per with H-ARQ error reduction " << totalPer
                      << " - CQI[" << cqi << "]- random error extracted[" << er << "]" << endl;

   // emit SINR statistic
   emit(rcvdSinr_, sumSnr / usedRBs);

   if (er <= totalPer)
   {
       EV << "This is NOT your lucky day (" << er << " < " << totalPer
               << ") -> do not receive." << endl;
       // Signal too weak, we can't receive it
       return false;
   }
   // Signal is strong enough, receive this Signal
   EV << "This is your lucky day (" << er << " > " << totalPer
           << ") -> Receive AirFrame." << endl;
   return true;
}

bool LteRealisticChannelModel::error_D2D(LteAirFrame *frame, UserControlInfo* lteInfo, const std::vector<double>& rsrpVector)
{
   EV << "LteRealisticChannelModel::error_D2D" << endl;

   //get codeword
   unsigned char cw = lteInfo->getCw();
   //get number of codeword
   int size = lteInfo->getUserTxParams()->readCqiVector().size();

   //get position associated to the packet
   Coord coord = lteInfo->getCoord();

   //if total number of codeword is equal to 1 the cw index should be only 0
   if (size == 1)
       cw = 0;

   // Get CQI used to transmit this cw
   Cqi cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];
   EV << "LteRealisticChannelModel:: CQI: "<< cqi << endl;

   MacNodeId id;
   Direction dir = (Direction) lteInfo->getDirection();

   // Get MacNodeId of UE
   if (dir == DL)
       id = lteInfo->getDestId();
   else // UL or D2D
       id = lteInfo->getSourceId();

   EV<<NOW<< "LteRealisticChannelModel::FROM: "<< id << endl;
   // Get Number of RTX
   unsigned char nTx = lteInfo->getTxNumber();

   //consistency check
   if (nTx == 0)
       throw cRuntimeError("Transmissions counter should not be 0");

   //Get txmode
   TxMode txmode = (TxMode) lteInfo->getTxMode();

   // If rank is 1 and we used SMUX to transmit we have to corrupt this packet
   if (txmode == CL_SPATIAL_MULTIPLEXING
       || txmode == OL_SPATIAL_MULTIPLEXING)
   {
       //compare lambda min (smaller eingenvalues of channel matrix) with the threshold used to compute the rank
       if (binder_->phyPisaData.getLambda(id, 1) < lambdaMinTh_)
           return false;
   }
   // SINR vector(one SINR value for each band)
   std::vector<double> snrV;
   if (lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI)
   {
       MacNodeId peerUeMacNodeId = lteInfo->getDestId();
       Coord peerCoord = phy_->getCoord();
       // TODO get an appropriate way to get EnbId
       MacNodeId enbId = 1;

       if (lteInfo->getDirection() == D2D)
       {
           snrV = getSINR_D2D(frame,lteInfo,peerUeMacNodeId,peerCoord,enbId);
       }
       else  // D2D_MULTI
       {
           snrV = getSINR_D2D(frame,lteInfo,peerUeMacNodeId,peerCoord,enbId,rsrpVector);
       }
   }
   //ROSSALI-------END------------------------------------------------
   else  snrV = getSINR(frame, lteInfo); // Take SINR

   //Get the resource Block id used to transmit this packet
   RbMap rbmap = lteInfo->getGrantedBlocks();

   //Get txmode
   unsigned int itxmode = txModeToIndex[txmode];

   double bler = 0;
   std::vector<double> totalbler;
   double finalSuccess = 1;
   RbMap::iterator it;
   std::map<Band, unsigned int>::iterator jt;

   //for each Remote unit used to transmit the packet
   for (it = rbmap.begin(); it != rbmap.end(); ++it)
   {
       //for each logical band used to transmit the packet
       for (jt = it->second.begin(); jt != it->second.end(); ++jt)
       {
           //this Rb is not allocated
           if (jt->second == 0) continue;

           //check the antenna used in Das
           if ((lteInfo->getTxMode() == CL_SPATIAL_MULTIPLEXING
                   || lteInfo->getTxMode() == OL_SPATIAL_MULTIPLEXING)
               && rbmap.size() > 1)
           //we consider only the snr associated to the LB used
           if (it->first != lteInfo->getCw()) continue;

           //Get the Bler
           if (cqi == 0 || cqi > 15)
               throw cRuntimeError("A packet has been transmitted with a cqi equal to 0 or greater than 15 cqi:%d txmode:%d dir:%d rb:%d cw:%d rtx:%d", cqi,lteInfo->getTxMode(),dir,jt->second,cw,nTx);
           int snr = snrV[jt->first];//XXX because jt->first is a Band (=unsigned short)
           if (snr < 1)   // XXX it was < 0
               return false;
           else if (snr > binder_->phyPisaData.maxSnr())
                   bler = 0;
               else
                   bler = binder_->phyPisaData.getBler(itxmode, cqi - 1, snr);

           EV << "\t bler computation: [itxMode=" << itxmode << "] - [cqi-1=" << cqi-1
              << "] - [snr=" << snr << "]" << endl;

           double success = 1 - bler;
           //compute the success probability according to the number of RB used
           double successPacket = pow(success, (double)jt->second);

           // compute the success probability according to the number of LB used
           finalSuccess *= successPacket;

           EV << " LteRealisticChannelModel::error direction " << dirToA(dir)
              << " node " << id << " remote unit " << dasToA((*it).first)
              << " Band " << (*jt).first << " SNR " << snr << " CQI " << cqi
              << " BLER " << bler << " success probability " << successPacket
              << " total success probability " << finalSuccess << endl;
       }
   }
   // Compute total error probability
   double per = 1 - finalSuccess;
   // Harq Reduction
   double totalPer = per * pow(harqReduction_, nTx - 1);

   double er = uniform(0.0, 1.0);

   EV << " LteRealisticChannelModel::error direction " << dirToA(dir)
      << " node " << id << " total ERROR probability  " << per
      << " per with H-ARQ error reduction " << totalPer
      << " - CQI[" << cqi << "]- random error extracted[" << er << "]" << endl;
   if (er <= totalPer)
   {
       EV << "This is NOT your lucky day (" << er << " < " << totalPer << ") -> do not receive." << endl;

       // Signal too weak, we can't receive it
       return false;
   }
   // Signal is strong enough, receive this Signal
   EV << "This is your lucky day (" << er << " > " << totalPer << ") -> Receive AirFrame." << endl;

   return true;
}

void LteRealisticChannelModel::computeLosProbability(double d,
       MacNodeId nodeId)
{
   double p = 0;
   if (!dynamicLos_)
   {
       losMap_[nodeId] = fixedLos_;
       return;
   }
   switch (scenario_)
   {
   case INDOOR_HOTSPOT:
       if (d < 18)
           p = 1;
       else if (d >= 37)
           p = 0.5;
       else
           p = exp((-1) * ((d - 18) / 27));
       break;
   case URBAN_MICROCELL:
       p = (((18 / d) > 1) ? 1 : 18 / d) * (1 - exp(-1 * d / 36))
       + exp(-1 * d / 36);
       break;
   case URBAN_MACROCELL:
       p = (((18 / d) > 1) ? 1 : 18 / d) * (1 - exp(-1 * d / 36))
       + exp(-1 * d / 36);
       break;
   case RURAL_MACROCELL:
       if (d <= 10)
           p = 1;
       else
           p = exp(-1 * (d - 10) / 200);
       break;
   case SUBURBAN_MACROCELL:
       if (d <= 10)
           p = 1;
       else
           p = exp(-1 * (d - 10) / 1000);
       break;
   default:
       throw cRuntimeError("Wrong path-loss scenario value %d", scenario_);
   }
   double random = uniform(0.0, 1.0);
   if (random <= p)
       losMap_[nodeId] = true;
   else
       losMap_[nodeId] = false;
}

double LteRealisticChannelModel::computePathLoss(double distance, double dbp, bool los)
{
    //compute attenuation based on selected scenario and based on LOS or NLOS
    double pathLoss = 0;
    switch (scenario_)
    {
    case INDOOR_HOTSPOT:
        pathLoss = computeIndoor(distance, los);
        break;
    case URBAN_MICROCELL:
        pathLoss = computeUrbanMicro(distance, los);
        break;
    case URBAN_MACROCELL:
        pathLoss = computeUrbanMacro(distance, los);
        break;
    case RURAL_MACROCELL:
        pathLoss = computeRuralMacro(distance, dbp, los);
        break;
    case SUBURBAN_MACROCELL:
        pathLoss = computeSubUrbanMacro(distance, dbp, los);
        break;
    default:
        throw cRuntimeError("Wrong value %d for path-loss scenario", scenario_);
    }
    return pathLoss;
}

double LteRealisticChannelModel::computeIndoor(double d, bool los)
{
   double a, b;
   if (los)
   {
       if (d > 150 || d < 3)
           throw cRuntimeError("Error LOS indoor path loss model is valid for 3<d<150");
       a = 16.9;
       b = 32.8;
   }
   else
   {
       if (d > 250 || d < 6)
           throw cRuntimeError("Error NLOS indoor path loss model is valid for 6<d<250");
       a = 43.3;
       b = 11.5;
   }
   return a * log10(d) + b + 20 * log10(carrierFrequency_);
}

double LteRealisticChannelModel::computeUrbanMicro(double d, bool los)
{
   if (d < 10)
       d = 10;

   double dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
                       * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);
   if (los)
   {
       // LOS situation
       if (d > 5000){
           if(tolerateMaxDistViolation_)
               return ATT_MAXDISTVIOLATED;
           else
               throw cRuntimeError("Error LOS urban microcell path loss model is valid for d<5000 m");
       }
       if (d < dbp)
           return 22 * log10(d) + 28 + 20 * log10(carrierFrequency_);
       else
           return 40 * log10(d) + 7.8 - 18 * log10(hNodeB_ - 1)
       - 18 * log10(hUe_ - 1) + 2 * log10(carrierFrequency_);
   }
   // NLOS situation
   if (d < 10)
       throw cRuntimeError("Error NLOS urban microcell path loss model is valid for 10m < d ");
   if (d > 5000){
       if(tolerateMaxDistViolation_)
           return ATT_MAXDISTVIOLATED;
       else
           throw cRuntimeError("Error NLOS urban microcell path loss model is valid for d <2000 m");
   }
   return 36.7 * log10(d) + 22.7 + 26 * log10(carrierFrequency_);
}

double LteRealisticChannelModel::computeUrbanMacro(double d, bool los)
{
   if (d < 10)
       d = 10;

   double dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
                       * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);
   if (los)
   {
       if (d > 5000){
           if(tolerateMaxDistViolation_)
               return ATT_MAXDISTVIOLATED;
           else
               throw cRuntimeError("Error LOS urban macrocell path loss model is valid for d<5000 m");
       }
       if (d < dbp)
           return 22 * log10(d) + 28 + 20 * log10(carrierFrequency_);
       else
           return 40 * log10(d) + 7.8 - 18 * log10(hNodeB_ - 1)
       - 18 * log10(hUe_ - 1) + 2 * log10(carrierFrequency_);
   }

   if (d < 10)
       throw cRuntimeError("Error NLOS urban macrocell path loss model is valid for 10m < d ");
   if (d > 5000){
       if(tolerateMaxDistViolation_)
           return ATT_MAXDISTVIOLATED;
       else
           throw cRuntimeError("Error NLOS urban macrocell path loss model is valid for d <5000 m");
   }

   double att = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
   - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
   + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d) - 3)
   + 20 * log10(carrierFrequency_)
   - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
   return att;
}

double LteRealisticChannelModel::computeSubUrbanMacro(double d, double& dbp, bool los)
{
   if (d < 10)
       d = 10;

   dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
                       * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);
   if (los)
   {
       if (d > 5000) {
           if(tolerateMaxDistViolation_)
               return ATT_MAXDISTVIOLATED;
           else
               throw cRuntimeError("Error LOS suburban macrocell path loss model is valid for d<5000 m");
       }
       double a1 = (0.03 * pow(hBuilding_, 1.72));
       double b1 = 0.044 * pow(hBuilding_, 1.72);
       double a = (a1 < 10) ? a1 : 10;
       double b = (b1 < 14.72) ? b1 : 14.72;
       if (d < dbp)
       {
           double primo = 20 * log10((40 * M_PI * d * carrierFrequency_) / 3);
           double secondo = a * log10(d);
           double quarto = 0.002 * log10(hBuilding_) * d;
           return primo + secondo - b + quarto;
       }
       else
           return 20 * log10((40 * M_PI * dbp * carrierFrequency_) / 3)
       + a * log10(dbp) - b + 0.002 * log10(hBuilding_) * dbp
       + 40 * log10(d / dbp);
   }
   if (d > 5000) {
       if(tolerateMaxDistViolation_)
           return ATT_MAXDISTVIOLATED;
       else
           throw cRuntimeError("Error NLOS suburban macrocell path loss model is valid for 10 < d < 5000 m");
   }
   double att = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
   - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
   + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d) - 3)
   + 20 * log10(carrierFrequency_)
   - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
   return att;
}

double LteRealisticChannelModel::computeRuralMacro(double d, double& dbp, bool los)
{
   if (d < 10)
       d = 10;

   dbp = 4 * (hNodeB_ - 1) * (hUe_ - 1)
                       * ((carrierFrequency_ * 1000000000) / SPEED_OF_LIGHT);
   if (los)
   {
       // LOS situation
       if (d > 10000) {
           if(tolerateMaxDistViolation_)
               return ATT_MAXDISTVIOLATED;
           else
               throw cRuntimeError("Error LOS rural macrocell path loss model is valid for d < 10000 m");
       }

       double a1 = (0.03 * pow(hBuilding_, 1.72));
       double b1 = 0.044 * pow(hBuilding_, 1.72);
       double a = (a1 < 10) ? a1 : 10;
       double b = (b1 < 14.72) ? b1 : 14.72;
       if (d < dbp)
           return 20 * log10((40 * M_PI * d * carrierFrequency_) / 3)
       + a * log10(d) - b + 0.002 * log10(hBuilding_) * d;
       else
           return 20 * log10((40 * M_PI * dbp * carrierFrequency_) / 3)
       + a * log10(dbp) - b + 0.002 * log10(hBuilding_) * dbp
       + 40 * log10(d / dbp);
   }
   // NLOS situation
   if (d > 5000) {
       if(tolerateMaxDistViolation_)
           return ATT_MAXDISTVIOLATED;
       else
           throw cRuntimeError("Error NLOS rural macrocell path loss model is valid for d<5000 m");
   }

   double att = 161.04 - 7.1 * log10(wStreet_) + 7.5 * log10(hBuilding_)
   - (24.37 - 3.7 * pow(hBuilding_ / hNodeB_, 2)) * log10(hNodeB_)
   + (43.42 - 3.1 * log10(hNodeB_)) * (log10(d) - 3)
   + 20 * log10(carrierFrequency_)
   - (3.2 * (pow(log10(11.75 * hUe_), 2)) - 4.97);
   return att;
}

double LteRealisticChannelModel::getStdDev(bool dist, MacNodeId nodeId)
{
   switch (scenario_)
   {
   case URBAN_MICROCELL:
   case INDOOR_HOTSPOT:
       if (losMap_[nodeId])
           return 3.;
       else
           return 4.;
       break;
   case URBAN_MACROCELL:
       if (losMap_[nodeId])
           return 4.;
       else
           return 6.;
       break;
   case RURAL_MACROCELL:
   case SUBURBAN_MACROCELL:
       if (losMap_[nodeId])
       {
           if (dist)
               return 4.;
           else
               return 6.;
       }
       else
           return 8.;
       break;
   default:
       throw cRuntimeError("Wrong path-loss scenario value %d", scenario_);
   }
   return 0.0;
}

bool LteRealisticChannelModel::computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, Coord coord, bool isCqi,
       std::vector<double>* interference)
{
   EV << "**** Ext Cell Interference **** " << endl;

   // get external cell list
   ExtCellList list = binder_->getExtCellList();
   ExtCellList::iterator it = list.begin();

   Coord c;
   double dist, // meters
   recvPwr, // watt
   recvPwrDBm, // dBm
   att, // dBm
   angolarAtt; // dBm

   //compute distance for each cell
   while (it != list.end())
   {
       // get external cell position
       c = (*it)->getPosition();
       // computer distance between UE and the ext cell
       dist = coord.distance(c);

       EV << "\t distance between UE[" << coord.x << "," << coord.y <<
               "] and extCell[" << c.x << "," << c.y << "] is -> "
               << dist << "\t";

       // compute attenuation according to some path loss model
       att = computeExtCellPathLoss(dist, nodeId);

       //=============== ANGOLAR ATTENUATION =================
       if ((*it)->getTxDirection() == OMNI)
       {
           angolarAtt = 0;
       }
       else
       {
           // compute the angle between uePosition and reference axis, considering the eNb as center
           double ueAngle = computeAngle(c, coord);

           // compute the reception angle between ue and eNb
           double recvAngle = fabs((*it)->getTxAngle() - ueAngle);

           if (recvAngle > 180)
               recvAngle = 360 - recvAngle;

           // compute attenuation due to sectorial tx
           angolarAtt = computeAngolarAttenuation(recvAngle);
       }
       //=============== END ANGOLAR ATTENUATION =================

       // TODO do we need to use (- cableLoss_ + antennaGainEnB_) in ext cells too?
       // compute and linearize received power
       recvPwrDBm = (*it)->getTxPower() - att - angolarAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;
       recvPwr = dBmToLinear(recvPwrDBm);

       // add interference in those bands where the ext cell is active
       for (unsigned int i = 0; i < band_; i++) {
           int occ;
           if (isCqi)  // check slot occupation for this TTI
           {
               occ = (*it)->getBandStatus(i);
           }
           else        // error computation. We need to check the slot occupation of the previous TTI
           {
               occ = (*it)->getPrevBandStatus(i);
           }

           // if the ext cell is active, add interference
           if (occ)
           {
               (*interference)[i] += recvPwr;
           }
       }

       it++;
   }

   return true;
}

double LteRealisticChannelModel::computeExtCellPathLoss(double dist, MacNodeId nodeId)
{
   double movement = .0;
   double speed = .0;

   speed = computeSpeed(nodeId, phy_->getCoord());

   //    EV << "LteRealisticChannelModel::computeExtCellPathLoss:" << scenario_ << "-" << shadowing_ << "\n";

   //compute attenuation based on selected scenario and based on LOS or NLOS
   bool los = losMap_[nodeId];
   double dbp = 0;
   double attenuation = computePathLoss(dist, dbp, los);

   //TODO Apply shadowing to each interfering extCell signal

   //    Applying shadowing only if it is enabled by configuration
   //    log-normal shadowing
   if (shadowing_)
   {
       double mean = 0;

       //        Get std deviation according to los/nlos and selected scenario

       //        double stdDev = getStdDev(dist < dbp, nodeId);
       double time = 0;
       double space = 0;
       double att;
       //
       //
       //        //If the shadowing attenuation has been computed at least one time for this user
       //        // and the distance traveled by the UE is greated than correlation distance
       //        if ((NOW - lastComputedSF_.at(nodeId).first).dbl() * speed > correlationDistance_)
       //        {
       //            //get the temporal mark of the last computed shadowing attenuation
       //            time = (NOW - lastComputedSF_.at(nodeId).first).dbl();
       //
       //            //compute the traveled distance
       //            space = time * speed;
       //
       //            //Compute shadowing with a EAW (Exponential Average Window) (step1)
       //            double a = exp(-0.5 * (space / correlationDistance_));
       //
       //            //Get last shadowing attenuation computed
       //            double old = lastComputedSF_.at(nodeId).second;
       //
       //            //Compute shadowing with a EAW (Exponential Average Window) (step2)
       //            att = a * old + sqrt(1 - pow(a, 2)) * normal(getEnvir()->getRNG(0), mean, stdDev);
       //        }
       //         if the distance traveled by the UE is smaller than correlation distance shadowing attenuation remain the same
       //        else
       {
           att = lastComputedSF_.at(nodeId).second;
       }
       EV << "(" << att << ")";
       attenuation += att;
   }

   return attenuation;
}

LteRealisticChannelModel::JakesFadingMap * LteRealisticChannelModel::obtainUeJakesMap(MacNodeId id)
{
   // obtain a reference to UE phy
   LtePhyBase * ltePhy = check_and_cast<LtePhyBase*>(
           getSimulation()->getModule(binder_->getOmnetId(id))->getSubmodule("lteNic")->getSubmodule("phy"));

   // get the associated channel and get a reference to its Jakes Map
   LteRealisticChannelModel * re = dynamic_cast<LteRealisticChannelModel *>(ltePhy->getChannelModel());
   JakesFadingMap * j = re->getJakesMap();

   return j;
}

bool LteRealisticChannelModel::computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, Coord coord, bool isCqi, const RbMap& rbmap,
       std::vector<double> * interference)
{
   EV << "**** Downlink Interference ****" << endl;

   // reference to the mac/phy/channel of each cell
   LtePhyBase * ltePhy;

   int temp;
   double att;

   double txPwr;

   std::vector<EnbInfo*> * enbList = binder_->getEnbList();
   std::vector<EnbInfo*>::iterator it = enbList->begin(), et = enbList->end();

   while(it!=et)
   {
       MacNodeId id = (*it)->id;

       if (id == eNbId)
       {
           ++it;
           continue;
       }

       // initialize eNb data structures
       if(!(*it)->init)
       {
           // obtain a reference to enb phy and obtain tx power
           ltePhy = check_and_cast<LtePhyBase*>(getSimulation()->getModule(binder_->getOmnetId(id))->getSubmodule("lteNic")->getSubmodule("phy"));
           (*it)->txPwr = ltePhy->getTxPwr();//dBm

           // get tx direction
           (*it)->txDirection = ltePhy->getTxDirection();

           // get tx angle
           (*it)->txAngle = ltePhy->getTxAngle();

           // get real Channel
           (*it)->realChan = dynamic_cast<LteRealisticChannelModel *>(ltePhy->getChannelModel());

           //get reference to mac layer
           (*it)->mac = check_and_cast<LteMacEnb*>(getMacByMacNodeId(id));

           (*it)->init = true;
       }

       // compute attenuation using data structures within the cell
       att = (*it)->realChan->getAttenuation(ueId,UL,coord);
       EV << "EnbId [" << id << "] - attenuation [" << att << "]" << endl;

       //=============== ANGOLAR ATTENUATION =================
       double angolarAtt = 0;
       if ((*it)->txDirection == ANISOTROPIC)
       {
           //get tx angle
           double txAngle = (*it)->txAngle;

           // compute the angle between uePosition and reference axis, considering the eNb as center
           double ueAngle = computeAngle((*it)->realChan->phy_->getCoord(), coord);

           // compute the reception angle between ue and eNb
           double recvAngle = fabs(txAngle - ueAngle);
           if (recvAngle > 180)
               recvAngle = 360 - recvAngle;

           // compute attenuation due to sectorial tx
           angolarAtt = computeAngolarAttenuation(recvAngle);

       }
       // else, antenna is omni-directional
       //=============== END ANGOLAR ATTENUATION =================

       txPwr = (*it)->txPwr - angolarAtt - cableLoss_ + antennaGainEnB_ + antennaGainUe_;

       if(isCqi)// check slot occupation for this TTI
       {
           for(unsigned int i=0;i<band_;i++)
           {
               // compute the number of occupied slot (unnecessary)
               temp = (*it)->mac->getDlBandStatus(i);
               if(temp!=0)
                   (*interference)[i] += dBmToLinear(txPwr-att);//(dBm-dB)=dBm

               EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << (*interference)[i] << "]" << endl;
           }
       }
       else // error computation. We need to check the slot occupation of the previous TTI
       {
           for(unsigned int i=0;i<band_;i++)
           {
               // if we are decoding a data transmission and this RB has not been used, skip it
               // TODO fix for multi-antenna case
               if (rbmap.at(MACRO).at(i) == 0)
                   continue;

               // compute the number of occupied slot (unnecessary)
               temp = (*it)->mac->getDlPrevBandStatus(i);
               if(temp!=0)
                   (*interference)[i] += dBmToLinear(txPwr-att);//(dBm-dB)=dBm

               EV << "\t band " << i << " occupied " << temp << "/pwr[" << txPwr << "]-int[" << (*interference)[i] << "]" << endl;
           }
       }
       ++it;
   }

   return true;
}

bool LteRealisticChannelModel::computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, bool isCqi, const RbMap& rbmap, std::vector<double> * interference)
{
   EV << "**** Uplink Interference for cellId[" << eNbId << "] node["<<senderId<<"] ****" << endl;


   const std::vector<UeAllocationInfo>* allocatedUes;
   std::vector<UeAllocationInfo>::const_iterator ue_it, ue_et;

   if(isCqi)// check slot occupation for this TTI
   {
       for(unsigned int i=0;i<band_;i++)
       {
           // compute the number of occupied slot (unnecessary)
           allocatedUes = binder_->getUlTransmissionMap(CURR_TTI, i);
           if (allocatedUes->empty()) // no UEs allocated on this band
               continue;

           ue_it = allocatedUes->begin(), ue_et = allocatedUes->end();
           for (; ue_it != ue_et; ++ue_it)
           {
               MacNodeId ueId = ue_it->nodeId;
               MacCellId cellId = ue_it->cellId;
               LtePhyUe* uePhy = check_and_cast<LtePhyUe*>(ue_it->phy);
               Direction dir = ue_it->dir;

               // no self interference
               if (ueId == senderId)
                   continue;

               // no interference from UL/D2D connections of the same cell  (no D2D-UL reuse allowed)
               if (cellId == eNbId)
                   continue;

               EV<<NOW<<" LteRealisticChannelModel::computeUplinkInterference - Interference from UE: "<< ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

               // get tx power and attenuation from this UE
               double txPwr = uePhy->getTxPwr(dir) - cableLoss_ + antennaGainUe_ + antennaGainEnB_;
               double att = getAttenuation(ueId, UL, uePhy->getCoord());
               (*interference)[i] += dBmToLinear(txPwr-att);//(dBm-dB)=dBm

               EV << "\t band " << i << "/pwr[" << txPwr-att << "]-int[" << (*interference)[i] << "]" << endl;
           }
       }
   }
   else // Error computation. We need to check the slot occupation of the previous TTI
   {
       // For each band we have to check if the Band in the previous TTI was occupied by the interferringId
       for(unsigned int i=0;i<band_;i++)
       {
           // if we are decoding a data transmission and this RB has not been used, skip it
           // TODO fix for multi-antenna case
           if (rbmap.at(MACRO).at(i) == 0)
               continue;

           allocatedUes = binder_->getUlTransmissionMap(PREV_TTI, i);
           if (allocatedUes->empty()) // no UEs allocated on this band
               continue;

           ue_it = allocatedUes->begin(), ue_et = allocatedUes->end();
           for (; ue_it != ue_et; ++ue_it)
           {
               MacNodeId ueId = ue_it->nodeId;
               MacCellId cellId = ue_it->cellId;
               LtePhyUe* uePhy = check_and_cast<LtePhyUe*>(ue_it->phy);
               Direction dir = ue_it->dir;

               // no self interference
               if (ueId == senderId)
                   continue;

               // no interference from UL connections of the same cell (no D2D-UL reuse allowed)
               if (cellId == eNbId)
                   continue;

               EV<<NOW<<" LteRealisticChannelModel::computeUplinkInterference - Interference from UE: "<< ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

               // get tx power and attenuation from this UE
               double txPwr = uePhy->getTxPwr(dir) - cableLoss_ + antennaGainUe_ + antennaGainEnB_;
               double att = getAttenuation(ueId, UL, uePhy->getCoord());
               (*interference)[i] += dBmToLinear(txPwr-att);//(dBm-dB)=dBm

               EV << "\t band " << i << "/pwr[" << txPwr-att << "]-int[" << (*interference)[i] << "]" << endl;
           }
       }
   }

   // Debug Output
   EV << NOW << " LteRealisticChannelModel::computeUplinkInterference - Final Band Interference Status: "<<endl;
   for(unsigned int i=0;i<band_;i++)
       EV << "\t band " << i << " int[" << (*interference)[i] << "]" << endl;

   return true;
}

bool LteRealisticChannelModel::computeD2DInterference(MacNodeId eNbId, MacNodeId senderId, Coord senderCoord, MacNodeId destId, Coord destCoord, bool isCqi, const RbMap& rbmap,
   std::vector<double> * interference,Direction dir)
{
   EV << "**** D2D Interference for cellId[" << eNbId << "] node["<<destId<<"] ****" << endl;

   // get the reference to the MAC of the eNodeB
   LteMacEnbD2D* macEnb = check_and_cast<LteMacEnbD2D*>(binder_->getMacFromMacNodeId(eNbId));

   const std::vector<UeAllocationInfo>* allocatedUes;
   std::vector<UeAllocationInfo>::const_iterator ue_it, ue_et;

   if(isCqi)// check slot occupation for this TTI
   {
       for(unsigned int i=0;i<band_;i++)
       {
           // compute the number of occupied slot (unnecessary)
           allocatedUes = binder_->getUlTransmissionMap(CURR_TTI, i);
           if (allocatedUes->empty()) // no UEs allocated on this band
               continue;

           ue_it = allocatedUes->begin(), ue_et = allocatedUes->end();
           for (; ue_it != ue_et; ++ue_it)
           {
               MacNodeId ueId = ue_it->nodeId;
               MacCellId cellId = ue_it->cellId;
               LtePhyUe* uePhy = check_and_cast<LtePhyUe*>(ue_it->phy);
               Direction dir = ue_it->dir;

               // no self interference
               if (ueId == senderId || ueId == destId)
                   continue;

               // no interference from UL connections of the same cell (no D2D-UL reuse allowed)
               if (dir == UL && cellId == eNbId)
                   continue;

               // no interference from D2D connections of the same cell when reuse is disabled (otherwise, computation of CQI is misleading)
               if (cellId == eNbId && (!macEnb->isReuseD2DEnabled() && !macEnb->isReuseD2DMultiEnabled()))
                   continue;

               EV<<NOW<<" LteRealisticChannelModel::computeD2DInterference - Interference from UE: "<< ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

               // get tx power and attenuation from this UE
               double txPwr = uePhy->getTxPwr(dir) - cableLoss_ + 2 * antennaGainUe_;
               double att = getAttenuation_D2D(ueId, D2D, uePhy->getCoord(), destId, destCoord);
               (*interference)[i] += dBmToLinear(txPwr-att);//(dBm-dB)=dBm

               EV << "\t band " << i << "/pwr[" << txPwr-att << "]-int[" << (*interference)[i] << "]" << endl;
           }
       }
   }
   else // Error computation. We need to check the slot occupation of the previous TTI
   {
       // For each band we have to check if the Band in the previous TTI was occupied by the interferringId
       for(unsigned int i=0;i<band_;i++)
       {
//           // if we are decoding a data transmission and this RB has not been used, skip it
//           // TODO fix for multi-antenna case
//           if (rbmap.at(MACRO).at(i) == 0)
//               continue;

           allocatedUes = binder_->getUlTransmissionMap(PREV_TTI, i);
           if (allocatedUes->empty()) // no UEs allocated on this band
               continue;

           ue_it = allocatedUes->begin(), ue_et = allocatedUes->end();
           for (; ue_it != ue_et; ++ue_it)
           {
               MacNodeId ueId = ue_it->nodeId;
               MacCellId cellId = ue_it->cellId;
               LtePhyUe* uePhy = check_and_cast<LtePhyUe*>(ue_it->phy);
               Direction dir = ue_it->dir;

               // no self interference
               if (ueId == senderId || ueId == destId)
                   continue;

               // no interference from UL connections of the same cell (no D2D-UL reuse allowed)
               if (dir == UL && cellId == eNbId)
                   continue;

               // no interference from D2D connections of the same cell when reuse is disabled
               if (cellId == eNbId && (!macEnb->isReuseD2DEnabled() && !macEnb->isReuseD2DMultiEnabled()))
                   continue;

               EV<<NOW<<" LteRealisticChannelModel::computeD2DInterference - Interference from UE: "<< ueId << "(dir " << dirToA(dir) << ") on band[" << i << "]" << endl;

               // get tx power and attenuation from this UE
               double txPwr = uePhy->getTxPwr(dir) - cableLoss_ + 2 * antennaGainUe_;
               double att = getAttenuation_D2D(ueId, D2D, uePhy->getCoord(), destId, destCoord);
               (*interference)[i] += dBmToLinear(txPwr-att);//(dBm-dB)=dBm

               EV << "\t band " << i << "/pwr[" << txPwr-att << "]-int[" << (*interference)[i] << "]" << endl;
           }
       }
   }

   // Debug Output
   EV << NOW << " LteRealisticChannelModel::computeD2DInterference - Final Band Interference Status: "<<endl;
   for(unsigned int i=0;i<band_;i++)
       EV << "\t band " << i << " int[" << (*interference)[i] << "]" << endl;

   return true;
}


void LteRealisticChannelModel::setBand( unsigned int band ) { band_ = band ;}
void LteRealisticChannelModel::setPhy( LtePhyBase * phy ) { phy_ = phy ; }
