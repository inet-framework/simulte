//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

/*! \class MeshMaster meshMaster.h
 *  \brief Define the manager of the conflict graph (CG) for resource allocation purposes.
 *
 *  This module maintains all the information about the building of the conflict graph among UEs
 *  for enabling frequency reuse during resource allocation of D2D-capable UEs (i.e. conflicting
 *  UEs should not be allocated on the same resource block).
 *  This module builds a directed CG where vertices are UEs and there is an edge between UE a and
 *  UE b when the power perceived by b from a is above a certain threshold.
 */

#ifndef MESHMASTER_H
#define	MESHMASTER_H

#include "stack/mac/conflict_graph_utilities/utilities.h"

class MeshMaster {
    
    RecPwrMatrix RPAntenna;             ///for each couple of antenna stores the received power
    
    Graph conflictGraph;
    Graph connectivityGraph;
    
    double connectivityTh;   // connectivity threshold (define if two UEs can communicate)
    double conflictTh;       // conflict threshold (define if two UEs interfere)

    std::vector<std::vector<bool> > connectivityMatrix;
    
    /// Reference to the eNodeB MAC layer
    LteMacEnb *mac_;
    // Antenna gain of UE
    double antennaGainUe_;
    // Cable loss
    double cableLoss_;

    std::map<MacNodeId,std::set<MacNodeId> > conflictMap_;

public:
   
    MeshMaster();
    MeshMaster(LteMacEnb* macEnb, double conflictThreshold);
    virtual ~MeshMaster();
    
    //receive power structure
    void initRecPwrStruct();
    void computeRecPwrStruct();
    
    //initialize the matrix
    bool initConnectivityMatrix();
    
    //compute Connectivity Matrix
    void computeConnectivityMatrix();
    
    //print the matrix
    void printConnectivityMatrix();
    void printConflictMap();
    
    //compute Connectivity Graph
    void computeConnGraph();
    
    //compute Conflict Graph
    void computeConflictGraph();
    
    //print the Graph
    void printGraph(GraphType graph);

    int getEdgeNumber() {return connectivityGraph.size();}
    
    void cleanMeshMaster();
    // Return a reference to the conflict map
    const std::map<MacNodeId,std::set<MacNodeId> >* getConflictMap();

    // initialize all the structure
    bool initStructure();
    void computeStruct();
    
private:

    void computeAntennaRecvPwr(MacNodeId nodeId);
    int computeMatrixDimension();

    /// utility function used to compute the conflict graph
    void computeConflictEdge(double treshold);
    
    //given the two end points of an edge return the index
    int getEdgeIndex(int tx, int rx);
    
    void free_and_null(char **ptr);
    
    double computeReceivedPower(MacNodeId receiver_nodeId,MacNodeId transmitter_nodeId);

};

#endif	/* MESHMASTER_H */

