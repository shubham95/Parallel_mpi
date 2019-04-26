#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "profile.h"
#include <math.h>

int MPI_Init( int *argc, char ***argv )
{
	printf("sunil\n");
	int result = PMPI_Init(argc, argv);
	INIT_PROFILE();
	return result;
}

int MPI_Send(const void* buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
	printf("In send haha\n");
	int size;
	long int totalBytes = 0;
	
	START_PROFILE(MPI_SEND);
	
	int result = PMPI_Send(buffer, count, datatype, dest, tag, comm);

	
	PMPI_Type_size(datatype, &size);

	totalBytes += count*size;
		
	STOP_PROFILE(MPI_SEND, totalBytes);
	
	return result;
}

int MPI_Recv(void *buffer, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status * status)
{
	int size;
	long int totalBytes = 0;

	START_PROFILE(MPI_RECV);
	
	int result 		= PMPI_Recv(buffer,count,datatype,source,tag,comm,status);

	PMPI_Type_size(datatype,&size);

	totalBytes += count * size;

	STOP_PROFILE(MPI_RECV, totalBytes);

	return result;
}

/*
int MPIC_Recv(void *buf, int count, int type, int src, int tag, MPID_Comm *comm_ptr, 
		MPI_Status *status, MPIR_Errflag_t *errflag)
{
	PMPIC_Recv(buf, count, type, src, tag, comm_ptr, status, errflag);
}
*/

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
               MPI_Comm comm)
{
	int comm_size;
	int data_size;
	long int totalBytes = 0;

	START_PROFILE(MPI_SCATTER);
	int result		= PMPI_Scatter(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm);
	

	PMPI_Comm_size(comm,&comm_size);
	PMPI_Type_size(sendtype,&data_size);

	//Handle_MPI_Scatter(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm);

	data_size 		*= sendcount;
	for (int j=0; j < ceil(log10(comm_size)/log10(2)); ++j)
	{
		totalBytes 	+= (comm_size/pow(2,j+1)) * (pow(2,j) * data_size);
	}

	
	STOP_PROFILE(MPI_SCATTER, totalBytes);

	return result;
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype,
               int root, MPI_Comm comm)
{

	int comm_size;
	int data_size;
	long int totalBytes = 0;

	START_PROFILE(MPI_GATHER);
	int result = PMPI_Gather(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm);

	PMPI_Comm_size(MPI_COMM_WORLD,&comm_size);
	PMPI_Type_size(sendtype,&data_size);

	data_size 		*= sendcount; 
//	printf("%d %d %d\n",comm_size,data_size,sendcount );

	for (int i = 0; i < ceil(log10(comm_size)/log10(2)); ++i)
	{
		totalBytes += (comm_size/pow(2,i+1)) * (data_size * pow(2,i)) ;
	}

	STOP_PROFILE(MPI_GATHER, totalBytes);

	return result;
}

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm)
{
	int comm_size;
	int data_size;
	long int totalBytes = 0;

	START_PROFILE(MPI_REDUCE);
	int result		= PMPI_Reduce(sendbuf,recvbuf,count,datatype,op,root,comm);
	

	PMPI_Comm_size(MPI_COMM_WORLD,&comm_size);
	PMPI_Type_size(datatype,&data_size);

	data_size *= count;

	for (int i = 0; i < ceil(log10(comm_size)/log10(2)); ++i)
	{
		totalBytes += (comm_size/pow(2,i+1)) * count * data_size;

	}

	STOP_PROFILE(MPI_REDUCE, totalBytes);

	return result;	
}

int MPI_Finalize()
{
	int result;
	
	FINISH_PROFILE();

	result = PMPI_Finalize();
	return result;
}
