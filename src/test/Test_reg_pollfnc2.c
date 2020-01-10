/**
 * $Id: Test_reg_pollfnc2.c,v 1.4 2009/09/23 16:58:50 tyreld Exp $
 * 
 * (C) Copyright IBM Corp. 2004, 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Konrad Rzeszutek <konradr@us.ibm.com>
 * Date  :	      09/20/2004
 */

#include <ind_helper.h>
#include <cmpidt.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef CMPI_VER_100
#define CONST const
#else
#define CONST
#endif
int test1_values = 0;

int debug = 0;
CMPIStatus good = {CMPI_RC_OK, NULL};
CMPIContext context;
CMPIContext* _fake_prepareAttachThread(CONST CMPIBroker* b, CONST CMPIContext* ctx);
CMPIStatus _fake_attachThread  (CONST CMPIBroker*b, CONST CMPIContext*c);
CMPIStatus _fake_detachThread (CONST CMPIBroker* mb, CONST CMPIContext* ctx);
IndErrorT test1(CMPIData *v);
IndErrorT test2(CMPIData *v);

IndErrorT test1(CMPIData *v)
{

  test1_values ++;
  if (debug)
    fprintf(stderr,"test1 called\n");
  if (test1_values > 3)
    ind_stop();
  
  return IND_OK;
}

IndErrorT test2(CMPIData *v)
{
  if (debug)
    fprintf(stderr,"test2 called\n");
  return IND_OK;
}
CMPIContext* _fake_prepareAttachThread(CONST CMPIBroker* b, CONST CMPIContext* ctx)
{
  return &context;
}
CMPIStatus _fake_attachThread  (CONST CMPIBroker*b, CONST CMPIContext*c)
{

  return good;
}

CMPIStatus _fake_detachThread (CONST CMPIBroker* mb, CONST CMPIContext* ctx)
{
  return good;;
}

int main (void ) {

  CMPIBroker b;
  CMPIContext ctx;
  int loop = 0;

  if (getenv("DEBUG"))
    debug = 1;

  /* Set the broker with some fake functions */
  b.bft = (CMPIBrokerFT *) malloc( sizeof (CMPIBrokerFT));
  b.bft->prepareAttachThread = _fake_prepareAttachThread;
  b.bft->attachThread = _fake_attachThread;
  b.bft->detachThread = _fake_detachThread;

  for ( loop = 0, test1_values = 0; loop < 1;test1_values =0, loop++) {
    
  /* Register the fake broker */
  assert ( ind_reg(&b, &ctx) == IND_OK);

  /* Register the test function */
  assert ( ind_reg_pollfnc("test1", "A", test1, 1,IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test1", "B", test2, 1,IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test1", "C", test2, 1,IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( test1_values == 1 );

  /* Start the worker */

  assert ( ind_start() == IND_OK );

  /* Sleep for two seconds, should be enough to call the function
     at least one */

    sleep(4);

  /* No need to stop the worker, the function registerd 
     does that already, but we do it here just to check to see
     if does not complain if you stop it repeadly */

  assert ( ind_stop() == IND_OK );

  assert ( test1_values > 2);

  assert (ind_unreg_pollfnc("test1" ,"A") == IND_OK);
  /* Whoops, we forgot to unregister a function. Let's 
     see if ind_shutdown() de-allocates that function properly */

  //  assert (ind_unreg_pollfnc("test1" ,"B") == IND_OK); 
  //assert (ind_unreg_pollfnc("test1" ,"C") == IND_OK);

  assert ( ind_shutdown() == IND_OK );

  }
  /* Don't forget to de-allocte. No memory leaks here! */
  
  free(b.bft);

  printf("+++ %s succeeded\n", __FILE__);
  return 0;
}
