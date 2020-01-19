/**
 * $Id: Test_genIndication_fail.c,v 1.3 2005/05/17 17:20:31 konradrzeszutek Exp $
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
#include <unistd.h>
#include <string.h>

#ifdef CMPI_VER_100
#define CONST const
#else
#define CONST
#endif

IndErrorT test_uint16(CMPIData *v);
IndErrorT test_string(CMPIData *v);
IndErrorT get_value(CMPIData *v);
CMPIContext* _fake_prepareAttachThread(CONST CMPIBroker* b, CONST CMPIContext* ctx);;
CMPIStatus _fake_attachThread  (CONST CMPIBroker*b, CONST CMPIContext*c);;
CMPIStatus _fake_detachThread (CONST CMPIBroker* mb, CONST CMPIContext* ctx);;
CMPIContext* _fake_prepareAttachThread(CONST CMPIBroker* b, CONST CMPIContext* ctx);
CMPIStatus _fake_attachThread  (CONST CMPIBroker*b, CONST CMPIContext*c);
CMPIStatus _fake_detachThread (CONST CMPIBroker* mb, CONST CMPIContext* ctx);
char * _fake_getCharPtrString  (CONST CMPIString* st, CMPIStatus* rc);
CMPIStatus _fake_releaseString     (CMPIString* st) ;
CMPIString* _fake_cloneString(CONST CMPIString* st, CMPIStatus* rc);
CMPIString* _fake_newString     (CONST CMPIBroker* mb, const char *data, CMPIStatus* rc);
CMPIStatus _fake_release_ObjectPath      (CMPIObjectPath* op);
CMPIObjectPath* _fake_clone_ObjectPath      (CONST CMPIObjectPath* op, CMPIStatus* rc);
CMPIObjectPath* _fake_newObjectPath             (CONST CMPIBroker* mb, const char *ns, const char *cn, CMPIStatus* rc);
CMPIStatus _fake_release_Instance (CMPIInstance* inst);
CMPIStatus _fake_setProperty_Instance      (CONST CMPIInstance* inst, const char *name,       CONST CMPIValue* value, CMPIType type);

CMPIInstance* _fake_clone_Instance      (CONST CMPIInstance* inst, CMPIStatus* rc);
CMPIInstance* _fake_newInstance      (CONST CMPIBroker* mb,CONST CMPIObjectPath* op,CMPIStatus* rc);
CMPIStatus _fake_deliverIndication      (CONST CMPIBroker* mb, CONST CMPIContext* ctx,       const char *ns, CONST CMPIInstance* ind);
CMPIContext* _fake_prepareAttachThread(CONST CMPIBroker* mb, CONST CMPIContext* ctx);
CMPIStatus _fake_attachThread  (CONST CMPIBroker*b, CONST CMPIContext*c);
CMPIStatus _fake_detachThread (CONST CMPIBroker* mb, CONST CMPIContext* ctx);


static CMPIBroker b;
static CMPIContext ctx;
static int debug = 0;
int uint16_v = 10;
int indication_count = 0;
#define MAX 100
#define NAMESPACE "root/cimv2"

/* Test routines. Everytime they are called, the
   output parameter - v- is changed. */
IndErrorT test_uint16(CMPIData *v)
{

  v->type = CMPI_uint16;
  v->state = CMPI_goodValue;
  v->value.uint16 = uint16_v;

  uint16_v ++;

 /* Returning on even numbers a wrong return code causes
    the indication helper to skip creating a indication
    b/c an error has been observed */
  if (uint16_v % 2)
    return IND_OK;
  return -100;
}

IndErrorT test_string(CMPIData *v)
{
  CMPIStatus status;
  char temp[MAX];
  v->type = CMPI_string;
  v->state = CMPI_goodValue;

  snprintf(temp, MAX, "%s_%d", "test_string", uint16_v); 

  v->value.string = 
    CMNewString(&b, temp , &status);

  if (strcmp(temp, CMGetCharPtr(v->value.string)) != 0)
    return -100;

  
  return IND_OK;
}

/* Return the true value */

IndErrorT get_value(CMPIData *v)
{
  v->type = CMPI_uint16;
  v->state = CMPI_goodValue;
  v->value.uint16 = uint16_v;

  /* Change the return code on odd numbers */
  if (uint16_v % 2)
    return IND_OK;
  else
    return IND_INVALID_ARGS;
}
/* Fake CMPI functions. They were written so we don't have to
   depend on a CMPI broker and can use our own. */

CMPIContext context;
CMPIStatus good = {CMPI_RC_OK, NULL};
CMPIStatus bad = {CMPI_RC_ERROR, NULL};

/* Fake CMPIString handlers */
char * _fake_getCharPtrString  (CONST CMPIString* st, CMPIStatus* rc)
{
  /*
  if (debug)
    fprintf(stderr,"_fake_getCharPtrString: %x %x\n", st, rc);
  */
  rc->rc = CMPI_RC_OK;
  rc->msg = NULL;
  return (char *)st->hdl;
}

CMPIStatus _fake_releaseString     (CMPIString* st) 
{
  /*
  if (debug)
    fprintf(stderr,"_fake_releaseString: %x\n", st);
  */
  free(st->hdl);
  free(st->ft);
  free(st);

  return good;
}
CMPIString* _fake_cloneString(CONST CMPIString* st, CMPIStatus* rc)
{

  CMPIString *s = (CMPIString *)malloc( sizeof(CMPIString));
  /*
  if (debug)
    fprintf(stderr,"_fake_cloneString: %x -> %x\n", st, s);

  if (debug)
    fprintf(stderr,"st->hdl: [%s]\n", (char *)st->hdl);
  */
  s->hdl = (void *)strdup((char *)st->hdl);
  s->ft = (CMPIStringFT *) malloc( sizeof (CMPIStringFT));

  s->ft->release = _fake_releaseString;
  s->ft->clone = _fake_cloneString;
  s->ft->getCharPtr = _fake_getCharPtrString;
  rc->rc = CMPI_RC_OK;
  rc->msg = NULL;
  
  rc->rc=CMPI_RC_OK;
  return s;
}

CMPIString* _fake_newString     (CONST CMPIBroker* mb, const char *data, CMPIStatus* rc)
{

  CMPIString *s = (CMPIString *)malloc(sizeof(CMPIString));
  /*
  if (debug)
    fprintf(stderr,"_fake_NewString: %x -> %x\n", data, s);
  */
  s->hdl = (void *)strdup(data);
  /*
  if (debug)
    fprintf(stderr,"s->hdl: [%s]\n", (char *)s->hdl);
  */
  s->ft = (CMPIStringFT *) malloc( sizeof (CMPIStringFT));
  s->ft->release = _fake_releaseString;
  s->ft->clone = _fake_cloneString;
  s->ft->getCharPtr = _fake_getCharPtrString;

  rc->rc = CMPI_RC_OK;
  return s;
}

/* Fake CMPIObjectPath  */

typedef struct
{
  char *ns;
  char *cn;
} _objPath;
CMPIStatus _fake_release_ObjectPath      (CMPIObjectPath* op)
{
  _objPath *objPath;

  objPath = (_objPath *)op->hdl;
  
  free(objPath->ns);
  objPath->ns = NULL;
  free(objPath->cn);
  objPath->cn = NULL;

  free(op->hdl);
  free(op->ft);
	
  free(op);
  return good;
}

CMPIObjectPath* _fake_clone_ObjectPath      (CONST CMPIObjectPath* op, CMPIStatus* rc)
{
  CMPIObjectPath *objpath;
  _objPath *objPathData, *old;
  
  if (debug)
    fprintf(stderr,"_fake_newObjectPath called.\n");
  
  objpath = (CMPIObjectPath *)malloc(sizeof(CMPIObjectPath));
  
  objpath->ft = (CMPIObjectPathFT *) malloc(sizeof(CMPIObjectPathFT));
  objpath->ft->release = _fake_release_ObjectPath;
  objpath->ft->clone = _fake_clone_ObjectPath;
  
  objPathData = (_objPath *) malloc (sizeof (_objPath));
  
  old = (_objPath *)op->hdl;

  objPathData->ns = ( char *) strdup( old->ns);
  objPathData->cn = ( char *)strdup( old->cn);
   
  objpath->hdl = (void*)objPathData;

  rc->rc=CMPI_RC_OK;
  return objpath;
}

CMPIObjectPath* _fake_newObjectPath 		(CONST CMPIBroker* mb, const char *ns, const char *cn, CMPIStatus* rc)
{
   CMPIObjectPath *objpath; 
   _objPath *objPathData;

   if (debug)
	fprintf(stderr,"_fake_newObjectPath called.\n");

   objpath = (CMPIObjectPath *)malloc(sizeof(CMPIObjectPath));

   objpath->ft = (CMPIObjectPathFT *) malloc(sizeof(CMPIObjectPathFT));
   objpath->ft->release = _fake_release_ObjectPath;
   objpath->ft->clone = _fake_clone_ObjectPath;

   objPathData = (_objPath *) malloc (sizeof (_objPath));
   objPathData->ns = ( char *)strdup(ns);
   objPathData->cn = ( char *)strdup(cn);
   
   objpath->hdl = (void*)objPathData;

   rc->rc=CMPI_RC_OK;
   return objpath;
}

/* Fake CMPIInstance */

/* Our data structure to keep info */
typedef struct 
{
   CMPIObjectPath *objPath;
   char *name;
   CMPIData *data;
   void *next;
}_instance;

CMPIStatus _fake_release_Instance (CMPIInstance* inst)
{
  _instance *p, *prev;
  CMPIStatus status;

  if (debug)
    fprintf(stderr,"_fake_release_Instance called.\n");

  /* First release the objectPath (it is only the first item) */
  p = (_instance *)inst->hdl;
  if (p->objPath != NULL) {	
	status = p->objPath->ft->release(p->objPath);
	if (status.rc != CMPI_RC_OK) 
	  {
	    fprintf(stderr,"Problem releasing objectpath!");
	    /* Ouch! */
	    return bad;
	  }
	p->objPath = NULL;
      }

  /* Walk through the list of data */ 

  for (p = prev =(_instance *)inst->hdl; p != NULL;)
    {
      if (p->name != NULL)
	{
	  free(p->name);
	  p->name= NULL;	
	}
      if (p->data != NULL) {
	/* This fake CIMOM only supports String type and nothing else. */
	if (p->data->type == CMPI_string) {
	  status =p->data->value.string->ft->release(p->data->value.string);
	  if (status.rc != CMPI_RC_OK)
	    {
	      fprintf(stderr,"Yikes! Problems releasing string.\n");
	      return bad;
	    }
	}
	free(p->data);
	p->data = NULL;
      }
      prev = p;
      p = p->next;    
      free(prev);
    }
  /* The loop cleaned up the hdl, now clean up the function
     table */
  free(inst->ft);
  free(inst);
  return good;
}
CMPIStatus _fake_setProperty_Instance      (CONST CMPIInstance* inst, const char *name,       CONST CMPIValue* value, CMPIType type)
{
  /* No checking to see if the value exist */
  _instance  *last,*p;
  CMPIData *data;
  CMPIStatus status;
  /* Find the last item */

  if (debug)
    fprintf(stderr,"_fake_setProperty_Instance called.\n");

  for (p = (_instance *)inst->hdl; p->name != NULL; p=p->next)
    ;

  p->next = (_instance *)malloc(sizeof(_instance));
  last = p->next;

  last->name = NULL;
  last->objPath = p->objPath;
  last->next = NULL;
  last->data = NULL;  

  /* Fill with data */
  p->name = ( char *)strdup(name);

  if (debug)
    fprintf(stderr,"-> %s\n", name);
  data = (CMPIData *)malloc(sizeof(CMPIData));
  data->state = CMPI_goodValue;
  /* Copy the unint16 value */
  data->value.uint16 = value->uint16;

  data->type = type;

  /* This next line is ugly. I could have as well just used
     _fake_clone_String, but choose the difficult role */
  if (type == CMPI_string)
    data->value.string = value->string->ft->clone(value->string, &status);

  p->data = data;

  return good;
}

CMPIInstance* _fake_clone_Instance      (CONST CMPIInstance* inst, CMPIStatus* rc)
{
   CMPIInstance *instance;
   _instance *_inst, *_old, *p, *q;
   CMPIObjectPath *objPath = NULL;
   CMPIStatus s;

   if (debug)
	fprintf(stderr,"_fake_cloneInstance called.\n");

   instance = (CMPIInstance *)malloc(sizeof(CMPIInstance));
   instance->ft = (CMPIInstanceFT *) malloc (sizeof(CMPIInstanceFT));

   /* Create the instance data */
   _inst = (_instance *) malloc(sizeof(_instance));
   _old = (_instance *) inst->hdl;
   /* Make a copy of it. Why not reference it? */
   if (_old->objPath != NULL)
     objPath = _old->objPath->ft->clone(_old->objPath, &s);

   /* Start copying the data */
   for (p = _old, q=_inst; p != NULL; p=p->next)
     {
       
       q->objPath = objPath;
       q->name = ( char *)strdup(p->name);

       q->data = (CMPIData *)malloc(sizeof(CMPIData));
       q->data->type = p->data->type;
       q->data->state = p->data->state;
       if (q->data->type == CMPI_string) {
	 /* Wheew, lots of pointer dereferences */
	 q->data->value.string = q->data->value.string->ft->clone(q->data->value.string, &s);
       } else
	 {
	   // Support unint16
	   q->data->value.uint16 = p->data->value.uint16;
	 }
       if (p->next != NULL)
	 q->next = (_instance *) malloc(sizeof(_instance));
       else
	 q->next = NULL;
     }

   instance->hdl = (void *)_inst;
   
   instance->ft->release = _fake_release_Instance;
   instance->ft->clone = _fake_clone_Instance;
   instance->ft->setProperty = _fake_setProperty_Instance; 
    
   rc->rc=CMPI_RC_OK;
   return instance;
}


CMPIInstance* _fake_newInstance      (CONST CMPIBroker* mb,CONST CMPIObjectPath* op,CMPIStatus* rc)
{
   CMPIInstance *inst;
   _instance *instance;
   CMPIStatus s;
   if (debug)
	fprintf(stderr,"_fake_newInstance called.\n");
   inst = (CMPIInstance *)malloc(sizeof(CMPIInstance));
   inst->ft = (CMPIInstanceFT *) malloc (sizeof(CMPIInstanceFT));

   /* Create the instance data */
   instance = (_instance *) malloc(sizeof(_instance));

   instance->objPath = op->ft->clone(op, &s);
   instance->data = NULL;
   instance->name = NULL;
   instance->next = NULL;

   inst->hdl = (void *)instance;
   
   inst->ft->release = _fake_release_Instance;
   inst->ft->clone = _fake_clone_Instance;
   inst->ft->setProperty = _fake_setProperty_Instance;   

   rc->rc=CMPI_RC_OK;
   return inst;
}

/* Fake deliver indication */

CMPIStatus _fake_deliverIndication      (CONST CMPIBroker* mb, CONST CMPIContext* ctx,       const char *ns, CONST CMPIInstance* ind)
{
  _instance *inst, *p;
  CMPIObjectPath *objPath;
  _objPath *obj;

  indication_count ++;
  if (debug) 
   fprintf(stderr,"_fake_deliverIndication called.\n");

  printf("Indication #%d\n", indication_count);
  printf("Namespace:\t%s\n", ns);
  inst = (_instance *)ind->hdl;
  objPath = inst->objPath;
  obj = (_objPath *)objPath->hdl;
  printf("ObjPath:\t%s:%s\n", obj->ns, obj->cn);

  for (p = inst; p->name != NULL; p = p->next) {
    printf("Property:\t%s ", p->name);
    if (p->data->type == CMPI_string)
      printf("[%s]\n", CMGetCharPtr(p->data->value.string));
    if (p->data->type == CMPI_uint16)
      printf("[%d]\n", p->data->value.uint16);
  }


  return good;
}


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

  CMPIData data;
  CMPIStatus status;
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
  b.bft->deliverIndication = _fake_deliverIndication;

  b.eft->newString = _fake_newString;
  b.eft->newObjectPath = _fake_newObjectPath;
  b.eft->newInstance = _fake_newInstance;

  
  /* Register the fake broker */
  assert ( ind_reg(&b, &ctx) == IND_OK);

  /* Register functions with data types */
  assert ( ind_reg_pollfnc_ns(NAMESPACE,"test9-poll", "uint16", test_uint16, 1,IND_BEHAVIOR_AGAINST_ZERO) == IND_OK);

  assert ( ind_reg_pollfnc_ns(NAMESPACE,"test9-poll", "string", test_string, 1,IND_BEHAVIOR_LEVEL_EDGE) == IND_OK);

  /* Register a property for the test9 class */

  assert ( ind_set_property_f(NAMESPACE,"test9-prop","Value" ,get_value) == IND_OK);

  data.type = CMPI_string;
  data.value.string =CMNewString(&b, "localhost" , &status);

  assert ( ind_set_property_d(NAMESPACE,"test9-prop","SystemName" ,&data) == IND_OK);


  CMRelease(data.value.string);
  
  data.type = CMPI_uint16;
  data.value.uint16 = uint16_v;

  assert ( ind_set_property_d(NAMESPACE,"test9-prop","Uint16", &data) == IND_OK);
  
  /*  assert ( ind_set_classes(NAMESPACE,"test9-poll","test9!-prop") == IND_OK);*/

  /* Start and stop the worker */
  assert ( ind_start() == IND_OK );

  printf("Waiting 3 seconds...You will see some errors. Ignore them.");
  sleep (2);

  assert ( ind_stop() == IND_OK );

  /* We won't have any indications b/c we forgot
     to use 'ind_set_classes' routine.*/

  assert ( indication_count == 0 );
  /* Shutdown */
  assert ( ind_shutdown() == IND_OK);

  free(b.eft);
  free(b.bft);

  printf("+++ %s succeeded\n", __FILE__);

  return 0;
}
