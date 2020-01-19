/**
 * $Id: Test_setProperties.c,v 1.1.1.1 2005/02/02 19:06:51 darnok-oss Exp $
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define NR 10
#define MAX 20
#define CLASS __FILE__
#define NAMESPACE "Test6"

int debug = 0;

int main (void )
{
  int i;
  char temp[MAX];
  char *pn[NR];
  CMPIData *data[NR];

  if (getenv("DEBUG"))
      debug = 1;

  /* Create the properties */
  for (i = 0; i< NR; i++)
    {
      data[i] = (CMPIData *)malloc(sizeof(CMPIData));
      data[i]->state = CMPI_goodValue;
      data[i]->type = CMPI_uint32;
      data[i]->value.uint32 = 0xCAFEBEEF;
      
      memset(&temp, 0x00, MAX-1);
      snprintf(temp, MAX, "%s_%d", "test6", i);

      pn[i] = (char *)strdup(temp);       

      if (debug)
	fprintf(stderr,"Adding:%s\n", pn[i]);
    }
  /* Try some wrong cases */
  assert ( IND_INVALID_ARGS == ind_set_properties_d("",
						    CLASS,
						    pn, 
						    data, 
						    NR));

  assert ( IND_INVALID_ARGS == ind_set_properties_d(NAMESPACE,
						    NULL,
						    pn, 
						    data, 
						    NR));

  assert ( IND_INVALID_ARGS == ind_set_properties_d(NAMESPACE,
						    "",
						    pn, 
						    data, 
						    NR));
  assert ( IND_INVALID_ARGS == ind_set_properties_d(NAMESPACE,
						    CLASS,
						    pn, 
						    NULL, 
						    NR));


  assert ( IND_OK == ind_set_properties_d(NAMESPACE,
					  CLASS,
					  pn, 
					  data, 
					  NR));

  assert ( IND_OK == ind_unreg_properties(NAMESPACE,
					  CLASS,
					  pn,
					  NR));

  for (i = 0; i < NR; i++)
    {
      free(data[i]);
      free(pn[i]);
      
    }
  assert ( IND_OK == ind_shutdown()); 

  printf("+++ %s succeeded\n", __FILE__);
  return 0;
}
