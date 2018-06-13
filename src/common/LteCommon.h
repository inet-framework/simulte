//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//  Description:
//  This file contains LTE typedefs and constants.
//  At the end of the file there are some utility functions.
//

#ifndef _LTE_LTECOMMON_H_
#define _LTE_LTECOMMON_H_

#define _NO_W32_PSEUDO_MODIFIERS

#include <iostream>
#include <omnetpp.h>
#include <string>
#include <fstream>
#include <vector>
#include <bitset>
#include <set>
#include <queue>
#include <map>
#include <list>
#include <algorithm>
#include "inet/common/geometry/common/Coord.h"
#include "common/features.h"

using namespace omnetpp;

class LteBinder;
class LteCellInfo;
class LteMacEnb;
class LteMacBase;
class LtePhyBase;
//class cXMLElement;
class LteRealisticChannelModel;
class LteControlInfo;
class ExtCell;



/**
 * TODO
 */
#define ELEM(x) {x,#x}

/// Transmission time interval
#define TTI 0.001

/// Current simulation time
#define NOW simTime()

/// Node Id bounds
#define ENB_MIN_ID 1
#define ENB_MAX_ID 255
#define RELAY_MIN_ID 257
#define RELAY_MAX_ID 1023
#define UE_MIN_ID 1025
#define UE_MAX_ID 65535

/// Max Number of Codewords
#define MAX_CODEWORDS 2

// Number of QCI classes
#define LTE_QCI_CLASSES 9

/// MAC node ID
typedef unsigned short MacNodeId;

/// Cell node ID. It is numerically equal to eNodeB MAC node ID.
typedef unsigned short MacCellId;

/// X2 node ID. It is equal to the eNodeB MAC Cell ID
typedef unsigned short X2NodeId;

/// Omnet Node Id
typedef unsigned int OmnetId;

/// Logical Connection Identifier
typedef unsigned short LogicalCid;

/// Connection Identifier: <MacNodeId,LogicalCid>
typedef unsigned int MacCid;

/// Rank Indicator
typedef unsigned short Rank;

/// Channel Quality Indicator
typedef unsigned short Cqi;

/// Precoding Matrix Index
typedef unsigned short Pmi;

/// Transport Block Size
typedef unsigned short Tbs;

/// Logical band
typedef unsigned short Band;

/// Codeword
typedef unsigned short Codeword;

/// Link Directions
enum Direction
{
    DL, UL, D2D, D2D_MULTI, UNKNOWN_DIRECTION
};

/// Modulations
enum LteMod
{
    _QPSK = 0, _16QAM, _64QAM
};

/// Feedback reporting type
enum FeedbackType
{
    ALLBANDS = 0, PREFERRED, WIDEBAND
};

/// Feedback periodicity
enum FbPeriodicity
{
    PERIODIC, APERIODIC
};

/// Resource allocation type
enum RbAllocationType
{
    TYPE2_DISTRIBUTED, TYPE2_LOCALIZED
};

/* workaround: <windows.h> defines IN and OUT */
#undef IN
#undef OUT

/// Gate Direction
enum GateDirection
{
    IN = 0, OUT = 1
};

/// Lte Traffic Classes
enum LteTrafficClass
{
    CONVERSATIONAL, STREAMING, INTERACTIVE, BACKGROUND, UNKNOWN_TRAFFIC_TYPE
};

/// Scheduler grant type
enum GrantType
{
    FITALL = 0, FIXED_, URGENT, UNKNOWN_GRANT_TYPE /* Note: FIXED would clash with <windows.h> */
};

// QCI traffic descriptor
struct QCIParameters
{
    int priority;
    double packetDelayBudget;
    double packetErrorLossRate;
};

/// Lte RLC Types
enum LteRlcType
{
    TM, UM, AM, UNKNOWN_RLC_TYPE
};

// Attenuation vector for analogue models
typedef std::vector<double> AttenuationVector;

/// Index for UL transmission map
enum UlTransmissionMapTTI
{
    PREV_TTI, CURR_TTI
};

/// D2D Modes
// IM = Infastructure Mode
// DM = Direct (D2D) Mode
enum LteD2DMode
{
    IM, DM
};

/*************************
 *     Applications      *
 *************************/

enum ApplicationType
{
    VOIP = 0,
    VOD,
    WEB,
    CBR,
    FTP,
    GAMING,
    FULLBUFFER,
    UNKNOWN_APP
};

struct ApplicationTable
{
    ApplicationType app;
    std::string appName;
};

const ApplicationTable applications[] = {
    ELEM(VOIP),
    ELEM(VOD),
    ELEM(WEB),
    ELEM(CBR),
    ELEM(FTP),
    ELEM(FULLBUFFER),
    ELEM(UNKNOWN_APP)
};

/**************************
 * Scheduling discipline  *
 **************************/

enum SchedDiscipline
{
    DRR, PF, MAXCI, MAXCI_MB, MAXCI_OPT_MB, MAXCI_COMP, ALLOCATOR_BESTFIT, UNKNOWN_DISCIPLINE
};

struct SchedDisciplineTable
{
    SchedDiscipline discipline;
    std::string disciplineName;
};

const SchedDisciplineTable disciplines[] = {
    ELEM(DRR),
    ELEM(PF),
    ELEM(MAXCI),
    ELEM(MAXCI_MB),
    ELEM(MAXCI_OPT_MB),
    ELEM(MAXCI_COMP),
    ELEM(ALLOCATOR_BESTFIT),
    ELEM(UNKNOWN_DISCIPLINE)
};

// specifies how the final CQI will be computed from the multiband ones
enum PilotComputationModes
{
    MIN_CQI, MAX_CQI, AVG_CQI
};

/*************************
 *   Transmission Modes  *
 *************************/

enum TxMode
{
    // Note: If you add more tx modes, update DL_NUM_TXMODE and UL_NUM_TXMODE
    SINGLE_ANTENNA_PORT0 = 0,
    SINGLE_ANTENNA_PORT5,
    TRANSMIT_DIVERSITY,
    OL_SPATIAL_MULTIPLEXING,
    CL_SPATIAL_MULTIPLEXING,
    MULTI_USER,
    UNKNOWN_TX_MODE
};

struct TxTable
{
    TxMode tx;
    std::string txName;
};

const TxTable txmodes[] = {
    ELEM(SINGLE_ANTENNA_PORT0),
    ELEM(SINGLE_ANTENNA_PORT5),
    ELEM(TRANSMIT_DIVERSITY),
    ELEM(OL_SPATIAL_MULTIPLEXING),
    ELEM(CL_SPATIAL_MULTIPLEXING),
    ELEM(MULTI_USER),
    ELEM(UNKNOWN_TX_MODE)
};

enum TxDirectionType
{
    ANISOTROPIC,
    OMNI
};

struct TxDirectionTable
{
    TxDirectionType txDirection;
    std::string txDirectionName;
};

const TxDirectionTable txDirections[] = {
    ELEM(ANISOTROPIC),
    ELEM(OMNI)
};

// Lte feedback type
enum FeedbackGeneratorType
{
    IDEAL = 0,
    REAL,
    DAS_AWARE,
    UNKNOW_FB_GEN_TYPE
};

struct FeedbackRequest
{
    bool request;
    FeedbackGeneratorType genType;
    FeedbackType type;
    //used if genType==real
    TxMode txMode;
    bool dasAware;
    RbAllocationType rbAllocationType;
};

struct FeedbackGeneratorTypeTable
{
    FeedbackGeneratorType ty;
    std::string tyname;
};

const FeedbackGeneratorTypeTable feedbackGeneratorTypeTable[] = {
    ELEM(IDEAL),
    ELEM(REAL),
    ELEM(DAS_AWARE),
    ELEM(UNKNOW_FB_GEN_TYPE)
};

/// Number of transmission modes in DL direction.
const unsigned char DL_NUM_TXMODE = MULTI_USER + 1;

/// Number of transmission modes in UL direction.
const unsigned char UL_NUM_TXMODE = MULTI_USER + 1;

/// OFDMA layers (see FIXME lteAllocationModuble.h for "layers" meaning)
enum Plane
{
    MAIN_PLANE = 0, MU_MIMO_PLANE
};

enum DeploymentScenario
{
    INDOOR_HOTSPOT = 0,
    URBAN_MICROCELL,
    URBAN_MACROCELL,
    RURAL_MACROCELL,
    SUBURBAN_MACROCELL,
    UNKNOW_SCENARIO
};

struct DeploymentScenarioMapping
{
    DeploymentScenario scenario;
    std::string scenarioName;
};

const DeploymentScenarioMapping DeploymentScenarioTable[] = {
    ELEM(INDOOR_HOTSPOT),
    ELEM(URBAN_MICROCELL),
    ELEM(URBAN_MACROCELL),
    ELEM(RURAL_MACROCELL),
    ELEM(SUBURBAN_MACROCELL),
    ELEM(UNKNOW_SCENARIO)
};

const unsigned int txModeToIndex[6] = { 0, 0, 1, 2, 2, 0 };

const TxMode indexToTxMode[3] = {
    SINGLE_ANTENNA_PORT0,
    TRANSMIT_DIVERSITY,
    OL_SPATIAL_MULTIPLEXING
};

typedef std::map<MacNodeId, TxMode> TxModeMap;

const double cqiToByteTms[16] = { 0, 2, 3, 5, 11, 15, 20, 25, 36, 38, 49, 63, 72, 79, 89, 92 };

struct Lambda
{
    unsigned int index;
    unsigned int lambdaStart;
    unsigned int channelIndex;
    double lambdaMin;
    double lambdaMax;
    double lambdaRatio;
};

double dBmToLinear(double dbm);
double dBToLinear(double db);
double linearToDBm(double lin);
double linearToDb(double lin);

/*************************
 *      DAS Support      *
 *************************/

/// OFDMA Remotes (see FIXME LteAllocationModule.h for "antenna" meaning)
enum Remote
{
    MACRO = 0,
    RU1,
    RU2,
    RU3,
    RU4,
    RU5,
    RU6,
    UNKNOWN_RU
};

struct RemoteTable
{
    Remote remote;
    std::string remoteName;
};

const RemoteTable remotes[] = {
    ELEM(MACRO),
    ELEM(RU1),
    ELEM(RU2),
    ELEM(RU3),
    ELEM(RU4),
    ELEM(RU5),
    ELEM(RU6),
    ELEM(UNKNOWN_RU)
};

/**
 * Maximum number of available DAS RU per cell.
 * To increase this number, change former enumerate accordingly.
 * MACRO antenna excluded.
 */
const unsigned char NUM_RUS = RU6;

/**
 * Maximum number of available ANTENNAS per cell.
 * To increase this number, change former enumerate accordingly.
 * MACRO antenna included.
 */
const unsigned char NUM_ANTENNAS = NUM_RUS + 1;

/**
 *  Block allocation Map: # of Rbs per Band, per Remote.
 */
typedef std::map<Remote, std::map<Band, unsigned int> > RbMap;

/**
 * Lte PHY Frame Types
 */
enum LtePhyFrameType
{
    DATAPKT,
    BROADCASTPKT,
    FEEDBACKPKT,
    HANDOVERPKT,
    HARQPKT,
    GRANTPKT,
    RACPKT,
    D2DMODESWITCHPKT,
    UNKNOWN_TYPE
};

struct LtePhyFrameTable
{
    LtePhyFrameType phyType;
    std::string phyName;
};

const LtePhyFrameTable phytypes[] = {
    ELEM(DATAPKT),
    ELEM(BROADCASTPKT),
    ELEM(FEEDBACKPKT),
    ELEM(HANDOVERPKT),
    ELEM(GRANTPKT),
    ELEM(D2DMODESWITCHPKT),
    ELEM(UNKNOWN_TYPE)
};

/**
 * Lte Node Types
 */
enum LteNodeType
{
    INTERNET, /// Internet side of the Lte network
    ENODEB, /// eNodeB
    RELAY, /// Relay
    UE, /// UE
    UNKNOWN_NODE_TYPE /// unknown
};

struct LteNodeTable
{
    LteNodeType node;
    std::string nodeName;
};

const LteNodeTable nodetypes[] = {
    ELEM(INTERNET),
    ELEM(ENODEB),
    ELEM(RELAY),
    ELEM(UE),
    ELEM(UNKNOWN_NODE_TYPE)
};

/**
 * Subframe type
 */
enum LteSubFrameType
{
    NORMAL_FRAME_TYPE,
    MBSFN,
    PAGING,
    BROADCAST,
    SYNCRO,
    ABS,
    UNKNOWN_FRAME_TYPE
};

struct LteSubFrameTypeTable
{
    LteSubFrameType type;
    std::string typeName;
};

const LteSubFrameTypeTable subFrametypes[] = {
    ELEM(NORMAL_FRAME_TYPE),
    ELEM(MBSFN),
    ELEM(PAGING),
    ELEM(BROADCAST),
    ELEM(SYNCRO),
    ELEM(ABS),
    ELEM(UNKNOWN_FRAME_TYPE)
};

//|--------------------------------------------------|
//|----------------- ABS Management -----------------|
//|--------------------------------------------------|
//********* See 3GPP TS 36.423 for more info *********
const int ABS_WIN = 40;
typedef std::bitset<ABS_WIN> AbsBitset;

/*
 * ABS_INFO         macro->micro
 * ABS_STATUS_INFO    micro->macro
 *
 * these are the names from standard, so it's not my fault if they are stupid.
 */
enum X2MsgType
{
    ABS_INFO,
    ABS_STATUS_INFO
};

// Abs Status Information structure
struct AbsStatusInfoMsg
{
    double absStatus;
    AbsBitset usableAbsInfo;
};

//|--------------------------------------------------|

/*****************
 * X2 Support
 *****************/

class X2InformationElement;

/**
 * The Information Elements List, a list
 * of IEs contained inside a X2Message
 */
typedef std::list<X2InformationElement*> X2InformationElementsList;

/*
 * Types of BSR
 * TODO add LONG/TRUNCATED BSR
 */
enum BsrType
{
    SHORT_BSR, D2D_SHORT_BSR, D2D_MULTI_SHORT_BSR
};

/**
 * The following structure specifies a band and a byte amount which limits the schedulable data
 * on it.
 * If this limit is -1, it is considered an unlimited capping.
 * If this limit is -2, the band cannot be used.
 * Among other modules, the rtxAcid and grant methods of LteSchedulerEnb use this structure.
 */
struct BandLimit
{
    /// Band which the element refers to
    Band band_;
    /// Limit of bytes (per codeword) which can be requested for the current band
    std::vector<int> limit_;

    BandLimit()
    {
        band_ = 0;
        limit_.resize(MAX_CODEWORDS, -1);
    }

    // default "from Band" constructor
    BandLimit(Band b)
    {
        band_ = b;
        limit_.resize(MAX_CODEWORDS, -1);
    }
    bool operator<(const BandLimit rhs) const
    {
        return (limit_[0] > rhs.limit_[0]);
    }
};

/*****************
 * LTE Constants
 *****************/

const unsigned char MAXCW = 2;
const Cqi MAXCQI = 15;
const Cqi NOSIGNALCQI = 0;
const Pmi NOPMI = 0;
const Rank NORANK = 1;
const Tbs CQI2ITBSSIZE = 29;
const unsigned int PDCP_HEADER_UM = 1;
const unsigned int PDCP_HEADER_AM = 2;
const unsigned int RLC_HEADER_UM = 2; // TODO
const unsigned int RLC_HEADER_AM = 2; // TODO
const unsigned int MAC_HEADER = 2;
const unsigned int MAXGRANT = 4294967295U;

/*****************
 * MAC Support
 *****************/

class LteMacBuffer;
class LteMacQueue;
class MacControlElement;
class LteMacPdu;

/**
 * This is a map that associates each Connection Id with
 * a Mac Queue, storing  MAC SDUs (or RLC PDUs)
 */
typedef std::map<MacCid, LteMacQueue*> LteMacBuffers;

/**
 * This is a map that associates each Connection Id with
 *  a buffer storing the  MAC SDUs info (or RLC PDUs).
 */
typedef std::map<MacCid, LteMacBuffer*> LteMacBufferMap;

/**
 * This is the Schedule list, a list of schedule elements.
 * For each CID on each codeword there is a number of SDUs
 */
typedef std::map<std::pair<MacCid, Codeword>, unsigned int> LteMacScheduleList;

/**
 * This is the Pdu list, a list of scheduled Pdus for
 * each user on each codeword.
 */
typedef std::map<std::pair<MacNodeId, Codeword>, LteMacPdu*> MacPduList;

/*
 * Codeword list : for each node, it keeps track of allocated codewords (number)
 */
typedef std::map<MacNodeId, unsigned int> LteMacAllocatedCws;

/**
 * The Rlc Sdu List, a list of RLC SDUs
 * contained inside a RLC PDU
 */
typedef std::list<cPacket*> RlcSduList;

/**
 * The Mac Sdu List, a list of MAC SDUs
 * contained inside a MAC PDU
 */
typedef cPacketQueue MacSduList;

/**
 * The Mac Control Elements List, a list
 * of CEs contained inside a MAC PDU
 */
typedef std::list<MacControlElement*> MacControlElementsList;

/*****************
 * HARQ Support
 *****************/

/// Unknown acid code
#define HARQ_NONE 255

/// Number of harq tx processes
#define ENB_TX_HARQ_PROCESSES 8
#define UE_TX_HARQ_PROCESSES 8
#define ENB_RX_HARQ_PROCESSES 8
#define UE_RX_HARQ_PROCESSES 8

/// time interval between two transmissions of the same pdu
#define HARQ_TX_INTERVAL 7*TTI

/// time it takes to generate feedback for a pdu
#define HARQ_FB_EVALUATION_INTERVAL 3*TTI

/// H-ARQ feedback (ACK, NACK)
enum HarqAcknowledgment
{
    HARQNACK = 0, HARQACK = 1
};

/// TX H-ARQ pdu status
enum TxHarqPduStatus
{
    /// pdu is ready for retransmission (nack received)
    TXHARQ_PDU_BUFFERED = 0,
    /// pdu is waiting for feedback
    TXHARQ_PDU_WAITING,
    /// no pdu inside this process (empty process)
    TXHARQ_PDU_EMPTY,
    /// pdu selected for transmission
    TXHARQ_PDU_SELECTED
};

/// RX H-ARQ pdu status
enum RxHarqPduStatus
{
    /// no pdu, process is empty
    RXHARQ_PDU_EMPTY = 0,
    /// pdu is in evaluating state
    RXHARQ_PDU_EVALUATING,
    /// pdu has been evaluated and it is correct
    RXHARQ_PDU_CORRECT,
    /// pdu has been evaluated and it is not correct
    RXHARQ_PDU_CORRUPTED
};

struct RemoteUnitPhyData
{
    int txPower;
    inet::Coord m;
};

// Codeword List - returned by Harq functions
typedef std::list<Codeword> CwList;

/// Pair of acid, list of unit ids
typedef std::pair<unsigned char, CwList> UnitList;

/*********************
 * Incell Interference Support
 *********************/
enum EnbType
{
    // macro eNb
    MACRO_ENB,
    // micro eNb
    MICRO_ENB
};

struct EnbInfo
{
    bool init;         // initialization flag
    EnbType type;     // MICRO_ENB or MACRO_ENB
    double txPwr;
    TxDirectionType txDirection;
    double txAngle;
    MacNodeId id;
    LteMacEnb * mac;
    LteRealisticChannelModel * realChan;
    cModule * eNodeB;
    int x2;
};

struct UeInfo
{
    bool init;         // initialization flag
    double txPwr;
    MacNodeId id;
    MacNodeId cellId;
    LteRealisticChannelModel * realChan;
    cModule * ue;
    LtePhyBase* phy;
};

// uplink interference support
struct UeAllocationInfo{
    MacNodeId nodeId;
    MacCellId cellId;
    LtePhyBase* phy;
    Direction dir;
};

typedef std::vector<ExtCell*> ExtCellList;

/*****************
 *  PHY Support  *
 *****************/

typedef std::vector<std::vector<std::vector<double> > > BlerCurves;

/*************************************
 * Shortcut for structures using STL
 *************************************/

typedef std::vector<Cqi> CqiVector;
typedef std::vector<Pmi> PmiVector;
typedef std::set<Band> BandSet;
typedef std::set<Remote> RemoteSet;
typedef std::map<MacNodeId, bool> ConnectedUesMap;
typedef std::pair<int, simtime_t> PacketInfo;
typedef std::vector<RemoteUnitPhyData> RemoteUnitPhyDataVector;
typedef std::set<MacNodeId> ActiveUser;
typedef std::set<MacCid> ActiveSet;

/**
 * Used at initialization to pass the parameters
 * to the AnalogueModels and Decider.
 *
 * Parameters read from xml file are stored in this map.
 */
typedef std::map<std::string, cMsgPar> ParameterMap;

/*********************
 * Utility functions
 *********************/

const std::string dirToA(Direction dir);
const std::string d2dModeToA(LteD2DMode mode);
const std::string allocationTypeToA(RbAllocationType type);
const std::string modToA(LteMod mod);
const std::string periodicityToA(FbPeriodicity per);
const std::string txModeToA(TxMode tx);
TxMode aToTxMode(std::string s);
const std::string schedDisciplineToA(SchedDiscipline discipline);
SchedDiscipline aToSchedDiscipline(std::string s);
Remote aToDas(std::string s);
const std::string dasToA(const Remote r);
const std::string nodeTypeToA(const LteNodeType t);
LteNodeType aToNodeType(std::string name);
LteNodeType getNodeTypeById(MacNodeId id);
FeedbackType getFeedbackType(std::string s);
RbAllocationType getRbAllocationType(std::string s);
ApplicationType aToApplicationType(std::string s);
const std::string applicationTypeToA(std::string s);
const std::string lteTrafficClassToA(LteTrafficClass type);
LteTrafficClass aToLteTrafficClass(std::string s);
const std::string phyFrameTypeToA(const LtePhyFrameType r);
LtePhyFrameType aToPhyFrameType(std::string s);
const std::string rlcTypeToA(LteRlcType type);
char* cStringToLower(char* str);
LteRlcType aToRlcType(std::string s);
const std::string planeToA(Plane p);
MacNodeId ctrlInfoToUeId(LteControlInfo * info);
MacCid idToMacCid(MacNodeId nodeId, LogicalCid lcid);
MacCid ctrlInfoToMacCid(LteControlInfo * info);        // get the CID from the packet control info
MacNodeId MacCidToNodeId(MacCid cid);
LogicalCid MacCidToLcid(MacCid cid);
GrantType aToGrantType(std::string a);
const std::string grantTypeToA(GrantType gType);
LteBinder* getBinder();
LteCellInfo* getCellInfo(MacNodeId nodeId);
cModule* getMacByMacNodeId(MacNodeId nodeId);
cModule* getRlcByMacNodeId(MacNodeId nodeId, LteRlcType rlcType);
LteMacBase* getMacUe(MacNodeId nodeId);
FeedbackGeneratorType getFeedbackGeneratorType(std::string s);
const std::string fbGeneratorTypeToA(FeedbackGeneratorType type);
LteSubFrameType aToSubFrameType(std::string s);
const std::string SubFrameTypeToA(const LteSubFrameType r);
const std::string DeploymentScenarioToA(DeploymentScenario type);
DeploymentScenario aToDeploymentScenario(std::string s);
bool isMulticastConnection(LteControlInfo* lteInfo);

/**
 * Utility function that reads the parameters of an XML element
 * and stores them in the passed ParameterMap reference.
 *
 * @param xmlData XML parameters config element related to a specific section
 * @param[output] outputMap map to store read parameters
 */
void getParametersFromXML(cXMLElement* xmlData, ParameterMap& outputMap);

/**
 * Parses a CSV string parameter into an int array.
 *
 * The string must be in the format "v1,v2,..,vi,...,vN;" and if dim > N,
 * the values[i] are filled with zeros from when i = N + 1.
 * Warning messages are issued if the string has less or more values than dim.
 *
 * @param str string to be parsed
 * @param values pointer to the int array
 * @param dim dimension of the values array
 * @param pad default value to be used when the string has less than dim values
 */
void parseStringToIntArray(std::string str, int* values, int dim, int pad);

/**
 * Initializes module's channels
 *
 * A dinamically created node needs its channels to be initialized, this method
 * runs through all a module's and its submodules' channels recursively and
 * initializes all channels.
 *
 * @param mod module whose channels needs initialization
 */
void initializeAllChannels(cModule *mod);

#endif

