/**
 * $Id: Test_setProperties_d.c,v 1.1.1.1 2005/02/02 19:06:51 darnok-oss Exp $
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
#include <cmpidt.h>
#include <stdlib.h>
#define NR 10
#define MAX 20
#define CLASS "CIM_AlertIndication"
#define NAMESPACE "root/cimv2"
int debug = 0;

/* Properties taken from CIM_AlertIndication from CIM 2.8 */
char *CIM_AlertIndication[] = 
  {"Description",
   "AlertingManagedElement",
   "AlertingElementFormat",
   "OtherAlertingElementFormat",
   "AlertType",
   "OtherAlertType",
   "PerceivedSeverity",
   "OtherSeverity",
   "ProbableCause",
   "ProbableCauseDescription",
   "Trending",
   "EventID",
   /* "EventTime", */
   "SystemCreationClassName",
   "SystemName",
   "ProviderName"};

CMPIData CIM_AlertIndication_DATA[] = 
  {{.type = CMPI_chars,
   .state = CMPI_goodValue, 
   .value = {   
     .chars = "Startup indication"
   }
  },

   {.type = CMPI_chars,.state=CMPI_goodValue,.value= 
    {.chars = "root/cimv2:CIM_AlertIndication"}},

   {.type = CMPI_uint16,.state=CMPI_goodValue,.value=
    {.uint16 = 2}}, /* CIMObjectPath */

   {.type=CMPI_chars,.state=CMPI_goodValue,.value=
    {.chars = ""}},

   {.type=CMPI_uint16,.state=CMPI_goodValue,.value=
    {.uint16 = 2}}, /* Communications Alert */
   {.type=CMPI_chars, .state=CMPI_goodValue,.value=
    {.chars = ""}},
   {.type=CMPI_uint16, .state=CMPI_goodValue,.value=
    {.uint16= 3}}, /* Information */
   {.type=CMPI_chars, .state=CMPI_goodValue,.value=
    {.chars = ""}},
   {.type=CMPI_uint16, .state=CMPI_goodValue,.value=
    {.uint16 = 2}}, /* Other */
   {.type=CMPI_chars,.state=CMPI_goodValue,.value=
    {.chars = "Test"}},
   {.type=CMPI_uint16,.state=CMPI_goodValue,.value=
    {.uint16 = 0}}, /* Unknown */
   {.type=CMPI_chars,.state=CMPI_goodValue,.value=
    {.chars = ""}},
   {.type=CMPI_chars,.state=CMPI_goodValue,.value=
    {.chars = __FILE__}},
   /* {CMPI_dataTime, ... }  */
   {.type=CMPI_chars,.state=CMPI_goodValue,.value=
    {.chars = CLASS}},
   {.type=CMPI_chars,.state=CMPI_goodValue,.value=
    {.chars = "localhost"}},
   {.type=CMPI_chars,.state=CMPI_goodValue,.value=
    {.chars = __FILE__}}};
   
   
int main (void )
{

  if (getenv("DEBUG"))
      debug = 1;

  /* Whoops, p[] and *d are null */
  assert ( IND_INVALID_ARGS == ind_set_properties_da(NAMESPACE,
					  CLASS, 
					  NULL,
					  NULL,
					  NR));

  /* Whoops, namespace is "" */
  assert ( IND_INVALID_ARGS == ind_set_properties_da("",
					  CLASS, 
					  CIM_AlertIndication,
					  CIM_AlertIndication_DATA,
					  NR));

  /* Whoos, no class name! */
  assert ( IND_INVALID_ARGS == ind_set_properties_da(NAMESPACE,
					  "", 
					  CIM_AlertIndication,
					  CIM_AlertIndication_DATA,
					  NR));

  /* Both Namespace and classname is empty */
  assert ( IND_INVALID_ARGS == ind_set_properties_da("",
					  "", 
					  CIM_AlertIndication,
					  CIM_AlertIndication_DATA,
					  NR));

  /* The properties[] array is null */
  assert ( IND_INVALID_ARGS == ind_set_properties_da(NAMESPACE,
						     CLASS, 
						     NULL,
						     CIM_AlertIndication_DATA,
						     NR));

  /* OK, should be no problems */
  assert ( IND_OK == ind_set_properties_da(NAMESPACE,
					   CLASS, 
					   CIM_AlertIndication,
					   CIM_AlertIndication_DATA,
					   NR));

  
  assert ( IND_OK == ind_unreg_properties(NAMESPACE,
					  CLASS,
					  CIM_AlertIndication,
					  NR));
  assert ( IND_OK == ind_shutdown()); 

  printf("+++ %s succeeded\n", __FILE__);
  return 0;
}
