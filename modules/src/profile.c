#include "profile.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

char * fname;
static LOG function_logs[FUNCTIONS];
functions MPI_Functions[FUNCTIONS] = 	{
							{"MPI_Init", NONE},
							{"MPI_Send", PT2PT},
							{"MPI_Recv", PT2PT},
							{"MPI_Scatter", COLL},
							{"MPI_Gather", COLL},
							{"MPI_Reduce", COLL},
							{"MPI_Finish", NONE}
					};
static int size, rank, i;
static char machineName[256];
static int linkNum = 0;
static DataMovementInfo dmi[4096] = {0};
static FILE *dmiFP = NULL;
static int dmiIdx = 0;

void Handle_MPIC_Send(double timediff);
void Handle_MPIC_Recv(int count, MPI_Datatype type, int src);

void getProcInfo()
{
	PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
	PMPI_Comm_size(MPI_COMM_WORLD, &size);

        int x = 256;
        MPI_Get_processor_name(machineName, &x);
	linkNum = atoi(machineName + 5);
}

void openFiles()
{
	char filename[256];
	snprintf(filename, 256, "mpi_log_%d", rank);
       	profile_file = fopen(filename, "w");
       	if(profile_file == NULL)
	{
           	printf("Open profile file failed\n");
           	exit(0);
       	}

	snprintf(filename, 256, "data_movement_info_%d", rank);
       	dmiFP = fopen(filename, "w");
       	if(dmiFP == NULL)
	{
           	printf("Open profile file failed dmifp\n");
           	exit(0);
       	}
}

void closeFiles()
{
	if(profile_file)	fclose(profile_file);
	if(dmiFP)		fclose(dmiFP);
	profile_file = NULL;
	dmiFP = NULL;
}

void flushLogs()
{
	fprintf(profile_file, "%s\n","----------------------------------------------------------------------------" );
	fprintf(profile_file, "|%20s|%15s|%15s|%21s|\n","Function name","count","time","data");
	fprintf(profile_file, "%s\n","----------------------------------------------------------------------------" );
	for (int i = 0; i < FUNCTIONS; ++i)
	{
		if (function_logs[i].call_count > 0 )
		{
			fprintf(profile_file,"|%20s|%15ld|%15lf|%15ld Bytes|\n", MPI_Functions[i].f_name,function_logs[i].call_count,
										function_logs[i].totalTime,function_logs[i].totalBytes);
		}
	}
	fprintf(profile_file, "%s\n","----------------------------------------------------------------------------" );
}

void flushDMI()
{
	if(dmiFP == NULL)	return;

	for(int i = 0; i < dmiIdx; i++)
	{
		fprintf(dmiFP, "%s,%d,%d,%ld,%ld,%ld,%s\n", dmi[i].mName, dmi[i].rank, dmi[i].link, 
				dmi[i].dataSize, dmi[i].sTime, dmi[i].eTime, dmi[i].fName);
	}

	dmiIdx = 0;
}

void resetLogs()
{
	for(int i = 0; i < FUNCTIONS; ++i)
	{
		function_logs[i].startTime = 0.0;
		function_logs[i].totalTime = 0.0;
		function_logs[i].totalBytes = 0;
		function_logs[i].call_count = 0;
	}		
}

void writeLogs(int fIdx, long int dataSize)
{
	double ttime = MPI_Wtime() - function_logs[i].startTime;
	function_logs[fIdx].totalTime += ttime;
	function_logs[fIdx].totalBytes += dataSize;
	function_logs[fIdx].call_count += 1;
}

void writeDMI(char *mName, int rank, int link, long dataSize, long sTime, long eTime, char *fName)
{
	dmi[dmiIdx].mName = mName;
	dmi[dmiIdx].rank = rank;
	dmi[dmiIdx].link = link;
	dmi[dmiIdx].dataSize = dataSize;
	dmi[dmiIdx].sTime = sTime;
	dmi[dmiIdx].eTime = eTime;
	dmi[dmiIdx].fName = fName;

	dmiIdx++;

	if(dmiIdx >= 4096)	flushDMI();

}

void INIT_PROFILE()
{
	MPIC_Send_Hook = &Handle_MPIC_Send;
	//MPIC_Recv_Hook = &Handle_MPIC_Recv;
	getProcInfo();
	resetLogs();
	openFiles();
}

void START_PROFILE(int i)
{
	fname = MPI_Functions[i].f_name;
	//printf("profiling start\n");
	function_logs[i].startTime = MPI_Wtime();
}

void STOP_PROFILE(int fIdx, long int dataSize)
{
	//fprintf(profile_file,"MPI_Send: total bytes sent: %5d, total time taken: %10lf, %5d times called.\n",totalBytes,totalTime,++call_count);
	//fprintf(profile_file,"MPI_Scatter:  total bytes sent: %5d, total time taken: %10lf, %5d times called.\n",totalBytes,totalTime,++call_count);
	//printf("profiling stop\n");

	writeLogs(fIdx, dataSize);
	writeDMI(machineName, rank, linkNum, dataSize, 
			function_logs[fIdx].startTime, MPI_Wtime(), MPI_Functions[fIdx].f_name);
	fname = NULL;
}

void FINISH_PROFILE()
{
	//printf("profiling finished\n");
	flushLogs();
	flushDMI();
	closeFiles();
}


void Handle_MPIC_Send(double timediff)
{
	printf("Htemml [%lf]\n",timediff);
    FILE *filedesc = fopen("/home/connoisseur/logs.txt", "a");
	printf("Opend\n");

    if(!filedesc)
        return;
 
	fprintf(filedesc,"[%s] : [%lf]\n",fname,timediff);
	//fclose(filedesc);

	// int type_size = 0;
	// MPI_Type_size(type, &type_size);
	// writeDMI(machineName, rank, linkNum, type_size * count, 0, 0, "MPI_Scatter1");
}

void Handle_MPIC_Recv(int count, MPI_Datatype type, int dst)
{
	int type_size = 0;
	MPI_Type_size(type, &type_size);
	writeDMI(machineName, rank, linkNum, type_size * count, 0, 0, "MPI_Scatter2");
}

void Handle_MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                       MPI_Comm comm)
{
    MPI_Status status;
    MPI_Aint   extent=0, lb = 0;
    int rank, comm_size, is_homogeneous, sendtype_size;
    int curr_cnt, relative_rank, nbytes, send_subtree_cnt;
    int mask, recvtype_size=0, src, dst;
    int tmp_buf_size = 0;

    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &rank);

    if ( ((rank == root) && (sendcount == 0)) ||
         ((rank != root) && (recvcount == 0)) )
        return;

    is_homogeneous = 1;
    MPI_Type_get_extent(sendtype, &lb, &extent);
    MPI_Type_size(sendtype, &sendtype_size);
    MPI_Type_size(recvtype, &recvtype_size);
    
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;
    
    
    if (is_homogeneous) 
    {
        /* communicator is homogeneous */
        if (rank == root) 
	{
            /* We separate the two cases (root and non-root) because
               in the event of recvbuf=MPI_IN_PLACE on the root,
               recvcount and recvtype are not valid */
            nbytes = sendtype_size * sendcount;
        }
        else 
	{
            nbytes = recvtype_size * recvcount;
        }
        
        curr_cnt = 0;
        
        /* all even nodes other than root need a temporary buffer to
           receive data of max size (nbytes*comm_size)/2 */
        if (relative_rank && !(relative_rank % 2))
	    tmp_buf_size = (nbytes * comm_size)/2;
        
        /* if the root is not rank 0, we reorder the sendbuf in order of
           relative ranks and copy it into a temporary buffer, so that
           all the sends from the root are contiguous and in the right
           order. */
        if (rank == root) 
	{
            if (root != 0)	curr_cnt = nbytes * comm_size;
            else 		curr_cnt = sendcount * comm_size;
        }
        
        /* root has all the data; others have zero so far */
        
	int rcv;
	MPI_Status status;
        mask = 0x1;
        while (mask < comm_size) 
	{
            if (relative_rank & mask) 
	    {
                src = rank - mask; 
                if (src < 0) src += comm_size;
                
                /* The leaf nodes receive directly into recvbuf because
                   they don't have to forward data to anyone. Others
                   receive data into a temporary buffer. */
                if (relative_rank % 2) 
		{
		    PMPI_Recv(&rcv, 1, MPI_INT, src, 1, comm, &status);
		    writeDMI(machineName, rank, linkNum, recvtype_size * recvcount, 0, 0, "MPI_Scatter1");
                }
                else 
		{

		    PMPI_Recv(&rcv, 1, MPI_INT, src, 1, comm, &status);
		    curr_cnt = rcv;
		    writeDMI(machineName, rank, linkNum, rcv, 0, 0, "MPI_Scatter2");
                    /* the recv size is larger than what may be sent in
                           some cases. query amount of data actually received */
		    //TODO Krishna
                    //MPIR_Get_count_impl(&status, MPI_BYTE, &curr_cnt);
                }
                break;
            }
            mask <<= 1;
        }
        
        /* This process is responsible for all processes that have bits
           set from the LSB upto (but not including) mask.  Because of
           the "not including", we start by shifting mask back down
           one. */
        
	int send;
        mask >>= 1;
        while (mask > 0) 
	{
            if (relative_rank + mask < comm_size) 
	    {
                dst = rank + mask;
                if (dst >= comm_size) dst -= comm_size;
                
                if ((rank == root) && (root == 0))
		{
                    send_subtree_cnt = curr_cnt - sendcount * mask;
		    writeDMI(machineName, rank, linkNum, sendtype_size * send_subtree_cnt, 0, 0, "MPI_Scatter3");
		    send = sendtype_size * send_subtree_cnt;
		    PMPI_Send(&send, 1, MPI_INT, dst, 1, comm);
                }
                else
		{
                    /* non-zero root and others */
                    send_subtree_cnt = curr_cnt - nbytes*mask; 
		    writeDMI(machineName, rank, linkNum, 1 * send_subtree_cnt, 0, 0, "MPI_Scatter4");  //Sending in byte
		    send = send_subtree_cnt;
		    PMPI_Send(&send, 1, MPI_INT, dst, 1, comm);
                }
                curr_cnt -= send_subtree_cnt;
            }
            mask >>= 1;
        }
    }
}
