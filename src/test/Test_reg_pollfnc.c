/**
 * $Id: Test_reg_pollfnc.c,v 1.3 2005/05/17 17:20:31 konradrzeszutek Exp $
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef CMPI_VER_100
#define CONST const
#else
#define CONST 
#endif

static CMPIBroker b;
static CMPIContext ctx;
static int debug = 0;

IndErrorT test_uint16(CMPIData *v);
IndErrorT test_uint64(CMPIData *v);
IndErrorT test_uint32(CMPIData *v);
IndErrorT test_uint8(CMPIData *v);
IndErrorT test_sint64(CMPIData *v);
IndErrorT test_sint32(CMPIData *v);
IndErrorT test_sint16(CMPIData *v);
IndErrorT test_sint8(CMPIData *v);
IndErrorT test_real64(CMPIData *v);
IndErrorT test_real32(CMPIData *v);
IndErrorT test_boolean(CMPIData *v);
IndErrorT test_char16(CMPIData *v);
IndErrorT test_ptr(CMPIData *v);
IndErrorT test_char(CMPIData *v);
IndErrorT test_string(CMPIData *v);


char * _fake_getCharPtrString  (CONST CMPIString* st, CMPIStatus* rc);
CMPIStatus _fake_releaseString     (CMPIString* st) ;
CMPIString* _fake_cloneString(CONST CMPIString* st, CMPIStatus* rc);
CMPIString* _fake_newString     (CONST CMPIBroker* mb, const char *data, CMPIStatus* rc);
CMPIContext* _fake_prepareAttachThread(CONST CMPIBroker* mb, CONST CMPIContext* ctx);
CMPIStatus _fake_attachThread  (CONST CMPIBroker*b, CONST CMPIContext*c);
CMPIStatus _fake_detachThread (CONST CMPIBroker* mb, CONST CMPIContext* ctx);

/* Test routines */
IndErrorT test_uint16(CMPIData *v)
{

  v->type = CMPI_uint16;
  v->state = CMPI_goodValue;
  v->value.uint16 = 0xBEEF;

  return IND_OK;
}
IndErrorT test_uint64(CMPIData *v)
{
  v->type = CMPI_uint64;
  v->state = CMPI_goodValue;
  v->value.uint64 = 0xDEADBEEF;
  return IND_OK;
}
IndErrorT test_uint32(CMPIData *v)
{
  v->type = CMPI_uint32;
  v->state = CMPI_goodValue;
  v->value.uint32 = 0xDEADBEEF;
  return IND_OK;
}
IndErrorT test_uint8(CMPIData *v)
{
  v->type = CMPI_uint8;
  v->state = CMPI_goodValue;
  v->value.uint8 = 0xAB;
  return IND_OK;
}
IndErrorT test_sint64(CMPIData *v)
{
  v->type = CMPI_sint64;
  v->state = CMPI_goodValue;
  v->value.sint64 = -10000000;
  return IND_OK;
}
IndErrorT test_sint32(CMPIData *v)
{
  v->type = CMPI_sint32;
  v->state = CMPI_goodValue;
  v->value.sint32 = -1000;
  return IND_OK;
}
IndErrorT test_sint16(CMPIData *v)
{
  v->type = CMPI_sint16;
  v->state = CMPI_goodValue;
  v->value.sint16 = 0xBEEF;
  return IND_OK;
}
IndErrorT test_sint8(CMPIData *v)
{
  v->type = CMPI_sint8;
  v->state = CMPI_goodValue;
  v->value.sint8 = -100;
  return IND_OK;
}
IndErrorT test_real64(CMPIData *v)
{
  v->type = CMPI_real64;
  v->state = CMPI_goodValue;
  v->value.real64 = 1.2029329019029;
  return IND_OK;
}
IndErrorT test_real32(CMPIData *v)
{
  v->type = CMPI_real32;
  v->state = CMPI_goodValue;
  v->value.real32 = 1.9992;
  return IND_OK;
}
IndErrorT test_boolean(CMPIData *v)
{
  v->type = CMPI_boolean;
  v->state = CMPI_goodValue;
  v->value.boolean = 1;
  return IND_OK;
}
IndErrorT test_char16(CMPIData *v)
{
  v->type = CMPI_char16;
  v->state = CMPI_goodValue;
  v->value.char16 = 'a';
  return IND_OK;
}
IndErrorT test_ptr(CMPIData *v)
{
  v->type = CMPI_ptr;
  v->state = CMPI_goodValue;
  v->value.dataPtr.length = 4;
  v->value.dataPtr.ptr = (void *)strdup("TEST");
  
  return IND_OK;
}

IndErrorT test_char(CMPIData *v)
{

  v->type = CMPI_chars;
  v->state = CMPI_goodValue;
  v->value.chars = (char *)strdup("test");

  return IND_OK;
}

IndErrorT test_string(CMPIData *v)
{
  CMPIStatus status;
  const char *test = "TEST";

  v->type = CMPI_string;
  v->state = CMPI_goodValue;

  v->value.string = 
    CMNewString(&b, test , &status);

  if (strcmp(test, CMGetCharPtr(v->value.string)) != 0)
    return -100;

  if (debug)
    {      
      fprintf(stderr,"New string is: %s\n, ", CMGetCharPtr(v->value.string));
      fprintf(stderr,"CMPIStatus: %d ", status.rc);

    }
  return IND_OK;
}


/* Fake CMPI functions. They were written so we don't have to
   depend on a CMPI broker and can use our own. */

char * _fake_getCharPtrString  (CONST CMPIString* st, CMPIStatus* rc)
{
  if (debug)
    fprintf(stderr,"_fake_getCharPtrString: %p\n", st);

  rc->rc = CMPI_RC_OK;
  rc->msg = NULL;
  return (char *)st->hdl;
}

CMPIStatus _fake_releaseString     (CMPIString* st) 
{
  CMPIStatus status = {CMPI_RC_OK,NULL};

  if (debug)
    fprintf(stderr,"_fake_releaseString: %p\n", st);

  free(st->hdl);
  free(st->ft);
  free(st);

  return status;
}
CMPIString* _fake_cloneString(CONST CMPIString* st, CMPIStatus* rc)
{

  CMPIString *s = (CMPIString *)malloc( sizeof(CMPIString));
  if (debug)
    fprintf(stderr,"_fake_cloneString: %p -> %p\n", st, s);

  if (debug)
    fprintf(stderr,"st->hdl: [%s]\n", (char *)st->hdl);
  s->hdl = (void *)strdup((char *)st->hdl);
  s->ft = (CMPIStringFT *) malloc( sizeof (CMPIStringFT));

  s->ft->release = _fake_releaseString;
  s->ft->clone = _fake_cloneString;
  s->ft->getCharPtr = _fake_getCharPtrString;
  rc->rc = CMPI_RC_OK;
  rc->msg = NULL;

  return s;
}




CMPIString* _fake_newString     (CONST CMPIBroker* mb, const char *data, CMPIStatus* rc)
{

  CMPIString *s = (CMPIString *)malloc(sizeof(CMPIString));

  if (debug)
    fprintf(stderr,"_fake_NewString: %p -> %p\n", data, s);
  s->hdl = (void *)strdup(data);
  if (debug)
    fprintf(stderr,"s->hdl: [%s]\n", (char *)s->hdl);
  s->ft = (CMPIStringFT *) malloc( sizeof (CMPIStringFT));
  s->ft->release = _fake_releaseString;
  s->ft->clone = _fake_cloneString;
  s->ft->getCharPtr = _fake_getCharPtrString;

  return s;
}

CMPIContext context;
CMPIStatus good = {CMPI_RC_OK, NULL};

CMPIContext* _fake_prepareAttachThread(CONST CMPIBroker* mb, CONST CMPIContext* ctx)
{
  if (debug)
    fprintf(stderr,"_fake_prepareAttachThread called.\n");
  return &context;
}

CMPIStatus _fake_attachThread  (CONST CMPIBroker*b, CONST CMPIContext*c)
{
  if (debug)
    fprintf(stderr,"_fake_attachThread called.\n");
  return good;
}

CMPIStatus _fake_detachThread (CONST CMPIBroker* mb, CONST CMPIContext* ctx)
{
  if (debug)
    fprintf(stderr,"_fake_detachThread called.\n");
  return good;
}


int main (void ) {


  /* This test tests the indication helper library handling of
     data types. Whenever a functions is registered, it is
     called to get an initial data value. This test tests three
     different data types - a unsigned integer, a char* and 
     a string type. The string type is an interesting type b/c
     there is a life-cycle associated with it - creating a string
     releasing it and copying it (cloning it). All of these operations
     are performed via macros that expand to calls from functions 
     on function tables.  Set the DEBUG env to get a more feel of 
     what happends */

  if (getenv("DEBUG"))
    debug =1;
  /* Set the broker with some fake functions */
  b.bft = (CMPIBrokerFT *) malloc( sizeof (CMPIBrokerFT));
  b.eft = (CMPIBrokerEncFT *) malloc ( sizeof (CMPIBrokerEncFT));

  b.bft->prepareAttachThread = _fake_prepareAttachThread;
  b.bft->attachThread = _fake_attachThread;
  b.bft->detachThread = _fake_detachThread;

  b.eft->newString = _fake_newString;

  
  /* Register the fake broker */
  assert ( ind_reg(&b, &ctx) == IND_OK);

  /* Register functions with data types */
  assert ( ind_reg_pollfnc("test4", "uint16", test_uint16, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_reg_pollfnc("test4", "char", test_char, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_reg_pollfnc("test4" ,"string", test_string, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_reg_pollfnc("test4" ,"uint64", test_uint64, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test4" ,"uint32", test_uint32, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test4" ,"uint8", test_uint8, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test4" ,"sint64", test_sint64, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test4" ,"sint32", test_sint32, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test4" ,"sint16", test_sint16, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test4" ,"sint8", test_sint8, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_reg_pollfnc("test4" ,"real32", test_real32, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test4" ,"real64", test_real64, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_reg_pollfnc("test4" ,"boolean", test_boolean, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);
  assert ( ind_reg_pollfnc("test4" ,"char16", test_char16, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_reg_pollfnc("test4" ,"ptr", test_ptr, 1, IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  assert ( ind_start() == IND_OK);

  /* This should be enough time to call all of those functions and
     do a comparison */
  sleep (2);

  assert ( ind_stop() == IND_OK);
  /* The shutdown routine should clean up _all_ of the data */
  assert ( ind_shutdown() == IND_OK);

  free(b.eft);
  free(b.bft);
  printf("+++ %s succeeded\n", __FILE__);

  return 0;
}
