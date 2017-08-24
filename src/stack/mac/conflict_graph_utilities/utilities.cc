
/** \file utilities.cc
 *  \brief Definition of utility function
 */

#include "stack/mac/conflict_graph_utilities/utilities.h"
#include <string.h>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <iomanip>


MacNodeId indexToNodeId(int index)
{
    return 1025+index;
}

int nodeIdToIndex(MacNodeId nodeId)
{
    return nodeId-1025;
}

