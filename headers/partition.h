#pragma once
#include "!global.h"


class Partition {
public:
	Partition(char* prefsfile);   // create partition with preferences given in prefsfile

	virtual ClusterNo getNumOfClusters() const;   // number of clusters in partition

	virtual int readCluster (ClusterNo,       Buffer buffer);   // reads from the cluster with given id to the given buffer, if successful returns 1, otherwise 0
	virtual int writeCluster(ClusterNo, const Buffer buffer);   // writes from given buffer to the cluster with given id,    if successful returns 1, otherwise 0

	virtual ~Partition();   // virtual destructor

private:
	class PartitionImpl *myImpl;   // private implementation of partition
};
