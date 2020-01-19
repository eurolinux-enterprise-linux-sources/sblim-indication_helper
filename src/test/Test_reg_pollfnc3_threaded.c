/**
 * $Id: Test_reg_pollfnc3_threaded.c,v 1.3 2005/05/17 17:20:31 konradrzeszutek Exp $
 * 
 * (C) Copyright IBM Corp. 2004
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
 *
 * Author:       Konrad Rzeszutek <konradr@us.ibm.com>
 * Date  :	      09/20/2004
 */
#include <ind_helper.h>
#include <cmpidt.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

IndErrorT test1(CMPIData *v);


void *unreg_pollfuncs(void *arg);
void *reg_pollfuncs(void *arg);
void *set_pollfuncs(void *arg);


int debug = 0;
int loop_reg = 1;
pthread_t reg_thread_id;
int loop_set = 1;
pthread_t set_thread_id;
int loop_unreg = 1;
pthread_t unreg_thread_id;

static int rc = 0;
#define NR 100


IndErrorT test1(CMPIData *v)
{

  return IND_OK;
}


void *reg_pollfuncs(void *arg)
{
  int i;

  char pn[5];
  for (i = 0; i< NR; i++)
    {
      
      snprintf(pn, 100,"A%d", i);
      if (ind_reg_pollfnc("test1", pn, test1, 1,IND_BEHAVIOR_LEVEL_EDGE) == IND_MUTEX) {
      	rc = IND_MUTEX;
      	return NULL;
      }	
      if (debug)
	fprintf(stderr,"+");
    }
  return NULL;
}

void *set_pollfuncs(void *arg)
{
  int seed;
  int val;
  char pn[5];

  while (loop_set)
    {
      val=1+(int) (rand_r(&seed) % NR);
      snprintf(pn, 100,"A%d", val);
      if (debug)
	fprintf(stderr,".");

      if (ind_set_timer("test1",pn, 1) == IND_MUTEX)
      	{
		rc = IND_MUTEX;
		return NULL;
	}
    }
  return NULL;
}

void *unreg_pollfuncs(void *arg)

{
  int seed;
  int val;
  char pn[5];
  while (loop_set)
    {
      val=1+(int) (rand_r(&seed) % NR);
      snprintf(pn, 100,"A%d", val);
      if (debug)
	fprintf(stderr,"-");

      if (ind_unreg_pollfnc("test1",pn) == IND_MUTEX)
        {
	  rc= IND_MUTEX;
	  return NULL;
	}
    }
  return NULL;
}
int main (void ) {

  /*
   Tests possible dead-lock situations in adding, modifying
   and removing properties. 


   Create three threads. First puts new properties on,
   second modifies them, and third deletes them. The
   properties are in increasing order created, 
   randomly deleted and modified.

   Note: If you run this under valgrind it might tell you 
   have a memory leak - b/c you might allocate more than
   you de-allocate. However, the 'ind_shutdown' functions
   takes care of that and cleans up any remaining properties
   that might have not been removed.
  */
  if (getenv("DEBUG"))
  	debug = 1;
  
  pthread_create(&reg_thread_id,
		 NULL,
		 reg_pollfuncs,
		 NULL);
  
  pthread_create(&set_thread_id,
		 NULL,
		 set_pollfuncs,
		 NULL);
  
  pthread_create(&unreg_thread_id,
		 NULL,
		 unreg_pollfuncs,
		 NULL);
  
  if (debug)
    fprintf(stderr,"Sleeping for 4 seconds\n"); 
  sleep (4);
  loop_set = 0;
  loop_unreg = 0;
  loop_reg = 0;

  pthread_join(reg_thread_id, NULL);
  pthread_join(set_thread_id, NULL);
  pthread_join(unreg_thread_id, NULL);

  ind_shutdown();
  printf("+++ %s succedeed\n",__FILE__);

  return 0;
}
