/**
 * $Id: Test_all_functions.c,v 1.4 2009/09/23 16:58:50 tyreld Exp $
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

#include <stdio.h>
#include <ind_helper.h>
#include <assert.h>

int main (int i,char **args)
{

  /* This test does nothing spectacular, just calls each of the
     interface functions to make sure that they are implemented
     and return the right error code. */

  assert ( IND_INVALID_ARGS == ind_reg(NULL, NULL));
  /* Doing crazy stuff with the start/stop/shutdown functions */
  assert ( IND_OK == ind_start());
  assert ( IND_OK == ind_stop());
  assert ( IND_OK == ind_shutdown());
  assert ( IND_OK == ind_stop());
  assert ( IND_OK == ind_shutdown());
  assert ( IND_OK == ind_start());
  assert ( IND_OK == ind_start());
  assert ( IND_OK == ind_shutdown());

  /* Monitored */
  assert ( IND_INVALID_ARGS  == ind_reg_pollfnc("","",NULL, 0,IND_BEHAVIOR_LEVEL_EDGE ));
  assert ( IND_INVALID_ARGS  == ind_reg_pollfnc_ns("","","",NULL, 0, IND_BEHAVIOR_AGAINST_ZERO));

  assert ( IND_INVALID_ARGS == ind_unreg_pollfnc("",""));
  assert ( IND_INVALID_ARGS == ind_unreg_pollfnc_ns("","",""));

  assert ( IND_INVALID_ARGS == ind_set_pollfnc("","",NULL));
  assert ( IND_INVALID_ARGS == ind_set_pollfnc_ns("","","",NULL));

  assert ( IND_INVALID_ARGS == ind_set_timer("","",0));
  assert ( IND_INVALID_ARGS == ind_set_timer_ns("","","",0));

  /* Indication */
  assert ( IND_INVALID_ARGS == ind_set_property_f("","","", NULL));
  assert ( IND_INVALID_ARGS == ind_set_property_d("","","",NULL));

  assert ( IND_INVALID_ARGS == ind_unreg_property("","",""));

  assert ( IND_INVALID_ARGS == ind_set_properties_f("", "", NULL,NULL, 0));
  assert ( IND_INVALID_ARGS == ind_set_properties_d("", "", NULL,NULL, -1));
  assert ( IND_INVALID_ARGS == ind_set_properties_da("", "", NULL,NULL, -1));

  assert ( IND_INVALID_ARGS == ind_unreg_properties("","",NULL, -1));

  /* Tie function */
  assert ( IND_INVALID_ARGS == ind_set_classes("","",""));

  assert ( IND_INVALID_ARGS == ind_set_select("","",NULL));
  assert ( IND_INVALID_ARGS == ind_unreg_select("","",NULL));

  printf("+++ %s succeeded\n", __FILE__);
  return 0;
}
