#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include "pnmpimod.h"
#include "rempi_re.h"
#include "rempi_err.h"
#include "rempi_mem.h"
#include "clmpi.h"

#define  PNMPI_MODULE_REMPI "rempi"

using namespace std;

int rempi_re_cdc::init_clmpi()
{
  int err;
  PNMPI_modHandle_t handle_rempi, handle_clmpi;
  PNMPI_Service_descriptor_t serv;
  /*Load clock-mpi*/
  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_CLMPI, &handle_clmpi);
  if (err!=PNMPI_SUCCESS) {
    return err;
  }
  /*Get clock-mpi service*/
  err=PNMPI_Service_GetServiceByName(handle_clmpi,"clmpi_register_recv_clocks","pi",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  clmpi_register_recv_clocks=(PNMPIMOD_register_recv_clocks_t) ((void*)serv.fct);

  /*Load own moduel*/
  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_REMPI, &handle_rempi);
  if (err!=PNMPI_SUCCESS)
    return err;

  return err;
}

int rempi_re_cdc::re_init(int *argc, char ***argv)
{
  int ret;
  /*Init CLMPI*/
  init_clmpi();

  ret = PMPI_Init(argc, argv);
  /*Init from configuration and for valiables for errors*/
  init_after_pmpi_init(argc, argv);

  return ret;
}

int rempi_re_cdc::re_init_thread(
			     int *argc, char ***argv,
			     int required, int *provided)
{
  int ret;
  /*Init CLMPI*/
  init_clmpi();

  ret = PMPI_Init_thread(argc, argv, required, provided);

  /*Init from configuration and for valiables for errors*/
  init_after_pmpi_init(argc, argv);

  return ret;
}


int rempi_re_cdc::re_irecv(
		 void *buf,
		 int count,
		 MPI_Datatype datatype,
		 int source,
		 int tag,
		 MPI_Comm comm,
		 MPI_Request *request)
{
  int ret;
  ret = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
  return ret;
}
  
int rempi_re_cdc::re_test(
		    MPI_Request *request, 
		    int *flag, 
		    MPI_Status *status)
{
  int ret;
  size_t clock;
  if (status == NULL) {
    /*TODO: allocate array_of_statuses in ReMPI instead of the error below*/
    REMPI_ERR("ReMPI requires status in MPI_Test");
  }
  clmpi_register_recv_clocks(&clock, 1);
  ret = PMPI_Test(request, flag, status);
  if (*flag) {
    REMPI_DBG("Test: Clock %d |%d| (flag: %d)", clock, status->MPI_SOURCE, *flag);
  }
  return ret;
}
  
int rempi_re_cdc::re_testsome(
			  int incount, 
			  MPI_Request array_of_requests[],
			  int *outcount, 
			  int array_of_indices[], 
			  MPI_Status array_of_statuses[])
{
  int ret;
  size_t *clocks;

  if (array_of_statuses == NULL) {
    /*TODO: allocate array_of_statuses in ReMPI instead of the error below*/
    REMPI_ERR("ReMPI requires array_of_statues in MPI_Testsome");
  }
  clocks = (size_t*)malloc(sizeof(size_t) * incount);
  clmpi_register_recv_clocks(clocks, incount);
  ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);

  for (int i = 0; i < *outcount; i++) {
    int source;
    int index = array_of_indices[i];
    source =    array_of_statuses[index].MPI_SOURCE;
    // REMPI_DBG("out: (%lu |%d|) read index: %d", clocks[index], source, index);
    //    REMPI_DBG("clock %lu, source: %d (%d/%d); incount:%d", clocks[index], source, index, *outcount, incount);
    REMPI_DBG(" => Testsome: clock %lu |%d|", clocks[index], source);
    //    REMPI_DBG("source: %d (%d/%d); incount:%d", source, index, *outcount, incount);
  }


  //  REMPI_DBG(" ----------- ");
  free(clocks);
  return ret;
}


int rempi_re_cdc::re_finalize()
{
  int ret;
  ret = PMPI_Finalize();
  return ret;
}



