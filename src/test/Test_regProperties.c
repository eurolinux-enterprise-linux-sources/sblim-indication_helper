/**
 * $Id: Test_regProperties.c,v 1.2 2005/03/22 20:07:04 konradrzeszutek Exp $
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

#include <stdio.h>
#include <ind_helper.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define NR 10
#define MAX 20
#define CLASS __FILE__
#define NAMESPACE "test6"
int debug = 0;
IndErrorT test_func (CMPIData *d);

IndErrorT test_func (CMPIData *d)
{

 return IND_OK;
}
int main (void )
{
  int i;
  char temp[MAX];
  char *pn[NR];

  IndErrorT (*data[NR]) (CMPIData *);

  if (getenv("DEBUG"))
      debug = 1;

  /* Create the properties */
  for (i = 0; i< NR; i++)
    {
      data[i]= test_func;
      
      memset(&temp, 0x00, MAX-1);
      snprintf(temp, MAX, "%s_%d", "test6", i);
      
      pn[i] = (char *)strdup(temp);
      
      if (debug)
	fprintf(stderr,"Adding:%s\n",pn[i]);
    }

  assert ( IND_INVALID_ARGS == ind_set_properties_f("",
						    CLASS, 
						    pn, 
							data,
						    NR));
  
  assert ( IND_OK == ind_set_properties_f(NAMESPACE,
					  CLASS, 
					  pn, 
					  data,
					  NR));

  /* Try to unregister class in a wrong namespace */
  
  assert ( IND_INVALID_ARGS == ind_unreg_properties(NULL,
					       CLASS,
					       pn,
					       NR));

  assert ( IND_INVALID_ARGS == ind_unreg_properties("",
					       CLASS,
					       pn,
					       NR));

  /* Ok, unregister them */
  assert ( IND_OK == ind_unreg_properties(NAMESPACE,
					  CLASS,
					  pn,
					  NR));
  for (i = 0; i < NR; i++)
    {
      
      free(pn[i]);
    }

  assert ( IND_OK == ind_shutdown()); 

  printf("+++ %s succeeded\n", __FILE__);
  return 0;
}
