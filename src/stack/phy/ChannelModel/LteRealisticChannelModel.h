//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEREALISTICCHANNELMODEL_H_
#define _LTE_LTEREALISTICCHANNELMODEL_H_

#include "LteChannelModel.h"

class LteBinder;

/*
 * Realistic Channel Model as taken from
 * "ITU-R M.2135-1 Guidelines for evaluation of radio interface technologies for IMT-Advanced"
 */
class LteRealisticChannelModel : public LteChannelModel
{
  private:
    // Carrier Frequency
    double carrierFrequency_;

    // stores my Playground Coords
    const Coord& myCoord_;

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
    bool enableMultiCellInterference_;
    bool enableD2DInCellInterference_;

    typedef std::pair<simtime_t, Coord> Position;

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

//    typedef std::map<MacNodeId,std::vector<JakesFadingData> > JakesFadingMap;

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

  public:
    LteRealisticChannelModel(ParameterMap& params, const Coord& myCoord, unsigned int band);
    virtual ~LteRealisticChannelModel();
    /*
     * Compute Attenuation caused by pathloss and shadowing (optional)
     *
     * @param nodeid mac node id of UE
     * @param dir traffic direction
     * @param coord position of end point comunication (if dir==UL is the position of UE else is the position of eNodeB)
     */
    virtual double getAttenuation(MacNodeId nodeId, Direction dir, Coord coord);
    /*
     * Compute Attenuation for D2D caused by pathloss and shadowing (optional)
     *
     * @param nodeid mac node id of UE
     * @param dir traffic direction
     * @param coord position of end point comunication (if dir==UL is the position of UE else is the position of eNodeB)
     */
    virtual double getAttenuation_D2D(MacNodeId nodeId, Direction dir, Coord coord,MacNodeId node2_Id, Coord coord_2);
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
    virtual std::vector<double> getRSRP_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord);
    /*
     * Compute sinr (D2D) for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord,MacNodeId enbId);
    virtual std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, Coord destCoord,MacNodeId enbId,std::vector<double> rsrpVector);
    /*
     * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     * @param rsrpVector the received signal for each RB, if it has already been computed
     */
    virtual bool error_D2D(LteAirFrame *frame, UserControlInfo* lteI, std::vector<double> rsrpVector);
    /*
     * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual bool error(LteAirFrame *frame, UserControlInfo* lteI);
    /*
     * The same as before but used for das TODO to be implemnted
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    virtual bool errorDas(LteAirFrame *frame, UserControlInfo* lteI)
    {
        throw cRuntimeError("DAS PHY LAYER TO BE IMPLEMENTED");
        return -1;
    }
    /*
     * Compute attenuation for indoor scenario
     *
     * @param distance between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    double computeIndoor(double distance, MacNodeId nodeId);
    /*
     * Compute attenuation for Urban Micro cell
     *
     * @param distance between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    double computeUrbanMicro(double distance, MacNodeId nodeId);
    /*
     * compute scenario for Urban Macro cell
     *
     * @param distance between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    double computeUrbanMacro(double distance, MacNodeId nodeId);
    /*
     * compute scenario for Sub Urban Macro cell
     *
     * @param distance between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    double computeSubUrbanMacro(double distance, double& dbp, MacNodeId nodeId);
    /*
     * Compute scenario for rural macro cell
     *
     * @param distance between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    double computeRuralMacro(double distance, double& dbp, MacNodeId nodeId);
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

  protected:

    /* compute speed (m/s) for a given node
     * @param nodeid mac node id of UE
     * @return the speed in m/s
     */
    double computeSpeed(const MacNodeId nodeId, const Coord coord);

    /*
     * Updates position for a given node
     * @param nodeid mac node id of UE
     */
    void updatePositionHistory(const MacNodeId nodeId, const Coord coord);

    /*
     * compute total interference due to eNB coexistence
     * @param eNbId id of the considered eNb
     * @param isCqi if we are computing a CQI
     */
    bool computeMultiCellInterference(MacNodeId eNbId, MacNodeId ueId, Coord coord, bool isCqi,
        std::vector<double> * interference);

    /*
     * compute total interference due to D2D transmissions within the same cell
     */
    bool computeInCellD2DInterference(MacNodeId eNbId, MacNodeId senderId, Coord senderCoord, MacNodeId destId, Coord destCoord, bool isCqi,std::vector<double>* interference,Direction dir);

    /*
     * evaluates total intercell interference seen from the spot given by coord
     * @return total interference expressed in dBm
     */
    bool computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, Coord coord, bool isCqi,
        std::vector<double>* interference);

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

#endif
