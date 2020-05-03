// _____________________________________________________________________________________________________________________________________________
// PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION
// _____________________________________________________________________________________________________________________________________________
// PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION
// _____________________________________________________________________________________________________________________________________________
// PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION...PARTITION

#pragma once
#include "!global.h"


// user accessible class that represents a disk partition
class Partition
{
private:
    // private (kernel) implementation of the disk partition
    // this class is a wrapper to the kernel's implementation
    class PartitionImpl *myImpl;

public:
    // construct partition with preferences given in prefsfile
	Partition(char* prefsfile);

    // copy the contents from the cluster with given id to the given buffer, if successful returns MFS_PART_OK, otherwise MFS_PART_ERR
	virtual int readCluster (ClusterNo,       Buffer buffer);
    // copy the contents from the given buffer into the cluster with given id, if successful returns MFS_PART_OK, otherwise MFS_PART_ERR
	virtual int writeCluster(ClusterNo, const Buffer buffer);

    // get the number of clusters in the partition
	virtual ClusterNo getNumOfClusters() const;

    // destruct partition 
	virtual ~Partition();
};
