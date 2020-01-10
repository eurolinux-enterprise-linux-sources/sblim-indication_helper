 /**
 * $Id: ind_helper.c,v 1.13 2009/09/23 16:58:50 tyreld Exp $
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
 *
 */

#include <ind_helper.h>
#include <config.h>
#include <cmpimacs.h>

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#define _GNU_SOURCE 1
#include <string.h>
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif


#if defined(DEBUG)
#define IND_HLP_DEBUG(X) fprintf(stderr, "%s:%d:[%s]\n", __FILE__, __LINE__, X); fflush(stderr);
#else
#define IND_HLP_DEBUG(X)
#endif

#define DEFAULT_NAMESPACE "root/cimv2"

#define IND_BEHAVIOR_NONE 0
/* 
 * Flags used in ind_class_property.flag value 
 */
#define LOCK    0x0002
#define UNLOCK  0x0004
#define FIRST_TIME	0x0001
/**
 * The class properties. This structure is used in three different 
 * ways.
 */

typedef struct
{
  char *ci_cn;                  /* The indication class name. This name is a bit
                                   misleading, as this value  is not used
                                   when ind_set_property_* or ind_set_properties_*
                                   is called, but rather when "tieing" in monitored
                                   class with the indication class */
  CMPISelectExp *filter;        /* The filter used on the indication class
                                   name. */

  char *cn;                     /* Class name */
  char *pn;                     /* Property */
  char *namespace;              /*Namespace */
  CMPIData data;
  IndTimeT timer;               /* The fine-granular time value */
    IndErrorT (*check) (CMPIData *);    /* Function to get data from */
  IndCheckBehaviour logic;      /* The logic of comparing data against the check function */
  int flag;                     /* To see if its locked, unlocked, or triggered */
  void *p;                      /* Pointer to next record */
} ind_class_property;


/**
 * Mutex structures to protect ind_class_property data
 */
typedef struct
{
  pthread_mutex_t mutex;
  ind_class_property *data;
} data_mutex;



/* Internal flags used in 'flag_mutex' data structure */
#define BROKER_NOT_REGISTERED 0x0000
#define BROKER_REGISTERED     0x0001
#define WORKER_START          0x0002
#define WORKER_STOP           0x0004
#define WORKER_END_LIFE       0x0008
#define WORKER_RUNNING        0x0010

/* Data structure used to protect the flag value */
typedef struct
{
  pthread_mutex_t mutex;
  int flag;
} flag_mutex;

/* Data structure with sleep timer */
typedef struct
{
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  struct timespec timer;
} sleep_mutex;


/* Intialized to default values global values. */

static data_mutex monitor_properties = { PTHREAD_MUTEX_INITIALIZER, NULL };
static data_mutex ind_class_properties = { PTHREAD_MUTEX_INITIALIZER, NULL };
static data_mutex ind_filters = { PTHREAD_MUTEX_INITIALIZER, NULL };
static flag_mutex flag = { PTHREAD_MUTEX_INITIALIZER, BROKER_NOT_REGISTERED };
static sleep_mutex sleep =
  { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, {0, 0} };

static pthread_t thread_id;


/* Function prototypes */

/* Functions dealing with CMPI */
static IndErrorT _generateIndication (ind_class_property *,
                                      const CMPIBroker *, const CMPIContext *);

/* Pthread run function */
static void *_worker (void *);

/* Functions operating on data */
static IndErrorT _clean (data_mutex * d);
static IndErrorT _addProperty (data_mutex *,
                               const char *namespace,
                               const char *cn, const char *pn,
                               IndErrorT (*check) (CMPIData *),
                               IndTimeT t, IndCheckBehaviour logic,
                               const CMPISelectExp * filter);

static IndErrorT _deleteProperty (data_mutex * d,
                                  const char *namespace,
                                  const char *cn, const char *pn);

#define CHANGE_TIME 0
#define CHANGE_FUNC 1
#define CHANGE_DATA 2

static IndErrorT _modifyProperty (data_mutex * d,
                                  const char *namespace,
                                  const char *cn, const char *pn,
                                  const int changeAttr,
                                  IndTimeT t,
                                  IndErrorT (*check) (CMPIData *),
                                  const CMPIData * data);

/* Helper functions */

static IndErrorT _cloneData (CMPIData * dst, const CMPIData * sc);

static IndErrorT _releaseData (CMPIData * data);
static IndErrorT _releaseDataValue (CMPIData * data);

static int _compareProperty (void *, void *);
static int _compareCIMValue (CMPIData, CMPIData);

/**
 * -------------------------------------------------
 * The external functions visible to user
 * -------------------------------------------------
 */
IndErrorT
ind_reg (const CMPIBroker * broker, const CMPIContext * ctx)
{

  CMPIContext *context = NULL;
  const void **data;

  IND_HLP_DEBUG ("ind_reg_CMPIBroker called");

  if (broker == NULL)
    return IND_INVALID_ARGS;

  context = CBPrepareAttachThread (broker, ctx);

  if (context == NULL)
    return IND_INVALID_ARGS;

  /* Check to make sure this is can only be called once */
  if ((flag.flag & BROKER_REGISTERED) == BROKER_REGISTERED)
    {
      IND_HLP_DEBUG ("Tried to register twice.");
      return IND_DUPLICATE;
    }

  pthread_mutex_lock (&flag.mutex);
  flag.flag = BROKER_REGISTERED | WORKER_STOP;
  pthread_mutex_unlock (&flag.mutex);

  /* This might seem like a memory leak, since it is not de-allocated
     anywhere in the scope of this function, but it is free-ed in 
     _worker routine. */
  data = (const void **) malloc (sizeof (CMPIBroker *) + sizeof (CMPIContext *));

  if (data == NULL)
    return IND_NO_MEMORY;

  data[0] = broker;
  data[1] = context;


  thread_id = 0;
  pthread_create (&thread_id, NULL, _worker, data);

  IND_HLP_DEBUG ("ind_reg_CMPIBroker exited");

  return IND_OK;
}

IndErrorT
ind_stop (void)
{

  IND_HLP_DEBUG ("ind_stop called.");

  if (pthread_mutex_lock (&flag.mutex) != 0)
    {
      IND_HLP_DEBUG ("Mutex lock error.");
      return IND_MUTEX;
    }
  flag.flag = (flag.flag & (~WORKER_START) & (~WORKER_RUNNING)) | WORKER_STOP;
  if (pthread_mutex_unlock (&flag.mutex) != 0)
    {
      IND_HLP_DEBUG ("Mutex unlock error.");
      return IND_MUTEX;
    }

  IND_HLP_DEBUG ("ind_stop exited.");
  return IND_OK;
}

IndErrorT
ind_start (void)
{

  IND_HLP_DEBUG ("ind_start called.");

  if (pthread_mutex_lock (&flag.mutex) != 0)
    return IND_MUTEX;

  flag.flag = (flag.flag & (~WORKER_STOP)) | WORKER_START;

  if (pthread_mutex_unlock (&flag.mutex) != 0)
    return IND_MUTEX;

  /* Awake the thread */
  if (pthread_cond_signal (&sleep.cond) != 0)
    IND_HLP_DEBUG ("Signal condition error.");

  IND_HLP_DEBUG ("ind_start exited.");
  return IND_OK;
}

IndErrorT
ind_shutdown (void)
{
  int rc = IND_OK;


  IND_HLP_DEBUG
    ("Cleaning up data left by ind_reg_property, ind_set_property or ind_set_properties");

  rc = _clean (&ind_class_properties);
  if (rc != IND_OK)
    return rc;

  IND_HLP_DEBUG ("Cleaninig up data left by ind_reg_pollfnc");
  rc = _clean (&monitor_properties);
  if (rc != IND_OK)
    return rc;

  IND_HLP_DEBUG ("Cleaning up data left by ind_set_filter");
  rc = _clean (&ind_filters);
  if (rc != IND_OK)
    return rc;


  /* The loop (worker) is only activated when the 
     broker is registered */
  if ((flag.flag & BROKER_REGISTERED) == BROKER_REGISTERED)
    {
      IND_HLP_DEBUG ("Terminating worker");
      pthread_mutex_lock (&flag.mutex);
      flag.flag = WORKER_END_LIFE;
      pthread_mutex_unlock (&flag.mutex);

      /* Awake the thread if it sleeps */
      if (pthread_cond_signal (&sleep.cond) != 0)
        IND_HLP_DEBUG ("Signal condition error.");

      /* Join the worker. Make sure it exits */
      pthread_join (thread_id, NULL);

      /* Reset the flag to initial state */
      pthread_mutex_lock (&flag.mutex);
      flag.flag = BROKER_NOT_REGISTERED;
      pthread_mutex_unlock (&flag.mutex);
    }
  IND_HLP_DEBUG ("ind_shutdown exited.");
  return rc;
}



/**
 * +-----------------------------------
 * + 
 * +    Monitored class operations.
 * +
 * +-----------------------------------
 */


IndErrorT
ind_reg_pollfnc (const char *cn,
                 const char *pn,
                 IndErrorT (*check) (CMPIData *),
                 IndTimeT timer, IndCheckBehaviour logic)
{
  return _addProperty (&monitor_properties,
                       DEFAULT_NAMESPACE, cn, pn, check, timer, logic, NULL);
}

IndErrorT
ind_reg_pollfnc_ns (const char *namespace,
                    const char *cn,
                    const char *pn,
                    IndErrorT (*check) (CMPIData *),
                    IndTimeT timer, IndCheckBehaviour logic)
{
  return _addProperty (&monitor_properties, namespace,
                       cn, pn, check, timer, logic, NULL);
}

IndErrorT
ind_unreg_pollfnc (const char *cn, const char *p)
{

  return _deleteProperty (&monitor_properties, DEFAULT_NAMESPACE, cn, p);
}


IndErrorT
ind_unreg_pollfnc_ns (const char *ns, const char *cn, const char *p)
{

  return _deleteProperty (&monitor_properties, ns, cn, p);
}

IndErrorT
ind_set_pollfnc (const char *cn, const char *pn, IndErrorT (*check) (CMPIData *))
{

  if ((check == NULL) || (cn == NULL))
    return IND_INVALID_ARGS;
  if (strlen (pn) == 0)
    return IND_INVALID_ARGS;

  return _modifyProperty (&monitor_properties, DEFAULT_NAMESPACE,
                          cn, pn, CHANGE_FUNC, 0, check, NULL);
}



IndErrorT
ind_set_pollfnc_ns (const char *ns,
                    const char *cn, const char *pn, IndErrorT (*check) (CMPIData *))
{

  if ((check == NULL) || (cn == NULL) || (pn == NULL))
    return IND_INVALID_ARGS;
  if (strlen (pn) == 0)
    return IND_INVALID_ARGS;

  return _modifyProperty (&monitor_properties, ns,
                          cn, pn, CHANGE_FUNC, 0, check, NULL);
}

IndErrorT
ind_set_timer (const char *cn, const char *pn, IndTimeT timer)
{

  return _modifyProperty (&monitor_properties,
                          DEFAULT_NAMESPACE,
                          cn, pn, CHANGE_TIME, timer, NULL, NULL);

}

IndErrorT
ind_set_timer_ns (const char *ns, const char *cn, const char *pn, IndTimeT timer)
{

  return _modifyProperty (&monitor_properties, ns,
                          cn, pn, CHANGE_TIME, timer, NULL, NULL);

}

/**
 * +-----------------------------------
 * + 
 * +    Indication class 
 * +
 * +-----------------------------------
 */

IndErrorT
ind_set_property_f (const char *ns,
                    const char *ci,
                    const char *pn, IndErrorT (*getAlertProperty) (CMPIData *))
{
  return _addProperty (&ind_class_properties, ns,
                       ci, pn, getAlertProperty, 0, IND_BEHAVIOR_NONE, NULL);

}

IndErrorT
ind_set_property_d (const char *ns, const char *cn, const char *pn, const CMPIData * d)
{

  /* First we must add it */
  _addProperty (&ind_class_properties, ns,
                cn, pn, NULL, 0, IND_BEHAVIOR_NONE, NULL);

  return _modifyProperty (&ind_class_properties, ns,
                          cn, pn, CHANGE_DATA, 0, NULL, d);
}

IndErrorT
ind_unreg_property (const char *ns, const char *ci, const char *pn)
{

  return _deleteProperty (&ind_class_properties, ns, ci, pn);

}

IndErrorT
ind_set_properties_d (const char *ns,
                      const char *ci, const char *pn[], const CMPIData * d[], size_t l)
{

  int i = 0;
  IndErrorT rc = IND_OK;

  if ((ns == NULL) || (ci == NULL) || (pn == NULL) || (d == NULL))
    return IND_INVALID_ARGS;

  if (strlen (ns) == 0)
    return IND_INVALID_ARGS;

  if (strlen (ci) == 0)
    return IND_INVALID_ARGS;

  for (i = 0; i < l; i++)
    {
      rc = ind_set_property_d (ns, ci, pn[i], d[i]);
      if (rc != IND_OK)
        break;
    }
  return rc;

}

IndErrorT
ind_set_properties_da (const char *ns,
                       const char *ci, const char *pn[], const CMPIData * d, size_t l)
{
  IndErrorT rc = IND_OK;
  int i;

  if ((ns == NULL) || (d == NULL) || (pn == NULL) || (ci == NULL))
    return IND_INVALID_ARGS;

  if (strlen (ns) == 0)
    return IND_INVALID_ARGS;

  if (strlen (ci) == 0)
    return IND_INVALID_ARGS;

  for (i = 0; i < l; i++)
    {
      rc = ind_set_property_d (ns, ci, pn[i], &d[i]);
      if (rc != IND_OK)
        break;
    }


  return rc;
}

IndErrorT
ind_set_properties_f (const char *ns,
                      const char *ci,
                      const char *pn[],
                      IndErrorT (**getAlertProperty) (CMPIData *), size_t l)
{
  int i = 0;
  int rc = IND_OK;

  if ((ns == NULL) || (ci == NULL) || (pn == NULL)
      || (getAlertProperty == NULL))
    return IND_INVALID_ARGS;

  if (strlen (ns) == 0)
    return IND_INVALID_ARGS;

  for (i = 0; i < l; i++)
    {
      rc = ind_set_property_f (ns, ci, pn[i], getAlertProperty[i]);
      if (rc != IND_OK)
        break;
    }
  return rc;
}

IndErrorT
ind_unreg_properties (const char *ns, const char *ci, const char **pn, size_t l)
{

  int i = 0;
  int rc = IND_OK;

  if ((ci == NULL) || (pn == NULL) || (ns == NULL))
    return IND_INVALID_ARGS;

  if (strlen (ns) == 0)
    return IND_INVALID_ARGS;

  for (i = 0; i < l; i++)
    {
      rc = ind_unreg_property (ns, ci, pn[i]);
      if (rc != IND_OK)
        break;
    }
  return rc;

}

/**
 * +-----------------------------------
 * + 
 * +    Indication and monitor class shared function.
 * +
 * +-----------------------------------
 */

IndErrorT
ind_set_classes (const char *ns, const char *cn, const char *ci)
{
  size_t ns_l, ci_l, cn_l;
  ind_class_property *p;
  IndErrorT rc = IND_NOT_FOUND;

  IND_HLP_DEBUG ("ind_set_classes called.");

  if ((ns == NULL) || (cn == NULL) || (ci == NULL))
    {
      IND_HLP_DEBUG ("Arguments are null!");
      return IND_INVALID_ARGS;
    }

  ns_l = strlen (ns);
  ci_l = strlen (ci);
  cn_l = strlen (cn);


  if ((ns_l == 0) || (ci_l == 0) || (cn_l == 0))
    {
      IND_HLP_DEBUG ("Invalid arguments!");
      return IND_INVALID_ARGS;
    }

  if (pthread_mutex_lock (&ind_class_properties.mutex) != 0)
    {
      IND_HLP_DEBUG ("Lock mutex error.");
      return IND_MUTEX;
    }
  for (p = ind_class_properties.data; p != NULL; p = p->p)
    {
      if ((strncmp (p->namespace, ns, ns_l) == 0) &&
          (strncmp (p->cn, ci, ci_l) == 0))
        {
          /* Found the entry */
          p->ci_cn = strdup (cn);
          /*
             fprintf(stderr,"%s:%s set to %s\n",
             p->namespace, p->cn, p->ci_cn);
           */
          if (p->ci_cn == NULL)
            {
              /* Yuck */
              pthread_mutex_unlock (&ind_class_properties.mutex);
              return IND_NO_MEMORY;
            }
          /*
             fprintf(stderr,"p->ns: %s, p->cn: %s, p->ci_cn: %s\n",
             p->namespace, p->cn, p->ci_cn);
           */
          rc = IND_OK;
        }
    }
  /* Exit critical section for monitor_properties */
  if (pthread_mutex_unlock (&ind_class_properties.mutex) != 0)
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      return IND_MUTEX;
    }
  IND_HLP_DEBUG ("ind_set_classes exited.");
  return rc;

}


/**
 * +-----------------------------------
 * + 
 * +    Provider specific operation
 * +
 * +-----------------------------------
 */

IndErrorT
ind_set_select (const char *ns, const char *ci, const CMPISelectExp * filter)
{
  size_t ns_l, ci_l;
  IndErrorT rc = IND_NOT_FOUND;
  CMPIStatus status = { CMPI_RC_OK, NULL };
  CMPIString *filter_str;

  IND_HLP_DEBUG ("ind_set_select called.");

  if ((filter == NULL) || (ns == NULL) || (ci == NULL))
    {
      IND_HLP_DEBUG ("Arguments are null!");
      return IND_INVALID_ARGS;
    }

  ns_l = strlen (ns);
  ci_l = strlen (ci);

  if ((ns_l == 0) || (ci_l == 0))
    {
      IND_HLP_DEBUG ("Invalid arguments!");
      return IND_INVALID_ARGS;
    }

  /* Construct a property name from filter expression */

  filter_str = CMGetSelExpString (filter, &status);
  if (status.rc != CMPI_RC_OK)
    {
      IND_HLP_DEBUG ("CMGetSelExpString operation failed!");
      return IND_CMPI_OPERATION;
    }


  rc = _addProperty (&ind_filters, ns, ci, CMGetCharPtr (filter_str), NULL,     /* No function */
                     0,         /* No timer */
                     IND_BEHAVIOR_NONE,        /* No logic */
                     filter);

  IND_HLP_DEBUG ("ind_set_select exited.");
  return rc;
}



IndErrorT
ind_unreg_select (const char *ns, const char *ci, const CMPISelectExp * filter)
{
  size_t ns_l, ci_l;
  IndErrorT rc = IND_NOT_FOUND;
  CMPIStatus status = { CMPI_RC_OK, NULL };
  CMPIString *filter_str;

  IND_HLP_DEBUG ("ind_unreg_select called.");

  if ((filter == NULL) || (ns == NULL) || (ci == NULL))
    {
      IND_HLP_DEBUG ("Arguments are null!");
      return IND_INVALID_ARGS;
    }

  ns_l = strlen (ns);
  ci_l = strlen (ci);

  if ((ns_l == 0) || (ci_l == 0))
    {
      IND_HLP_DEBUG ("Invalid arguments");
      return IND_INVALID_ARGS;
    }

  filter_str = CMGetSelExpString (filter, &status);
  if (status.rc != CMPI_RC_OK)
    {
      return IND_CMPI_OPERATION;
    }

  return _deleteProperty (&ind_filters, ns, ci, CMGetCharPtr (filter_str));

  IND_HLP_DEBUG ("ind_unreg_select exited.");
  return rc;

}

/**
 * --------------------------------------
 * CMPI specific functions
 *
 * --------------------------------------
 */
static IndErrorT
_generateIndication (ind_class_property * monitor_property,
                     const CMPIBroker * broker, const CMPIContext * ctx)
{
  ind_class_property *p, *property = NULL;
  size_t cn_l, ns_l;
  CMPIData data;
  CMPIInstance *instance = NULL;
  CMPIObjectPath *op = NULL;
  CMPIStatus status;
  CMPIBoolean b;
  IndErrorT rc = IND_OK;
#if defined(DEBUG)
  CMPIString *temp = NULL;
#endif
  /* Send an indication */

  IND_HLP_DEBUG ("_generateIndication called.");

  /* First order of business is to find the indication class
     which is tied in with the monitored property */

  cn_l = strlen (monitor_property->cn);

  if (pthread_mutex_lock (&ind_class_properties.mutex) != 0)
    {
      IND_HLP_DEBUG ("Lock mutex error.");
      return IND_MUTEX;
    }

  for (p = ind_class_properties.data; p != NULL; p = p->p)
    {
      if (p->ci_cn != NULL)
        {
          if (strncmp (monitor_property->cn, p->ci_cn,  /* This value is set only when
                                                           'ind_set_classes' has been called. */
                       cn_l) == 0)
            {
              property = p;
              break;
            }
        }
    }
  if (pthread_mutex_unlock (&ind_class_properties.mutex) != 0)
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      return IND_MUTEX;
    }

  if (property == NULL)
    return IND_NOT_TIED_TOGETHER;


  ns_l = strlen (property->namespace);

  /* Make the the object path */
  op = CMNewObjectPath (broker, property->namespace,    /* "root/cimv2" */
                        property->cn,   /* "CIM_Alert" */
                        &status);

  if (status.rc != CMPI_RC_OK)
    {
      IND_HLP_DEBUG ("Creating new object path failed.");
      rc = IND_CMPI_OPERATION;
      goto exit;
    }
  instance = CMNewInstance (broker, op, &status);

  if (status.rc != CMPI_RC_OK)
    {
      IND_HLP_DEBUG ("Creating new instance failed.");
      rc = IND_CMPI_OPERATION;
      goto exit;
    }

  if (pthread_mutex_lock (&ind_class_properties.mutex) != 0)
    {
      IND_HLP_DEBUG ("Lock mutex error.");
      return IND_MUTEX;
    }

  /* Go thru the list of properties for this class and put them
     in the instance */
  for (p = ind_class_properties.data; p != NULL; p = p->p)
    {
      /* Find the rest of class and namespace in which this happend */
      if ((strncmp (property->cn, p->cn, cn_l) == 0) &&
          (strncmp (property->namespace, p->namespace, ns_l) == 0))
        {
          if (p->check == NULL)
            {
              CMSetProperty (instance, p->pn,   /* "SystemName" */
                             &p->data.value,    /* "darnok.com" */
                             p->data.type);     /* CMPI_chars */
            }
          else
            {
              IND_HLP_DEBUG ("Calling the function");
              if (p->check (&data) == IND_OK)
                {
                  CMSetProperty (instance, p->pn,       /* "SystemName" */
                                 &data.value,   /* "darnok.com" */
                                 data.type);    /* CMPI_chars */
                }
              else
                {
                  IND_HLP_DEBUG
                    ("Called functions returns not OK return code!!!");
                  /* If encounter this situation, should we abondon
                     the creation of the indication?
                     If so this will do the job:
                     pthread_mutex_unlock(&ind_class_properties.mutex);
                     goto exit;

                     For right now, just print a message:
                   */

                  fprintf (stderr,
                           "%s(%d):Called function to construct the property: %s for %s:%s returns a wrong return code!\n",
                           __FILE__, __LINE__, p->pn, p->namespace, p->cn);

                }
              _releaseData (&data);
            }
        }

    }
  if (pthread_mutex_unlock (&ind_class_properties.mutex) != 0)
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      return IND_MUTEX;
    }
  IND_HLP_DEBUG ("Looking for filters activated");
  /* Check the query filters and see if they are any that return true. */
  if (pthread_mutex_lock (&ind_filters.mutex) != 0)
    {
      IND_HLP_DEBUG ("Lock mutex error.");
      rc = IND_MUTEX;
      goto exit;
    }

  for (p = ind_filters.data; p != NULL; p = p->p)
    {
#ifdef DEBUG
      fprintf (stderr, "Looking at %s ? %s, %s ? %s\n",
               property->namespace, p->namespace, property->cn, p->cn);

#endif

      if ((strncmp (property->cn, p->cn, cn_l) == 0) &&
          (strncmp (property->namespace, p->namespace, ns_l) == 0))
        {
          /* Found a class and namespace that match this one. Try
             the filter on it */

#ifdef DEBUG
          temp = CDToString (broker, instance, &status);
          fprintf (stderr, "Trying filter: %p from %s:%s on instance %s\n",
                   p->filter, p->namespace, p->cn,
                   (temp->hdl != NULL) ? CMGetCharPtr (temp) : "unknown.");
          if (temp)
            {
              CMRelease (temp);
              temp = NULL;
            }
#endif
          if (p->filter != NULL)
            {
              IND_HLP_DEBUG ("Evaluating instance using found filter.");
              status.rc = CMPI_RC_OK;

              /* Note:
                 If you are using Pegasus 2.3.2, this routine will
                 always return false (why? B/c it was not implemented)
               */
              b = CMEvaluateSelExp (p->filter, instance, &status);

              if (status.rc != CMPI_RC_OK)
                {
                  IND_HLP_DEBUG ("Problems evaluating the instance!");
                  pthread_mutex_unlock (&ind_class_properties.mutex);
                  goto exit;
                }
#ifdef HAVE_PEGASUS_2_3_2
              /* Pegasus 2.3.2 does not evaluate the query expression
                 and always returns false */
              fprintf (stderr,
                       "Pegasus 2.3.2 CMEvaluateSelExp evaluation always returns false. Hence not using query evaluation");
              if (1)
#else
              if (b == 1)       /* True */
#endif
                {
                  status = CBDeliverIndication (broker,
                                                ctx,
                                                property->namespace,
                                                instance);

                  if (status.rc == CMPI_RC_OK)
                    {
                      IND_HLP_DEBUG ("Indication delievered.");
                    }
                 else
                    {
		      char mmsg[50];
                      snprintf(mmsg,50,"Indication NOT delievered, rc=%d",status.rc);
		      IND_HLP_DEBUG (mmsg);
                    }
                  break;        /* Break out of the loop. That way we can do 
                                   our job - delievering the indication based
                                   on the filter */


                }               /* B==1 */
            }                   /* filter == NULL */
          else
            {
              IND_HLP_DEBUG ("No filter for this instance found.");
            }
          /* If there are no filter for this entry, that does not 
             mean that there are no other ones. Loop through all of
             the filters */
        }
    }


  if (pthread_mutex_unlock (&ind_filters.mutex) != 0)
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      rc = IND_MUTEX;
      /* goto exit; */
    }

exit:
  if (instance != NULL)
    CMRelease (instance);
  if (op != NULL)
    CMRelease (op);

  _releaseData (&data);
  IND_HLP_DEBUG ("_generateIndication exited.");

  return rc;
}

static void *
_worker (void *cmpi_data)
{
  size_t n = 0, i, cn_l, ns_l;
  int rc = IND_OK;
  ind_class_property *curr = NULL, *p, *q;
  void **args = (void **) cmpi_data;
  CMPIContext *ctx = (CMPIContext *) args[1];
  CMPIBroker *broker = (CMPIBroker *) args[0];
  CMPIData data;
  time_t abs_time = 0;

  IND_HLP_DEBUG ("_worker called.");
  free (args);

  if ((broker == NULL) || (ctx == NULL))
    {
      IND_HLP_DEBUG ("Null broker or context passed to worker thread.");
      return (void *) IND_INVALID_ARGS;
    }

  CBAttachThread (broker, ctx);

  while (1)
    {
      /* We do not lock the flag. B/c we are the readers and if it
         happends that the data is outdated we don't worry much */

      if ((flag.flag & WORKER_END_LIFE) == WORKER_END_LIFE)
        {
          IND_HLP_DEBUG ("Quiting _worker");
          break;
        }

      if ((flag.flag & WORKER_STOP) == WORKER_STOP)
        /*
           Stop and go to sleep for a year (can be interruptable).
           Unlock all of the properties.
         */
        {
          time (&abs_time);
          sleep.timer.tv_sec = abs_time + 365 * 60 * 60 * 24;


          /* Unlock all of the properties, lock critical section */
          if (pthread_mutex_lock (&monitor_properties.mutex) != 0)
            {
              IND_HLP_DEBUG ("Lock mutex error.");
              rc = IND_MUTEX;
              break;
            }
          else
            {                   /* Success locking the critical section */
              for (p = monitor_properties.data; p != NULL;
                   p->flag = (p->flag & (~LOCK)) | UNLOCK, p = p->p)
                ;

              if (pthread_mutex_unlock (&monitor_properties.mutex) != 0)
                {
                  IND_HLP_DEBUG ("Unlock mutex error.");
                  rc = IND_MUTEX;
                  break;
                }
            }
        }

      if ((flag.flag & WORKER_START) == WORKER_START)
        {
          /*
           * Start with a clean slate.
           */
          if (pthread_mutex_lock (&monitor_properties.mutex) == 0)
            {
              curr = monitor_properties.data;
              sleep.timer.tv_sec = 0;
              sleep.timer.tv_nsec = 0;
              abs_time = 0;
              flag.flag |= WORKER_RUNNING;
              rc = IND_OK;
            }
          else
            {
              IND_HLP_DEBUG ("Lock mutex error.");
              rc = IND_MUTEX;
              break;
            }
          if (pthread_mutex_unlock (&monitor_properties.mutex) != 0)
            {
              IND_HLP_DEBUG ("Unlock mutex error.");
              rc = IND_MUTEX;
              break;
            }

        }                       /* Worker start */


      if ((flag.flag & WORKER_RUNNING) == WORKER_RUNNING)
        /*
           First part of a worker - find how long we have to sleep
           and "lock" (set the LOCK flag on, not locking specific
           properties in the list) job(s) that have to same sleep
           time. */
        {
          /* Critical section */
          if (pthread_mutex_lock (&monitor_properties.mutex) != 0)
            {
              IND_HLP_DEBUG ("Lock mutex error.");
              rc = IND_MUTEX;
              break;
            }
          else                  /* Locked */
            {
              if (curr == NULL)
                {
                  IND_HLP_DEBUG ("Current property disappeared!");
                  /* Recover, get the first item of the list. */
                  curr = monitor_properties.data;
                }
              sleep.timer.tv_sec =
                curr->timer - sleep.timer.tv_sec - abs_time;

              for (p = curr, n = 0; p != NULL; n++, p = p->p)
                {
                  if (p->timer == sleep.timer.tv_sec)
                    p->flag = (p->flag & (~UNLOCK)) | LOCK;
                  else
                    break;
                }
              /* Fix the time to absolute time */
              time (&abs_time);
              sleep.timer.tv_sec += abs_time;

            }
          if (pthread_mutex_unlock (&monitor_properties.mutex) != 0)
            {
              IND_HLP_DEBUG ("Unlock mutex error.");
              rc = IND_MUTEX;
              break;
            }
        }                       /* Worker running */


      /* Sleep here. */
      pthread_cond_timedwait (&sleep.cond, &sleep.mutex, &sleep.timer);
      /* Awake */


      if ((flag.flag & WORKER_RUNNING) == WORKER_RUNNING)
        /*
           For all of the jobs that are locked, call the jobs and
           check the return value. If different from saved, send 
           an indication.
         */
        {
          if (pthread_mutex_lock (&monitor_properties.mutex) != 0)
            {
              IND_HLP_DEBUG ("Lock mutex error.");
              rc = IND_MUTEX;
              break;
            }
          else                  /* Lock succeeded */
            {
              for (p = curr; p != NULL && rc == IND_OK; p = p->p)
                {
                  /* Only call functions which are locked */
                  if (((p->flag & LOCK) == LOCK) && (p->check != NULL))
                    {
                      /* Call the routine */
                      data.type = CMPI_null;
                      data.state = CMPI_nullValue;

                      IND_HLP_DEBUG ("Calling the monitor function");

                      if (p->check (&data) != IND_OK)
                        {

                          fprintf (stderr,
                                   "%s(%d):Polling function [%s:%s:%s] returns invalid return code!\n",
                                   __FILE__, __LINE__, p->namespace, p->cn,
                                   p->pn);
                          _releaseData (&data);
                          break;
                        }

                      /* There are two mechanism to do the logic comparison: 
                         IND_BEHAVIOR_LEVEL_EDGE
                         IND_BEHAVIOR_AGAINST_ZERO
                       */
                      if (p->logic == IND_BEHAVIOR_AGAINST_ZERO)
                        {
                          IND_HLP_DEBUG
                            ("Logic is IND_BEHAVIOR_AGAINST_ZERO");
                          if ((p->flag & FIRST_TIME) == FIRST_TIME)
                            {
                              /* No need for this anymore. Remove it */
                              p->flag = (p->flag & (~FIRST_TIME));
                              /* Seed the p->data with the correct types, the _releaseDataValue
                                 resets the values to zero. */
                              if ((rc =
                                   _cloneData (&p->data, &data)) == IND_OK)
                                {
                                  IND_HLP_DEBUG
                                    ("Reseting the property->data to zero. ");
                                  _releaseDataValue (&p->data);
                                }
                              else
                                {
                                  IND_HLP_DEBUG
                                    (" We could not clone the data! One of the arguments is NULL");
                                }
                            }
                        }
                      else
                        {
                          IND_HLP_DEBUG ("Logic is IND_BEHAVIOR_LEVEL_EDGE");
                        }
                      if ((_compareCIMValue (data, p->data) != 0))
                        {       /* Yeey, data changed! */
                          /* Copy the data */
                          IND_HLP_DEBUG
                            ("Yeey, data changed! Generate indication.\n");

                          if (p->logic == IND_BEHAVIOR_LEVEL_EDGE)
                            {
                              /* Only keep the past property values if LEVEL_EDGE is used */
                              _releaseData (&p->data);
                              _cloneData (&p->data, &data);
                            }
                          rc = _generateIndication (p, broker, ctx);

                          if (rc != IND_OK)
                            {
                              IND_HLP_DEBUG
                                ("Could not generate indication!");
                              fprintf (stderr,
                                       "%s(%d):Could not generate indication for %s:%s.%s\n",
                                       __FILE__, __LINE__, p->namespace,
                                       p->cn, p->pn);

                              if (rc == IND_NOT_TIED_TOGETHER)
                                fprintf (stderr,
                                         "%s(%d):Developer did not call 'ind_set_classes' after setting the properties.\n",
                                         __FILE__, __LINE__);
                            }
                          ns_l = strlen (p->namespace);
                          cn_l = strlen (p->cn);

                          /* Set the flag to UNLOCK for all of  properties of
                             this class. So that we don't end up sending
                             many indications (b/c each property might 
                             spawn of new ones ) */
                          for (q = monitor_properties.data; q != NULL;
                               q = q->p)
                            {
                              if ((strncmp (p->namespace, q->namespace, ns_l)
                                   == 0)
                                  && (strncmp (p->cn, q->cn, cn_l) == 0))
                                {
                                  q->flag = (q->flag & (~LOCK)) | UNLOCK;
                                }
                            }
                        }
                      /* Release any data we might have allocated */
                      _releaseData (&data);
                    }
                  else
                    {
                      if (p->check == NULL)
                        IND_HLP_DEBUG ("Polling function is null.");
                    }
                }

              /**
	       * Advance the index by n.
	       */
              for (i = 0; i < n; i++)
                {

                  if (curr->p == NULL)
                    curr = monitor_properties.data;
                  else
                    curr = curr->p;
                }
              /* Critical section exit. */
              if (pthread_mutex_unlock (&monitor_properties.mutex) != 0)
                {
                  IND_HLP_DEBUG ("Unlock mutex error.");
                  rc = IND_MUTEX;
                }
            }
          n = 0;
        }                       /* if (worker== running .. */


    }                           /* while */


  /* Unlock all of the properties  */
  if (pthread_mutex_lock (&monitor_properties.mutex) == 0)
    {
      for (p = monitor_properties.data; p != NULL;
           p->flag = (p->flag & (~LOCK)) | UNLOCK, p = p->p)
        ;
      if (pthread_mutex_unlock (&monitor_properties.mutex) != 0)
        {
          IND_HLP_DEBUG ("Unlock mutex error.");
          rc = IND_MUTEX;
        }
    }
  else
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      rc = IND_MUTEX;
    }

  CBDetachThread (broker, ctx);


  pthread_detach (thread_id);
  IND_HLP_DEBUG ("_worker exited.");

  pthread_exit ((void *)(intptr_t) rc);
}

/**
 * ------------------------------------------------
 * Data structure functions
 *
 * ------------------------------------------------
 */

static IndErrorT
_clean (data_mutex * d)
{
  ind_class_property *p, *prev;

  IND_HLP_DEBUG ("_clean called.");

  /* Enter critical section for monitor_properties */

  if (pthread_mutex_lock (&d->mutex) != 0)
    {
      IND_HLP_DEBUG ("Lock mutex error.");
      return IND_MUTEX;
    }
  for (prev = p = d->data; p != NULL;)
    {
      /*
         fprintf(stderr,"Cleaning: %s, %s [%p, %p, %p]\n", p->cn,
         p->pn, prev, p, p->p);
       */
      /* Note: This 'ci_cn' is only used in ind_class_properties, not
         in monitor_properties */
      if (p->ci_cn != NULL)
        free ((void *) p->ci_cn);

      if (p->filter != NULL)
        CMRelease (p->filter);

      free ((void *) p->cn);
      free ((void *) p->pn);
      free ((void *) p->namespace);

      _releaseData (&p->data);

      prev = p;
      p = p->p;
      free (prev);

    }
  d->data = NULL;

  /* Exit critical section for monitor_properties */
  if (pthread_mutex_unlock (&d->mutex) != 0)
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      return IND_MUTEX;
    }

  return IND_OK;
}


/**
 * Add a property to be monitored for a specific class.
 */
static IndErrorT
_addProperty (data_mutex * d,
              const char *namespace,
              const char *cn,
              const char *pn,
              IndErrorT (*check) (CMPIData *),
              IndTimeT t, IndCheckBehaviour logic, const CMPISelectExp * filter)
{
  size_t cn_l, pn_l, ns_l;
  IndErrorT rc = IND_OK;
  ind_class_property *property, *curr, *prev;
  CMPIData data;
  CMPIStatus status = { CMPI_RC_OK, NULL };

  IND_HLP_DEBUG ("_addProperty called.");
  cn_l = strlen (cn);
  pn_l = strlen (pn);
  ns_l = strlen (namespace);

  if ((cn_l == 0) || (pn_l == 0) || (ns_l == 0))
    return IND_INVALID_ARGS;
#ifdef DEBUG
  fprintf (stderr, "ns: %s, cn: %s, pn: %s\n", namespace, cn, pn);
#endif

  /* Critical section lock */
  if (pthread_mutex_lock (&d->mutex) != 0)
    {
      IND_HLP_DEBUG ("Lock mutex error.");
      return IND_MUTEX;
    }

  if (d->data != NULL)
    {
      /* Search for the class and property. If found, return error */
      for (curr = d->data; curr != NULL; curr = curr->p)
        {

          if ((strncmp (cn, curr->cn, cn_l) == 0) &&
              (strncmp (pn, curr->pn, pn_l) == 0) &&
              (strncmp (namespace, curr->namespace, ns_l) == 0))
            {
              IND_HLP_DEBUG ("Duplicate property found.");
              /* Unlock critical section before exiting. */
              pthread_mutex_unlock (&d->mutex);
              return IND_DUPLICATE;
            }
        }
    }
  if (pthread_mutex_unlock (&d->mutex) != 0)
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      return IND_MUTEX;
    }

  /* Allocate new space, copy to the new location, and free the old */
  property = malloc (sizeof (ind_class_property));
  if (property == NULL)
    {
      IND_HLP_DEBUG ("Not enough memory.");
      pthread_mutex_unlock (&d->mutex);
      return IND_NO_MEMORY;
    }

  /* Set to 'a' so we can debug it easier */
  memset (property, 0x61, sizeof (ind_class_property));

  /* Set each individual entry */
  property->flag = UNLOCK;
  if (logic == IND_BEHAVIOR_AGAINST_ZERO)
    property->flag |= FIRST_TIME;

  property->logic = logic;
  property->p = NULL;
  property->data.type = CMPI_null;
  property->data.state = CMPI_nullValue;
  if (t <= 0)
    t = 1;
  property->timer = t;
  property->check = NULL;
  property->ci_cn = NULL;
  property->filter = NULL;

  property->pn = (char *) strdup (pn);
  if (property->pn == NULL)
    goto exit_error;
  property->cn = (char *) strdup (cn);
  if (property->cn == NULL)
    goto exit_error;


  property->namespace = (char *) strdup (namespace);
  if (property->namespace == NULL)
    goto exit_error;

  if (filter != NULL)
    {
      IND_HLP_DEBUG ("Cloning a filter");
      property->filter = CMClone (filter, &status);
      if (status.rc == CMPI_RC_ERR_NOT_SUPPORTED)
        {
          IND_HLP_DEBUG ("Cloning of filters is unsupported.");
          property->filter = filter;
        }
      else if (status.rc != CMPI_RC_OK)
        {
          IND_HLP_DEBUG ("Error cloning a filter. Not using it");
          goto exit_error;
        }
    }

  property->check = check;
  /* Call the function to get the value */

  if (check != NULL)
    {

      data.type = CMPI_null;
      data.state = CMPI_nullValue;
      /* We do not need to call the check function if our
         logic is using IND_BEHAVIOR_AGAINST_ZERO to get the
         previous value. */
      if (logic == IND_BEHAVIOR_LEVEL_EDGE)
        {
          if ((rc = check (&data)) != IND_OK)
            {
              IND_HLP_DEBUG ("Polling routine return code is wrong!");
              goto exit_error;
            }
          /* This is where the data is copied in-to memory */
          /* Not needed when the logic is IND_BEHAVIOR_AGAINST_ZERO here, as
             the _worker will call the check function and then populate the
             property->data */
          if ((rc = _cloneData (&property->data, &data)) != IND_OK)
            {
              IND_HLP_DEBUG
                ("Could not copy the data passed in from user routine");
              goto exit_error;
            }
        }
      /* Clean the temporary data */
      _releaseData (&data);
    }

  /* Critical section lock */
  if ((rc = pthread_mutex_lock (&d->mutex)) != 0)
    {
      IND_HLP_DEBUG ("Lock mutex error.");
      goto exit_error;
    }

  /* Sort */
  if (d->data != NULL)
    {
      /* Add the 'property' in order of "timer". */
      for (prev = curr = d->data; curr != NULL; prev = curr, curr = curr->p)
        {
          /* Property has to be added. After a smaller number
             and before a bigger number (if it exist) */
          if (_compareProperty (property, prev) >= 0)
            {
              /* Are more items to check against? If not
                 we are the last entry to be appended */
              if (curr->p == NULL)
                {
                  curr->p = property;
                  property->p = NULL;
                  break;
                }
            }
          else
            {
              /* Property has to be appended  to the prev, and before curr */
              property->p = curr;
              /*
                 if (prev != curr)
                 prev->p = property;
                 else */
              d->data = property;

              break;
            }
        }
    }
  else
    {
      /* First item */
      d->data = property;
    }

  /* Critical section exit. */
  if (pthread_mutex_unlock (&d->mutex) != 0)
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      return IND_MUTEX;
    }

  IND_HLP_DEBUG ("_addProperty exited.");
  return IND_OK;

exit_error:
  IND_HLP_DEBUG ("Cleaning up temporary values.");
  _releaseData (&data);
  if (property)
    {
      /* Free the property that was not added */
      if (property->namespace)
        free ((void *) property->namespace);
      if (property->cn)
        free ((void *) property->cn);
      if (property->pn)
        free ((void *) property->pn);

      if (property->filter != NULL)
        CMRelease (property->filter);

      /* free((void *)property->ci); */
      _releaseData (&property->data);
  /*_releaseData(&property->old_data);*/
      free (property);
    }

  return rc;

}

static IndErrorT
_deleteProperty (data_mutex * d,
                 const char *namespace, const char *cn, const char *pn)
{
  size_t cn_l, pn_l, ns_l;
  ind_class_property *prev, *curr;
  IndErrorT rc = IND_NOT_FOUND;

  IND_HLP_DEBUG ("_deleteProperty called.");

  if (d == NULL)
    return IND_INVALID_ARGS;

  cn_l = strlen (cn);
  pn_l = strlen (pn);
  ns_l = strlen (namespace);

  if ((cn_l == 0) || (pn_l == 0) || (ns_l == 0))
    return IND_INVALID_ARGS;

#ifdef DEBUG
  fprintf (stderr, "ns: %s, cn: %s, pn: %s\n", namespace, cn, pn);
#endif
  /* Critical section entrance */
  if (pthread_mutex_lock (&d->mutex) != 0)
    {
      IND_HLP_DEBUG ("Lock mutex error.");
      return IND_MUTEX;
    }

  for (curr = prev = d->data; curr != NULL; prev = curr, curr = curr->p)
    {
      if ((strncmp (cn, curr->cn, cn_l) == 0) &&
          (strncmp (pn, curr->pn, pn_l) == 0))
        {
          /* If the namespace was passed in as argument, one more check */
          if (ns_l != 0)
            {
              /* If the namespace does not match, then bail out */
              if (strncmp (namespace, curr->namespace, ns_l) != 0)
                break;
            }
          if ((curr->flag & LOCK) == LOCK)
            {
              IND_HLP_DEBUG ("Property locked by worker.");
              /* Unlock critical section */
              pthread_mutex_unlock (&d->mutex);
              return IND_LOCKED;
            }
          free ((void *) curr->cn);
          free ((void *) curr->pn);
          free ((void *) curr->namespace);
          free ((void *) curr->ci_cn);
          _releaseData (&curr->data);

          if (curr->filter != NULL)
            {
              IND_HLP_DEBUG ("Releasing query filter ");
              CMRelease (curr->filter);
            }
          /* _releaseData(&curr->old_data); */

          if (prev != curr)
            prev->p = curr->p;
          else
            d->data = curr->p;

          free (curr);

          rc = IND_OK;
          break;
        }
    }
  /* Critical section exit. */
  if (pthread_mutex_unlock (&d->mutex) != 0)
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      return IND_MUTEX;
    }

  IND_HLP_DEBUG ("_deleteProperty exited. ");
  return rc;
}


/**
 * Modify the properties.
 */
static IndErrorT
_modifyProperty (data_mutex * d,
                 const char *namespace,
                 const char *cn, const char *pn,
                 const int changeAttr,
                 IndTimeT t, IndErrorT (*check) (CMPIData *), const CMPIData * data)
{
  size_t cn_l, pn_l, ns_l;
  ind_class_property *p;
  IndErrorT rc = IND_NOT_FOUND;

  IND_HLP_DEBUG ("_modifyProperty called.");

  if (d == NULL)
    return IND_INVALID_ARGS;

  cn_l = strlen (cn);
  pn_l = strlen (pn);
  ns_l = strlen (namespace);

  if ((cn_l == 0) || (ns_l == 0))
    {
      IND_HLP_DEBUG ("ClassName or Namespace length is zero.");
      return IND_INVALID_ARGS;
    }

#ifdef DEBUG
  fprintf (stderr, "ns: %s, cn: %s, pn: %s\n", namespace, cn, pn);
#endif
  /* Enter critical section */

  if (pthread_mutex_lock (&d->mutex) != 0)
    {
      IND_HLP_DEBUG ("Lock mutex error.");
      return IND_MUTEX;
    }
  /* Search for the class and property. */
  for (p = d->data; p != NULL; p = p->p)
    {
      /* Find based on the class name and property. Also find
         only based on the class name. */
      if (((strncmp (cn, p->cn, cn_l) == 0) &&
           (strncmp (pn, p->pn, pn_l) == 0) &&
           (strncmp (namespace, p->namespace, ns_l) == 0))
          || ((pn_l == 0) && (strncmp (cn, p->cn, cn_l))))
        {
          if ((p->flag & LOCK) == LOCK)
            {
              /* Unlock critical section before exiting */
              pthread_mutex_unlock (&d->mutex);
              return IND_LOCKED;
            }
          if (changeAttr == CHANGE_TIME)
            p->timer = t;
          if (changeAttr == CHANGE_FUNC)
            p->check = check;
          if (changeAttr == CHANGE_DATA)
            {
              rc = _releaseData (&p->data);
              if (rc != CMPI_RC_OK)
                break;

              _cloneData (&p->data, data);
              if (rc != CMPI_RC_OK)
                break;
            }

          rc = IND_OK;

          /* Continue looping in case there are no passed in property
             and this is for all properties of that class */
          if (pn_l != 0)
            break;
        }
    }


  /* Critical section exit. */
  if (pthread_mutex_unlock (&d->mutex) != 0)
    {
      IND_HLP_DEBUG ("Unlock mutex error.");
      return IND_MUTEX;
    }
  IND_HLP_DEBUG ("_modifyProperty exited.");
  return rc;

}



/* ---------------------------------------
 * Helper functions (private)
 * ---------------------------------------
 */
/**
 * Routine borrowed from Linux_NFSv3SystemConfigurationUtil.c
 */
static int
_compareCIMValue (CMPIData value1, CMPIData value2)
{

  CMPIValue v1 = value1.value;
  CMPIValue v2 = value2.value;
  CMPIStatus status1, status2;
  CMPIString *v1_str, *v2_str;
  int v1_cnt, v2_cnt, i, rc;
  CMPIData v1_data, v2_data;
#if defined(DEBUG)
  fprintf (stderr, "type:%d == %d, state: %d %d\n",
           value1.type, value2.type, value1.state, value2.state);
#endif

  /* Check that the type of the two CIM values is the same */
  if (value1.type != value2.type)
    return 0;

  /* Check that the value of the two CIM values is the same */
  switch (value1.type)
    {
    case CMPI_null:
      return 0;
    case CMPI_string:
      if ((v1.string) || (v2.string))
        {
#if defined(DEBUG)
          fprintf (stderr, "%s =? %s\n", CMGetCharPtr (v1.string),
                   CMGetCharPtr (v2.string));
#endif
          if ((v1.string->hdl) && (v2.string->hdl))
            return strcmp (CMGetCharPtr (v1.string),
                           CMGetCharPtr (v2.string));
        }
      return 0;
    case CMPI_dateTime:
      return CMGetBinaryFormat (v1.dateTime,
                                NULL) != CMGetBinaryFormat (v2.dateTime,
                                                            NULL);

    case CMPI_ref:
#if defined(CMPI_VER_86)

      v1_str = (CMPIString *) v1.ref->ft->toString (v1.ref, &status1);

      v2_str = (CMPIString *) v2.ref->ft->toString (v2.ref, &status2);
      if ((status1.rc != CMPI_RC_OK) || (status2.rc != CMPI_RC_OK))
        {
          IND_HLP_DEBUG ("Could not convert objectPath to string!");
          return -1;
        }

      return strcmp (CMGetCharPtr (v1_str), CMGetCharPtr (v2_str));
#else
      return -1;
#endif
    case CMPI_args:
      v1_cnt = CMGetArgCount (v1.args, &status1);
      v2_cnt = CMGetArgCount (v2.args, &status2);
      if ((status1.rc != CMPI_RC_OK) || (status2.rc != CMPI_RC_OK))
        {
          IND_HLP_DEBUG ("Could not convert CMPIArgs to string!");
          return -1;
        }
      if (v1_cnt == v2_cnt)
        {                       /* More comparisons */
          /* This is a poor man comparison. */
          for (i = 0; i < v1_cnt; i++)
            {
              v1_data = CMGetArgAt (v1.args, i, &v1_str, &status1);
              v2_data = CMGetArgAt (v2.args, i, &v2_str, &status2);
              if ((status1.rc != CMPI_RC_OK) || (status2.rc != CMPI_RC_OK))
                {
                  IND_HLP_DEBUG
                    ("Could not convert CMPIArgs value using CMGetArgAt to string!");
                  return -1;
                }
              /* Compare names */
              rc = strcmp (CMGetCharPtr (v1_str), CMGetCharPtr (v2_str));
              if (rc == 0)
                {
                  /* Cimpare the values. Recursive call. */
                  rc = _compareCIMValue (v1_data, v2_data);
                  if (rc != 0)
                    return rc;
                }
              else
                {
                  return rc;
                }
            }
        }
      /* No changes observed */
      return 0;
    case CMPI_filter:
      v1_str = CMGetSelExpString (v1.filter, &status1);
      v2_str = CMGetSelExpString (v2.filter, &status2);
      if ((status1.rc != CMPI_RC_OK) || (status2.rc != CMPI_RC_OK))
        {
          IND_HLP_DEBUG ("Could not convert SelectExp to string!");
          return -1;
        }
      return strcmp (CMGetCharPtr (v1_str), CMGetCharPtr (v2_str));
    case CMPI_ptr:
      if (v1.dataPtr.length != v2.dataPtr.length)
        return 1;
      return memcmp (v1.dataPtr.ptr, v2.dataPtr.ptr, v1.dataPtr.length);
    case CMPI_chars:
#if defined(DEBUG)
      fprintf (stderr, "%s ?= %s\n", v1.chars, v2.chars);
#endif
      return strcmp (v1.chars, v2.chars);
    case CMPI_boolean:
      return v1.boolean != v2.boolean;
    case CMPI_char16:
      return v1.char16 != v2.char16;
    case CMPI_uint8:
#if defined(DEBUG)
      fprintf (stderr, "%d ?= %d\n", v1.uint8, v2.uint8);
#endif
      return v1.uint8 != v2.uint8;
    case CMPI_sint8:
      return v1.sint8 != v2.sint8;
    case CMPI_uint16:
#if defined(DEBUG)
      fprintf (stderr, "%d ?= %d\n", v1.uint16, v2.uint16);
#endif
      return v1.uint16 != v2.uint16;
    case CMPI_sint16:
      return v1.sint16 != v2.sint16;
    case CMPI_uint32:
#if defined(DEBUG)
      fprintf (stderr, "%ld ?= %ld\n", v1.uint32, v2.uint32);
#endif
      return v1.uint32 != v2.uint32;
    case CMPI_sint32:
      return v1.sint32 != v2.sint32;
    case CMPI_uint64:
      return v1.uint64 != v2.uint64;
    case CMPI_sint64:
      return v1.sint64 != v2.sint64;
    case CMPI_real32:
      return v1.real32 != v2.real32;
    case CMPI_real64:
      return v1.real64 != v2.real64;
    case CMPI_enumeration:
      /* Sigh, this is not implemented. Probe me when this is 
         needed */
    case CMPI_instance:
      /* Sigh, the same as above */
    default:
      fprintf (stderr, "%s(%d): Unsupported data type (%x)!", __FILE__,
               __LINE__, value1.type);
      return 0;
    }
  /* Other data types */
}

/**
 * Return the one with the smalles timer value.
 */
static int
_compareProperty (void *a, void *b)
{
  ind_class_property *l_p = (ind_class_property *) a;
  ind_class_property *r_p = (ind_class_property *) b;

  if (l_p->timer < r_p->timer)
    return -1;
  if (l_p->timer > r_p->timer)
    return 1;
  return 0;
}


static IndErrorT
_cloneData (CMPIData * dst, const CMPIData * src)
{
  CMPIStatus status = { CMPI_RC_OK, NULL };
  ;
  IND_HLP_DEBUG ("_cloneData called.");

  if ((dst == NULL) || (src == NULL))
    return IND_INVALID_ARGS;

  dst->type = src->type;
  dst->state = src->state;

  switch (src->type)
    {

    case CMPI_uint64:
      dst->value.uint64 = src->value.uint64;
      break;

    case CMPI_uint32:
      dst->value.uint32 = src->value.uint32;
      break;

    case CMPI_uint16:
      dst->value.uint16 = src->value.uint16;
      break;

    case CMPI_uint8:
      dst->value.uint8 = src->value.uint8;
      break;

    case CMPI_sint64:
      dst->value.sint64 = src->value.sint64;
      /*dst->value.Long = src->value.Long; */
      break;

    case CMPI_sint32:
      dst->value.sint32 = src->value.sint32;
      /*dst->value.Int = src->value.Int; */
      break;

    case CMPI_sint16:
      dst->value.sint16 = src->value.sint16;
      /*dst->value.Short = src->value.Short; */
      break;

    case CMPI_sint8:
      dst->value.sint8 = src->value.sint8;
      /*dst->value.Byte = src->value.Byte; */
      break;

    case CMPI_real64:
      dst->value.real64 = src->value.real64;
      /* dst->value.Double = src->value.Double; */
      break;

    case CMPI_real32:
      dst->value.real32 = src->value.real32;
      /* dst->value.Float = src->value.Float; */
      break;

    case CMPI_boolean:
      dst->value.boolean = src->value.boolean;
      break;

    case CMPI_char16:
      dst->value.char16 = src->value.char16;
      break;

    case CMPI_instance:
      dst->value.inst = CMClone (src->value.inst, &status);
      break;

    case CMPI_ref:
      dst->value.ref = CMClone (src->value.ref, &status);
      break;

    case CMPI_args:
      dst->value.args = CMClone (src->value.args, &status);
      break;


    case CMPI_filter:
      dst->value.filter = CMClone (src->value.filter, &status);
      break;

    case CMPI_enumeration:
      dst->value.Enum = CMClone (src->value.Enum, &status);
      break;

      /* Array is not used b/c it uses bit-mask flags instead */
    case CMPI_ARRAY:
      break;

    case CMPI_string:
      /* String is not used here b/c it can use a bitmaks. */
      break;

    case CMPI_chars:
      dst->value.chars = strdup (src->value.chars);
      break;

    case CMPI_dateTime:
      dst->value.dateTime = CMClone (src->value.dateTime, &status);
      break;

    case CMPI_ptr:
      dst->value.dataPtr.length = src->value.dataPtr.length;
      dst->value.dataPtr.ptr = malloc (dst->value.dataPtr.length);
      if (dst->value.dataPtr.ptr)
        memcpy (dst->value.dataPtr.ptr, src->value.dataPtr.ptr,
                dst->value.dataPtr.length);
      else
        {
          dst->type = CMPI_null;
        }
      break;

    }

  if ((src->type & CMPI_ARRAY) == CMPI_ARRAY)
    {
      IND_HLP_DEBUG ("Array copy");
      /* This does not really work well, be cautious */
      dst->value.array = CMClone (src->value.array, &status);
    }
  if (((src->type & CMPI_string) == CMPI_string) &&
      (src->type != CMPI_chars) && ((src->type & CMPI_ARRAY) != CMPI_ARRAY))
    {
      IND_HLP_DEBUG ("String copy");
      dst->value.string = CMClone (src->value.string, &status);
    }

  if (status.rc != CMPI_RC_OK)
    {
      IND_HLP_DEBUG ("Copying of data type failed.");
      return IND_CMPI_OPERATION;
    }
  IND_HLP_DEBUG ("_cloneData exited.");
  return IND_OK;
}

static IndErrorT
_releaseData (CMPIData * data)
{
  IND_HLP_DEBUG ("_releaseData called.");

  if (data == NULL)
    return IND_INVALID_ARGS;

  _releaseDataValue (data);
  /* Set the values to null values */

  data->type = CMPI_null;
  data->state = CMPI_nullValue;

  IND_HLP_DEBUG ("_releaseData exited.");
  return IND_OK;
}


static IndErrorT
_releaseDataValue (CMPIData * data)
{
  IND_HLP_DEBUG ("_releaseDataValue called.");
  if (data == NULL)
    return IND_INVALID_ARGS;

  switch (data->type)
    {
      /* Add more types */
    case CMPI_uint16:
      data->value.uint16 = 0;
      break;
    case CMPI_uint64:
      data->value.uint64 = 0;
      break;

    case CMPI_uint32:
      data->value.uint32 = 0;
      break;

    case CMPI_uint8:
      data->value.uint8 = 0;
      break;

    case CMPI_sint64:
      data->value.sint64 = 0;
      break;

    case CMPI_sint32:
      data->value.sint32 = 0;
      break;

    case CMPI_sint16:
      data->value.sint16 = 0;
      break;

    case CMPI_sint8:
      data->value.sint8 = 0;
      break;

    case CMPI_real64:
      data->value.real64 = 0;
      break;

    case CMPI_real32:
      data->value.real32 = 0;
      break;

    case CMPI_instance:
      if (data->value.inst)
        {
          CMRelease (data->value.inst);
          data->value.inst = NULL;
        }
      break;

    case CMPI_ref:
      if (data->value.ref)
        {
          CMRelease (data->value.ref);
          data->value.ref = NULL;
        }
      break;

    case CMPI_args:
      if (data->value.args)
        {
          CMRelease (data->value.args);
          data->value.args = NULL;
        }
      break;

    case CMPI_filter:
      if (data->value.filter)
        {
          CMRelease (data->value.filter);
          data->value.filter = NULL;
        }
      break;

    case CMPI_enumeration:
      if (data->value.Enum)
        {
          CMRelease (data->value.Enum);
          data->value.Enum = NULL;
        }
      break;

    case CMPI_chars:
      if (data->value.chars)
        {
          free (data->value.chars);
          data->value.chars = NULL;
        }
      break;

    case CMPI_dateTime:
      if (data->value.dateTime)
        {
          CMRelease (data->value.dateTime);
          data->value.dateTime = NULL;
        }
      break;

    case CMPI_ptr:

      data->value.dataPtr.length = 0;
      if (data->value.dataPtr.ptr)
        {
          free (data->value.dataPtr.ptr);
          data->value.dataPtr.ptr = NULL;
        }
      break;

    }

  if ((data->type & CMPI_ARRAY) == CMPI_ARRAY)
    {
      IND_HLP_DEBUG ("Array released.");
      /* Should we  go through each element on the list? */
      /*
         count = CMGetArrayCount(data->value.array, &status);
         for (i = 0; i < count; i++)
         {
         temp=CMGetArrayElementAt(data->value.array,
         i, &status);
         _releaseData(&temp);
         }
       */
      if (data->value.array)
        CMRelease (data->value.array);

    }

  if (((data->type & CMPI_string) == CMPI_string) &&
      (data->type != CMPI_chars) && ((data->type & CMPI_ARRAY) != CMPI_ARRAY))

    {
      IND_HLP_DEBUG ("String released.");
      if (data->value.string)
        {
          CMRelease (data->value.string);
          data->value.string = NULL;
        }
    }

  IND_HLP_DEBUG ("_releaseDataValue exited.");
  return IND_OK;
}
