//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef STACK_PHY_CHANNELMODEL_LTEREALISTICCHANNELMODEL_H_
#define STACK_PHY_CHANNELMODEL_LTEREALISTICCHANNELMODEL_H_

#include "stack/phy/ChannelModel/LteChannelModel.h"

class LteBinder;

class LteRealisticChannelModel : public LteChannelModel
{
private:

  // Carrier Frequency
  double carrierFrequency_;

  // Information needed about the playground
  bool useTorus_;

  // eNodeB Height
  double hNodeB_;

  // UE Height
  double hUe_;

  // average Building Heights
  double hBuilding_;

  // Average street's wide
  double wStreet_;

  // enable/disable the shadowing
  bool shadowing_;

  // enable/disable intercell interference computation
  bool enableExtCellInterference_;
  bool enableDownlinkInterference_;
  bool enableUplinkInterference_;
  bool enableD2DInterference_;

  typedef std::pair<simtime_t, inet::Coord> Position;

  // last position of current user
  std::map<MacNodeId, std::queue<Position> > positionHistory_;

  // scenario
  DeploymentScenario scenario_;

  // map that stores for each user if is in Line of Sight or not with eNodeB
  std::map<MacNodeId, bool> losMap_;

  // Store the last computed shadowing for each user
  std::map<MacNodeId, std::pair<simtime_t, double> > lastComputedSF_;

  //correlation distance used in shadowing computation and
  //also used to recompute the probability of LOS
  double correlationDistance_;

  //percentage of error probability reduction for each h-arq retransmission
  double harqReduction_;

  // eigen values of channel matrix
  //used to compute the rank
  double lambdaMinTh_;
  double lambdaMaxTh_;
  double lambdaRatioTh_;

  //Antenna gain of eNodeB
  double antennaGainEnB_;

  //Antenna gain of micro node
  double antennaGainMicro_;

  //Antenna gain of UE
  double antennaGainUe_;

  //Thermal noise
  double thermalNoise_;

  //pointer to Binder module
  LteBinder* binder_;

  //Cable loss
  double cableLoss_;

  //UE noise figure
  double ueNoiseFigure_;

  //eNodeB noise figure
  double bsNoiseFigure_;

  //Enabale disable fading
  bool fading_;

  //Number of fading paths in jakes fading
  int fadingPaths_;

  //avg delay spred in jakes fading
  double delayRMS_;

  bool tolerateMaxDistViolation_;

  //Struct used to store information about jakes fading
  struct JakesFadingData
  {
      std::vector<double> angleOfArrival;
      std::vector<simtime_t> delaySpread;
  };

  // for each node and for each band we store information about jakes fading
  std::map<MacNodeId, std::vector<JakesFadingData> > jakesFadingMap_;

  typedef std::vector<JakesFadingData> JakesFadingVector;
  typedef std::map<MacNodeId, JakesFadingVector> JakesFadingMap;

  enum FadingType
  {
      RAYLEIGH, JAKES
  };

  //Fading type (JAKES or RAYLEIGH)
  FadingType fadingType_;

  //enable or disable the dynamic computation of LOS NLOS probability for each user
  bool dynamicLos_;

  //if dynamicLos is false this boolean is initialized to true if all user will be in LOS or false otherwise
  bool fixedLos_;

  // statistics
  simsignal_t rcvdSinr_;


public:
  virtual void initialize();

  virtual void setBand( unsigned int band );
  virtual void setPhy( LtePhyBase * phy );

  /*
   * Compute Attenuation caused by pathloss and shadowing (optional)
   *
   * @param nodeid mac node id of UE
   * @param dir traffic direction
   * @param coord position of end point comunication (if dir==UL is the position of UE else is the position of eNodeB)
   */
  virtual double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord);
  /*
   * Compute Attenuation for D2D caused by pathloss and shadowing (optional)
   *
   * @param nodeid mac node id of UE
   * @param dir traffic direction
   * @param coord position of end point comunication (if dir==UL is the position of UE else is the position of eNodeB)
   */
  virtual double getAttenuation_D2D(MacNodeId nodeId, Direction dir, inet::Coord coord,MacNodeId node2_Id, inet::Coord coord_2);
  /*
   * Compute sir for each band for user nodeId according to multipath fading
   *
   * @param frame pointer to the packet
   * @param lteinfo pointer to the user control info
   */
  virtual std::vector<double> getSIR(LteAirFrame *frame, UserControlInfo* lteInfo);
  /*
   * Compute sinr for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
   *
   * @param frame pointer to the packet
   * @param lteinfo pointer to the user control info
   */
  virtual std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo* lteInfo);
  /*
   * Compute Received useful signal for D2D transmissions
   */
  virtual std::vector<double> getRSRP_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord);
  /*
   * Compute sinr (D2D) for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
   *
   * @param frame pointer to the packet
   * @param lteinfo pointer to the user control info
   */
  virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord,MacNodeId enbId);
  virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord,MacNodeId enbId,const  std::vector<double>& rsrpVector);
  /*
   * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
   * after that it throws a random number in order to check if this packet will be corrupted or not
   *
   * @param frame pointer to the packet
   * @param lteinfo pointer to the user control info
   * @param rsrpVector the received signal for each RB, if it has already been computed
   */
  virtual bool error_D2D(LteAirFrame *frame, UserControlInfo* lteI, const std::vector<double>& rsrpVector);
  /*
   * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
   * after that it throws a random number in order to check if this packet will be corrupted or not
   *
   * @param frame pointer to the packet
   * @param lteinfo pointer to the user control info
   */
  virtual bool isCorrupted(LteAirFrame *frame, UserControlInfo* lteI);
  /*
   * The same as before but used for das TODO to be implemnted
   *
   * @param frame pointer to the packet
   * @param lteinfo pointer to the user control info
   */
  virtual bool isCorruptedDas(LteAirFrame *frame, UserControlInfo* lteI)
  {
      throw cRuntimeError("DAS PHY LAYER TO BE IMPLEMENTED");
      return -1;
  }
  /*
   * Compute the path-loss attenuation according to the selected scenario
   *
   * @param distance between UE and eNodeB
   * @param los line-of-sight flag
   */
  virtual double computePathLoss(double distance, double dbp, bool los);
  /*
   * Compute attenuation for indoor scenario
   *
   * @param distance between UE and eNodeB
   * @param los line-of-sight flag
   */
  double computeIndoor(double distance, bool los);
  /*
   * Compute attenuation for Urban Micro cell
   *
   * @param distance between UE and eNodeB
   * @param los line-of-sight flag
   */
  double computeUrbanMicro(double distance, bool los);
  /*
   * compute scenario for Urban Macro cell
   *
   * @param distance between UE and eNodeB
   * @param los line-of-sight flag
   */
  double computeUrbanMacro(double distance, bool los);
  /*
   * compute scenario for Sub Urban Macro cell
   *
   * @param distance between UE and eNodeB
   * @param los line-of-sight flag
   */
  double computeSubUrbanMacro(double distance, double& dbp, bool los);
  /*
   * Compute scenario for rural macro cell
   *
   * @param distance between UE and eNodeB
   * @param los line-of-sight flag
   */
  double computeRuralMacro(double distance, double& dbp, bool los);
  /*
   * compute std deviation of shadowing according to scenario and visibility
   *
   * @param distance between UE and eNodeB
   * @param nodeid mac node id of UE
   */
  double getStdDev(bool dist, MacNodeId nodeId);
  /*
   * Compute Rayleigh fading
   *
   * @param i index in the trace file
   * @param nodeid mac node id of UE
   */
  double rayleighFading(MacNodeId id, unsigned int band);
  /*
   * Compute Jakes fading
   *
   * @param speed speed of UE
   * @param nodeid mac node id of UE
   * @param band logical bend id
   * @param cqiDl if true, the jakesMap in the UE side should be used
   */
  double jakesFading(MacNodeId noedId, double speed, unsigned int band, bool cqiDl);
  /*
   * Compute LOS probability
   *
   * @param d between UE and eNodeB
   * @param nodeid mac node id of UE
   */
  void computeLosProbability(double d, MacNodeId nodeId);

  JakesFadingMap * getJakesMap()
  {
      return &jakesFadingMap_;
  }

  virtual bool isUplinkInterferenceEnabled() { return enableUplinkInterference_; }
  virtual bool isD2DInterferenceEnabled() { return enableD2DInterference_; }
protected:

  /* compute speed (m/s) for a given node
   * @param nodeid mac node id of UE
   * @return the speed in m/s
   */
  double computeSpeed(const MacNodeId nodeId, const inet::Coord coord);

  /*
   * Updates position for a given node
   * @param nodeid mac node id of UE
   */
  void updatePositionHistory(const MacNodeId nodeId, const inet::Coord coord);

  /*
   * compute total interference due to eNB coexistence for the DL direction
   * @param eNbId id of the considered eNb
   * @param isCqi if we are computing a CQI
   */
  bool computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, inet::Coord coord, bool isCqi, const RbMap& rbmap, std::vector<double> * interference);

  /*
   * compute interference coming from neighboring cells for the UL direction
   */
  bool computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, bool isCqi, const RbMap& rbmap, std::vector<double> * interference);

  /*
   * compute interference coming from neighboring UEs for the D2D/D2D_MULTI direction
   */
  bool computeD2DInterference(MacNodeId eNbId, MacNodeId senderId, inet::Coord senderCoord, MacNodeId destId, inet::Coord destCoord, bool isCqi, const RbMap& rbmap, std::vector<double>* interference,Direction dir);

  /*
   * evaluates total interference from external cells seen from the spot given by coord
   * @return total interference expressed in dBm
   */
  bool computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, inet::Coord coord, bool isCqi, std::vector<double>* interference);

  /*
   * compute attenuation due to path loss and shadowing
   * @return attenuation expressed in dBm
   */
  double computeExtCellPathLoss(double dist, MacNodeId nodeId);

  /*
   * Obtain the jakes map for the specified UE
   * @param id mac id of the user
   */
  JakesFadingMap * obtainUeJakesMap(MacNodeId id);

};

#endif /* STACK_PHY_CHANNELMODEL_LTEREALISTICCHANNELMODEL_H_ */
