//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/conflict_graph_utilities/meshMaster.h"
#include "stack/mac/conflict_graph_utilities/utilities.h";
#include "stack/phy/ChannelModel/LteRealisticChannelModel.h"
#include "stack/phy/layer/LtePhyBase.h"
#include <iomanip>
#define NIL -1

/*!
 * \fn MeshMaster()
 * \memberof MeshMaster
 * \brief class constructor;
 */

MeshMaster::MeshMaster() {
    connectivityMatrix.clear();
}

/*!
 * \fn MeshMaster()
 * \memberof MeshMaster
 * \brief class constructor;
 * \param eNbScheduler pointer to the eNodeB scheduler
 */

MeshMaster::MeshMaster(LteMacEnb* macEnb, double conflictThreshold)
{
    connectivityMatrix.clear();

    // Get the reference to the eNodeB MAC Layer
    mac_ = macEnb;
    // Antenna gain of UE
    antennaGainUe_ = 0;
    // Cable loss
    cableLoss_ = 2;
    // Set the threshold
    connectivityTh = -50;
    conflictTh = conflictThreshold;
    conflictMap_.clear();
}

/*!
 * \fn ~MeshMaster()
 * \memberof MeshMaster
 * \brief class destructor
 */

MeshMaster::~MeshMaster()
{
    connectivityMatrix.clear();
    conflictMap_.clear();
}
/**
 * Clean all Mesh Master structure
 */
void MeshMaster::cleanMeshMaster()
{
    connectivityMatrix.clear();
    conflictMap_.clear();
}
/*!
 * \fn initRecPwrStruct
 * \memberof MeshMaster
 * \brief initialize the received power structure
 * Received power structure contains all the information about the received power
 * for each couple of antenna
 */

void MeshMaster::initRecPwrStruct()
{
    EV << "\tinitRecPwrStruct::\tInitialize Receive Power Structure" << endl;
    // Get the number of all the UEs in the Cell
    unsigned int num_ue = getBinder()->getUeList()->size();
    // For each UE in the Cell
    for (unsigned int i = 0; i < num_ue; i++)
    {
        // Create a new power vector
        PWRvector* tmpVector = new PWRvector[num_ue];
        // Initialize the "value" field of the PWRvector
        for (unsigned int var = 0; var < num_ue; ++var)
        {
            tmpVector[var].id_ = var;
            // Get the node id for the transmitter check
            MacNodeId nodeId = indexToNodeId(i);
            // Get the node Id for the receiver check
            MacNodeId nodeId2 = indexToNodeId(var);

            if (nodeId == nodeId2)
                tmpVector[var].value = -1;
        }
        pair<unsigned int, PWRvector*> tmp = pair<unsigned int, PWRvector*>(i, tmpVector);
        RPAntenna.insert(tmp);
    }
}


/*!
 * \fn recPwrStructPrint()
 * \memberof MeshMaster
 * \brief calculate the received power for each couple of antenna 
 *      in the mesh, stores information in the structure RPAntenna
 *  RPAntenna structure must be initialized
 */

void MeshMaster::computeRecPwrStruct()
{
    EV << "\tMeshMaster::\t\tComputing Receiver Power Structure" << endl;
    std::vector<UeInfo*>::const_iterator it = getBinder()->getUeList()->begin();
    std::vector<UeInfo*>::const_iterator et = getBinder()->getUeList()->end();
    ///compute the received power for all the Receivers
    for (;it!=et;++it)
    {
        computeAntennaRecvPwr((*it)->id);
    }
}

/*!
 * \fn computeAntennaRecvPwr(EnbId id)
 * \memberof MeshMaster
 * \brief calculate the received power between the antenna "id"(receiver) and 
 * all the others antennas inside the mesh network, stores information in RPAntenna
 * \param id id of the antenna
 */
void MeshMaster::computeAntennaRecvPwr(MacNodeId nodeId)
{
    // Get the index from the nodeId
    unsigned int id = nodeIdToIndex(nodeId);
    double tmp = 0;

    // et points at the right position of RPAntenna
    std::map<unsigned int, PWRvector*>::const_iterator et_id = RPAntenna.find(id);

    if (et_id == RPAntenna.end()) {
        cout << "MeshMaster:: " << endl;
        cout << "\tcomputeAntennaRecvPwr - Cannot find the antenna [id : " << id << " ]" << endl;
        exit(1);
    }

    // Set the iterators
    std::vector<UeInfo*>::const_iterator it = getBinder()->getUeList()->begin();
    std::vector<UeInfo*>::const_iterator et = getBinder()->getUeList()->end();

    // Compute the SNR for all the transmitters
    for (;it!=et;++it)
    {
        if (nodeId != (*it)->id)
        {
            // Get the received power
            tmp = computeReceivedPower(nodeId,(*it)->id);
        }
        // Get the index from the nodeId
        unsigned int index = nodeIdToIndex((*it)->id);
        if(index == id)
            et_id->second[index].value = -1; // No self interference is possible
        else
            et_id->second[index].value = tmp; // Set the receiving power
        // Set the id
        et_id->second[index].id_ = index;
    }
}

/**
 * Calculate the power received by the receiver_nodeId( All the scalar values are expressed in dBm).
 * @param receiver_nodeId The ID of the receiver
 * @param transmitter_nodeId The ID of the transmitter
 * @return Return the received power in dBm
 */
double MeshMaster::computeReceivedPower(MacNodeId receiver_nodeId,MacNodeId transmitter_nodeId)
{
    // Reference to the Physical Channel  of the transmitter
    LtePhyBase* transmitter_ltePhy;
    // Get the Physical Channel reference of the interfering node
    transmitter_ltePhy = check_and_cast<LtePhyBase*>(getSimulation()->getModule(getBinder()->getOmnetId(transmitter_nodeId))->getSubmodule("lteNic")->getSubmodule("phy"));
    // Get the transmission power
    double TxPower = transmitter_ltePhy->getTxPwr();
    // Get the Real Channel reference of the transmitter
    LteRealisticChannelModel* transmitter_realChan = dynamic_cast<LteRealisticChannelModel *>(transmitter_ltePhy->getChannelModel());
    // Get the receiver's position
    Coord receiver_coord = mac_->getDeployer()->getUePosition(receiver_nodeId);
    // Compute attenuation using data structures within the Macro Cell.
    double att = transmitter_realChan->getAttenuation(receiver_nodeId,UL,receiver_coord); // TODO: check if the attenuation is right
    double txPwr = TxPower - cableLoss_ + antennaGainUe_;
    // Compute the received power
    double rxPwr = txPwr - att;
    // Return the received power in dBm
    return rxPwr;
}

/*!
 * \fn computeMatrixDimension()
 * \memberof MeshMaster
 * \brief compute the dimension of the matrix used (number of the edge)
 */

int MeshMaster::computeMatrixDimension() {
    // Get the number of all the UEs in the Cell
    unsigned int num_ue = getBinder()->getUeList()->size();
    return num_ue*num_ue;
}

/*!
 * \fn computeConnGraph()
 * \memberof MeshMaster
 * \brief compute the connectivity Graph of the network evaluating received power
 *  between antennas
 */
void MeshMaster::computeConnGraph()
{
    Edge tmpEdge;
    // Get the number of all the UEs in the Cell
    unsigned int num_ue = getBinder()->getUeList()->size();
    int j = 0;
    for (std::map <unsigned int, PWRvector*>::const_iterator it = RPAntenna.begin(); it != RPAntenna.end(); it++)
    {
        for (unsigned int i = 0; i < num_ue; i++)
        {
            tmpEdge.ric = it->first;
            tmpEdge.mit = it->second[i].id_;
            //tmpEdge.value = channelMaster_->computeReceivedPower(it->second[i].id_, enbRic->getPosition(), LOS_ALL);
            tmpEdge.value = it->second[i].value;

            /// if the receiver is equal to the transmitter or the recv power is -1
            if (it->second[i].id_ == it->first || it->second[i].value==-1)
            {
                j++;
                continue;
            }

            /* If the received power is greater than the treshold, the antennas 
             * can communicate each other. Insert that couple in the connectivity 
             * graph.
             */
            if (it->second[i].value > connectivityTh)
            {
                //insert the couple in the conflict graph
                //insert the receiver
                pair<unsigned int, Edge> tmp = pair<unsigned int, Edge>(j, tmpEdge);
                connectivityGraph.insert(tmp);
            }
            j++;
        }
    }
}

/*!
 * \fn computeConflictGraph(string filename)
 * \memberof MeshMaster
 * \param conflict_Th conflict treshold
 * \brief compute the Conflict Graph of the mesh network
 *              - stores Conflict Graph informations in the file "filename"
 *              - fill the Incident Matrix
 *      
 */

/* File structure: idEdge1 idEdge2 mit1 ric1 mit2 ric2 label1 label2
 * idEdge1 : id of the first edge
 * idEdge2 : id of the second edge
 * mit1 : sender node of the first edge
 * ric1 : receiver node of the first edge
 * label1 : label for the first edge : "mit1,ric1"
 */


void MeshMaster::computeConflictGraph() {
    EV<<"MeshMaster::computeConflictGraph - computing conflict graph"<<endl;
    computeConflictEdge(conflictTh);
}

/*!
 * \fn printGraph(GraphType graph)
 * \memberof MeshMaster
 * \param graph type of graph to print on screen (Connectivity, Conflict)
 * \brief print the graph specified by the parameter
 */

void MeshMaster::printGraph(GraphType graph) {
    Graph* graphPrint;
    switch (graph) {
        case CONFLICT:
            graphPrint = &conflictGraph;
            cout << "--------------------CONFLICT GRAPH--------------------" << endl;
            cout << "\t\t[idEdge]\t\t[idmit]\t\t[idric]\t\t[RECPWR]" << endl;
            break;
        case CONNECTIVITY:
            graphPrint = &connectivityGraph;
            cout << "--------------------CONNECTIVITY GRAPH--------------------" << endl;
            cout << "\t\t[idEdge]\t\t[idmit]\t\t[idric]\t\t[RECPWR]" << endl;
            break;
        default:
            cout << "ChannelMaster::printGraph - Cannot recognized the Graph Type" << endl;
            exit(1);
            break;
    }

    for (std::map <unsigned int, Edge>::const_iterator it = graphPrint->begin(); it != graphPrint->end(); it++)
        cout << "\t\t" << it->first << "\t\t\t" << it->second.mit << "\t\t" << it->second.ric << "\t\t" << it->second.value << endl;

    cout << "----------------------END GRAPH----------------------" << endl;
}

/*!
 * \fn computeConflictEdge(double threshold)
 * \memberof MeshMaster
 * \param treshold treshold of the received power
 * \brief check if the power received from a node is higher then a threshold
 * compute the subset of node that can conflict
 */

void MeshMaster::computeConflictEdge(double treshold)
{
    Edge tmpEdge;
    int j = 0;

    // Get the number of all the UEs in the Cell
    unsigned int num_ue = getBinder()->getUeList()->size();

    // For all the node in the RecvPower struct
    for (std::map<unsigned int, PWRvector*>::const_iterator it = RPAntenna.begin(); it != RPAntenna.end(); it++)
    {
        for (unsigned int i = 0; i < num_ue; i++)
        {
            if (it->second[i].value == -1 )
            {
                j++;
                continue;
            }
            // Extract the parameters
            tmpEdge.mit = it->second[i].id_;
            tmpEdge.ric = it->first;
            tmpEdge.value = it->second[i].value;
            // Creates the pair
            pair<unsigned int, Edge> tmp = pair<unsigned int, Edge>(j, tmpEdge);
            // If the value is above the threshold insert the pair in the conflict graph
            if ((it->second[i].value > treshold ) ) // || getBinder()->checkD2DCapability(indexToNodeId(tmpEdge.mit), indexToNodeId(tmpEdge.ric)) )   // peering UEs are in conflict
            {
                conflictGraph.insert(tmp);
                conflictMap_[indexToNodeId(tmpEdge.mit)].insert(indexToNodeId(tmpEdge.ric));
            }
            j++;
        }
    }
}


/*!
 * \fn initStructure()
 * \memberof MeshMaster
 * \brief utility function used to initialize all the structure used by the MeshMaster module
 */

bool MeshMaster::initStructure() {
    EV << "MeshMaster::initStructure\n" << endl;
    bool done = false;
    initRecPwrStruct();

    done = initConnectivityMatrix();
    if (!done) {
        cout << "MeshMaster::initStructure - Error: Failed to initialize connectivity matrix.\n" << endl;
        exit(1);
    }

    return true;
}

// Compute the Received power matrix, the connectivity graph and the conflict graph
void MeshMaster::computeStruct()
{
      this->computeRecPwrStruct();
      this->computeConnGraph();
      this->computeConflictGraph();
}

/*!
 * \fn initConnectivityMatrix()
 * \memberof MeshMaster
 * \brief initialize the connectivity matrix used for the worst-case graph
 */

bool MeshMaster::initConnectivityMatrix() {
    //int numElements = macroList->size() + microList->size();
    // Get the number of all the UEs in the Cell
    unsigned int num_ue = getBinder()->getUeList()->size();
    //connectivityMatrix = (int **) malloc(num_ue * sizeof (int*));
    EV << "\tinitConnectivityMatrix::\tInitialize Connectivity Matrix\n" << endl;
    //for (unsigned int i = 0; i < num_ue; i++) {
      //  connectivityMatrix[i] = (int*) malloc(num_ue * sizeof (int*));
        //connectivityMatrix[i] = (int*) calloc(num_ue, sizeof (int*));
    //}
    for (unsigned int i = 0; i < num_ue; i++)
        //memset(connectivityMatrix[i], 0, num_ue * sizeof (int));
        connectivityMatrix.push_back(std::vector<bool>(num_ue,0));
    return true;
}

/*!
 * \fn computeConnectivityMatrix()
 * \memberof MeshMaster
 * \brief compute the connectivity matrix, if cell (i,j) is set to 1 node i can transmits to node j
 * \ IMPORTANT :the rows rapresents the node that transmits
 */

void MeshMaster::computeConnectivityMatrix() {
    cout << "\tMeshMaster::\t\tComputing Connectivity Matrix" << endl;
    for (std::map<unsigned int, Edge>::const_iterator it = connectivityGraph.begin(); it != connectivityGraph.end(); it++) {
        
        connectivityMatrix[it->second.mit][it->second.ric] = 1;
    }
}

void MeshMaster::free_and_null(char **ptr) {
    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
} 

/**
 * Print the content of the conflict map.
 */
void MeshMaster::printConflictMap()
{
    std::cout<<"********** CONFLICT MAP **********"<<endl;
    std::map<MacNodeId,std::set<MacNodeId> >::iterator it = conflictMap_.begin() , et = conflictMap_.end();
    while(it!=et)
    {
        std::cout<<"Node: "<<it->first<<" \tConflicting nodes: ";
        for(std::set<MacNodeId>::iterator it_set = it->second.begin();it_set!=it->second.end();++it_set)
        {
            std::cout<<" "<<*it_set<<" ";
        }
        std::cout<<endl;
        ++it;
    }
    std::cout<<"**********************************"<<endl;
}

/**
 * Return a reference to the conflict map
 * @return A reference to the conflict map
 */
const std::map<MacNodeId,std::set<MacNodeId> >* MeshMaster::getConflictMap()
{
    return &conflictMap_;
}
