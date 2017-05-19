// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

#include "stack/pdcp_rrc/ConnectionsTable.h"

ConnectionsTable::ConnectionsTable()
{
    // Table is resetted by putting all fields equal to 0xFF
    memset(ht_, 0xFF, sizeof(struct entry_) * TABLE_SIZE);
}

unsigned int ConnectionsTable::hash_func(uint32_t srcAddr, uint32_t dstAddr,
    uint16_t srcPort, uint16_t dstPort)
{
    return (srcPort | dstPort | srcAddr | dstAddr) % TABLE_SIZE;
}

unsigned int ConnectionsTable::hash_func(uint32_t srcAddr, uint32_t dstAddr,
    uint16_t srcPort, uint16_t dstPort, uint16_t dir)
{
    return (srcPort | dstPort | srcAddr | dstAddr | dir) % TABLE_SIZE;
}

LogicalCid ConnectionsTable::find_entry(uint32_t srcAddr, uint32_t dstAddr,
    uint16_t srcPort, uint16_t dstPort)
{
    int hashIndex = hash_func(srcAddr, dstAddr, srcPort, dstPort);
    while (1)
    {
        if (ht_[hashIndex].lcid_ == 0xFFFF)            // Entry not found
            return 0xFFFF;
        if (ht_[hashIndex].srcAddr_ == srcAddr &&
            ht_[hashIndex].dstAddr_ == dstAddr &&
            ht_[hashIndex].srcPort_ == srcPort &&
            ht_[hashIndex].dstPort_ == dstPort)
            return ht_[hashIndex].lcid_;                // Entry found
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    }
}

LogicalCid ConnectionsTable::find_entry(uint32_t srcAddr, uint32_t dstAddr,
    uint16_t srcPort, uint16_t dstPort, uint16_t dir)
{
    int hashIndex = hash_func(srcAddr, dstAddr, srcPort, dstPort, dir);
    while (1)
    {
        if (ht_[hashIndex].lcid_ == 0xFFFF)            // Entry not found
            return 0xFFFF;
        if (ht_[hashIndex].srcAddr_ == srcAddr &&
            ht_[hashIndex].dstAddr_ == dstAddr &&
            ht_[hashIndex].srcPort_ == srcPort &&
            ht_[hashIndex].dstPort_ == dstPort &&
            ht_[hashIndex].dir_ == dir)
            return ht_[hashIndex].lcid_;                // Entry found
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    }
}

void ConnectionsTable::create_entry(uint32_t srcAddr, uint32_t dstAddr,
    uint16_t srcPort, uint16_t dstPort, LogicalCid lcid)
{
    int hashIndex = hash_func(srcAddr, dstAddr, srcPort, dstPort);
    while (ht_[hashIndex].lcid_ != 0xFFFF)
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    ht_[hashIndex].srcAddr_ = srcAddr;
    ht_[hashIndex].dstAddr_ = dstAddr;
    ht_[hashIndex].srcPort_ = srcPort;
    ht_[hashIndex].dstPort_ = dstPort;
    ht_[hashIndex].lcid_ = lcid;
    return;
}

void ConnectionsTable::create_entry(uint32_t srcAddr, uint32_t dstAddr,
    uint16_t srcPort, uint16_t dstPort, uint16_t dir, LogicalCid lcid)
{
    int hashIndex = hash_func(srcAddr, dstAddr, srcPort, dstPort, dir);
    while (ht_[hashIndex].lcid_ != 0xFFFF)
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    ht_[hashIndex].srcAddr_ = srcAddr;
    ht_[hashIndex].dstAddr_ = dstAddr;
    ht_[hashIndex].srcPort_ = srcPort;
    ht_[hashIndex].dstPort_ = dstPort;
    ht_[hashIndex].dir_ = dir;
    ht_[hashIndex].lcid_ = lcid;
    return;
}

ConnectionsTable::~ConnectionsTable()
{
    memset(ht_, 0xFF, sizeof(struct entry_) * TABLE_SIZE);
}
