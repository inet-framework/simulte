//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef CONFLICTGRAPH_H
#define	CONFLICTGRAPH_H

#include "stack/mac/layer/LteMacEnbD2D.h"
#include "corenetwork/lteCellInfo/LteCellInfo.h"

typedef enum
{
    CG_DISTANCE,
    CG_UNKNOWN
} CGType;


/*
 * Define the structure for graph's vertices
 */
struct CGVertex
{
    MacNodeId srcId;
    MacNodeId dstId;

    bool operator<(const CGVertex& v1) const
    {
        if (srcId < v1.srcId)
            return true;
        if ((srcId == v1.srcId) && (dstId < v1.dstId))
            return true;
        return false;
    }

    bool operator==(const CGVertex& v1) const
    {
        return (srcId == v1.srcId) && (dstId == v1.dstId);
    }

public:
    CGVertex(MacNodeId src = 0, MacNodeId dst = 0)
    {
        srcId = src;
        dstId = dst;
    }
    bool isMulticast() const
    {
        return (dstId == 0);
    }
};

typedef std::map<CGVertex, std::map<CGVertex, bool> > CGMatrix;
class LteCellInfo;
class LteMacEnbD2D;

/*! \class ConflictGraph ConflictGraph.h
 *  \brief Define the manager of the conflict graph (CG) for resource allocation purposes.
 *
 *  This module maintains all the information about the building of the conflict graph among UEs
 *  for enabling frequency reuse during resource allocation of D2D-capable UEs (i.e. conflicting
 *  UEs should not be allocated on the same resource block).
 *  This module builds a directed CG where vertices are UEs and there is an edge between UE a and
 *  UE b when the power perceived by b from a is above a certain threshold.
 */
class ConflictGraph
{

protected:

    // reference to the MAC layer
    LteMacEnbD2D *macEnb_;

    // Reference to the LteCellInfo
    LteCellInfo *cellInfo_;

    // Conflict Graph
    CGMatrix conflictGraph_;

    // flag for enabling/disabling sharing models
    bool reuseD2D_;
    bool reuseD2DMulti_;

    // reset Conflict Graph
    void clearConflictGraph();

    virtual void findVertices(std::vector<CGVertex>& vertices) = 0;
    virtual void findEdges(const std::vector<CGVertex>& vertices) = 0;

public:
   
    ConflictGraph(LteMacEnbD2D* macEnb, bool reuseD2D, bool reuseD2DMulti);
    virtual ~ConflictGraph();

    // compute Conflict Graph
    void computeConflictGraph();

    // print Conflict Graph - for debug
    void printConflictGraph();

    const CGMatrix* getConflictGraph() { return &conflictGraph_; }
};

#endif	/* CONFLICTGRAPH_H */

