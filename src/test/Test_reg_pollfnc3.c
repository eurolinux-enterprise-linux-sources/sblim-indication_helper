/**
 * $Id: Test_reg_pollfnc3.c,v 1.5 2009/09/23 17:34:30 tyreld Exp $
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

IndErrorT test1(CMPIData *v);
IndErrorT test2(CMPIData *v);


IndErrorT test1(CMPIData *v)
{

  return IND_OK;
}

IndErrorT test2(CMPIData *v)
{

  return IND_OK;
}

int main (void ) {

  int loops = 0;
  /* The loop is used to check for memory leaks. 
     Run this test-case under Valgrind to see if
     there are any leaks */
  for (loops =0; loops < 5; loops++) 
    {
  /* Register */
  assert ( ind_reg_pollfnc("test1", "A", test1, 10, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_reg_pollfnc("test1", "A", NULL, 10,IND_BEHAVIOR_LEVEL_EDGE) == IND_DUPLICATE);

  assert ( ind_reg_pollfnc("test1" ,"B", test1, 20,IND_BEHAVIOR_AGAINST_ZERO) == IND_OK);

  assert ( ind_reg_pollfnc("test1", "C", test1, 30,IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_reg_pollfnc("test2", "D", test2, 2,IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);


  /* Modify the function */
  
  assert ( ind_set_pollfnc("test1","B", test2) == IND_OK);
  
  assert ( ind_set_pollfnc("test1","", test2) == IND_INVALID_ARGS);

  assert ( ind_set_pollfnc_ns("", "test1", "", test2) == IND_INVALID_ARGS);

  assert ( ind_set_pollfnc_ns("root/cimv2", "test1", "C", test2) == IND_OK);

  /* Modify the timer */

  assert ( ind_set_timer("TEST","A", 40) == IND_NOT_FOUND);
  assert ( ind_set_timer("test2","D", 40) == IND_OK);

  assert ( ind_set_timer("test1","", 10) == IND_OK);

  /* Remove them */

  assert ( ind_unreg_pollfnc("test1","B") == IND_OK);
  assert ( ind_unreg_pollfnc("test3","F") == IND_NOT_FOUND);
  assert ( ind_unreg_pollfnc("test1","A") == IND_OK);
  assert ( ind_unreg_pollfnc("test1","B") == IND_NOT_FOUND);
  assert ( ind_unreg_pollfnc("test1","C") == IND_OK);


  /* Modify removed functions */

  assert ( ind_set_timer("test2","D", 40) == IND_OK);
  assert ( ind_unreg_pollfnc("test2","D") == IND_OK);
  assert ( ind_set_timer("test2","D", 40) == IND_NOT_FOUND);
    
    }

  assert ( ind_reg_pollfnc("test1", "A", test1, 10, IND_BEHAVIOR_AGAINST_ZERO) == IND_OK);

  assert ( ind_reg_pollfnc("test1" ,"B", test1, 20, IND_BEHAVIOR_AGAINST_ZERO) == IND_OK);

  assert ( ind_reg_pollfnc("test1", "C", test1, 30, IND_BEHAVIOR_AGAINST_ZERO) == IND_OK);

  assert ( ind_reg_pollfnc("test2", "D", test2, 2, IND_BEHAVIOR_AGAINST_ZERO) == IND_OK);
 /* They should be deleted in order of : D A B C */
  assert ( ind_shutdown() == IND_OK);
  printf("+++ %s succedeed\n",__FILE__);

  return 0;
}
