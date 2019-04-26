#ifndef PROFILE_H
#define PROFILE_H
#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>


enum funNames
{
	MPI_INIT = 0,
	MPI_SEND,
	MPI_RECV,
	MPI_SCATTER,
	MPI_GATHER,
	MPI_REDUCE,
	MPI_FINISH,
	FUNCTIONS

};

enum funType
{
	NONE = 0,
	PT2PT,
	COLL
};

extern int MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE;

typedef struct {
	double startTime;
	double totalTime;
	long int totalBytes;
	long int call_count;
}LOG;

typedef struct 
{
	char * f_name;
	int type; // 0: , 1: pt2pt, 2: coll
}functions;


typedef struct
{
	char *mName;
	int rank;
	int link;
	long dataSize;
	long sTime;
	long eTime;
	char *fName;
}DataMovementInfo;

void INIT_PROFILE();
void START_PROFILE(int i);
void STOP_PROFILE(int i,long int t_data);
void FINISH_PROFILE();
void Handle_MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                       MPI_Comm comm);

FILE*  profile_file;

//extern void (*MPIC_Send_Hook) (int count, MPI_Datatype type, int dst);
extern void (*MPIC_Send_Hook) (double timediff);

//extern void (*MPIC_Recv_Hook) (int count, MPI_Datatype type, int src);
#endif

