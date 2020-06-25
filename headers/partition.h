// _____________________________________________________________________________________________________________________________________________
// PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION
// _____________________________________________________________________________________________________________________________________________
// PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION
// _____________________________________________________________________________________________________________________________________________
// PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION

#pragma once
#include "!global.h"


// user accessible class that represents a disk partition
// WARNING: do not reorder methods in this class, since it is already compiled! (the virtual method calls won't be correct since the vtable layout is fixed due to being already compiled with the previous virtual method layout)
class Partition
{
public:
    // construct partition with preferences given in prefsfile
    Partition(char* prefsfile);

    // get the number of clusters in the partition
    virtual ClusterNo getNumOfClusters() const;

    // copy the contents from the cluster with given id to the given buffer, if successful returns MFS_PART_OK, otherwise MFS_PART_ERR
    virtual int readCluster(ClusterNo id, char* buffer);
    // copy the contents from the given buffer into the cluster with given id, if successful returns MFS_PART_OK, otherwise MFS_PART_ERR
    virtual int writeCluster(ClusterNo id, const char* buffer);

    // destruct partition 
    virtual ~Partition();

private:
    // private (kernel) implementation of the disk partition
    // this class is a wrapper to the kernel's implementation
    class PartitionImpl* myImpl;
};
