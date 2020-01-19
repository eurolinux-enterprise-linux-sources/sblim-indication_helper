/**
 * $Id: Test_addProperty.c,v 1.3 2005/05/17 17:20:31 konradrzeszutek Exp $
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
#include <cmpimacs.h>
#include <cmpift.h>
#include <assert.h>
#include <stdlib.h>

IndErrorT test_BAD(CMPIData *v);
IndErrorT test_BAD2(CMPIData *v);


static int debug = 0;


/* Test routines */
IndErrorT test_BAD(CMPIData *v)
{

  v->type = CMPI_uint16;
  v->state = CMPI_goodValue;
  v->value.uint16 = 0xBEEF;

  return -100;
}

IndErrorT test_BAD2(CMPIData *v)
{
    return IND_OK;
}

int main (void ) {



  if (getenv("DEBUG"))
    debug =1;

  /* Register functions with data types */
  assert ( ind_reg_pollfnc("test5", "test_BAD", test_BAD, 10, IND_BEHAVIOR_LEVEL_EDGE) == -100);
  assert ( ind_reg_pollfnc("test5", "test_BAD2", test_BAD2, 10, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_shutdown() == IND_OK);

  printf("+++ %s succeeded\n", __FILE__);

  return 0;
}
