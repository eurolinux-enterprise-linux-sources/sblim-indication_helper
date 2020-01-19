/**
 * $Id: ind_helper.h,v 1.5 2005/11/10 09:30:12 mihajlov Exp $
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

#include <cmpift.h>

typedef unsigned int Uint32;


typedef Uint32 IndTimeT;

#define IND_TIME_DEFAULT 10;

/*
 The mechanism by which the check function is being compared against.
 Currently there are two logics, as explained below.
*/
typedef Uint32 IndCheckBehaviour;

/*
The polling function (defined as the <li>check</li) will
be pooled and the value passed in
to the  parameter will be checked against the previous value
that was returned earlier. If there is a change, the library
will immediately call all of the property functions, create
an instance and attempt to deliever it (if the evaluation of
the instance against the indication filter is true). 

This detection mechanism is also comonly called 'edge level
detection' or 'trigger detection'. It means that the CIM
Listener will not be bombarded with indications but only
when state changes. 

For example, if the monitor function was passing these
values through its parameter:

0,3,3,3,3,3,0,1,1,1,1,1,0,0,0
 ^         ^ ^         ^
(rise)  (doww&rise)   (level down)

There would only be four indications sent (of course, pending
the evaluation of the constructed instance agains the indication filter).

Caveat: On the first call to the monitor function, the
previous state is not known. The library considers this
first call as the seed call and does not deliever the
instance. Please make sure to structure your code
accordingly and have a check for the first call.

Note: The function prototype for the monitor function is the
same as for retrieving the property values. This means you
can use the same function for both purposes. For example,
assume you have a property named 'OperationStatus' that
returns two values: 2 (OK) or 4 (Stressed). The value of
this property could be the driver for triggering the
indication. Thus you could do this:

ind_reg_pollfnc("CIM_ModifyInstance", "OperationalStatus",
check_os, 10);
ind_set_property_f("root/cimv2","CIM_ModifyInstance","Operational
Instance",
check_os);

*/
#define IND_BEHAVIOR_LEVEL_EDGE 32

/*
The polling function (defined as the <li>check</li) will
be pooled and the value passed in through the parameter
will be checked against the "no-event" value - zero. Any value
other than "no-event" triggers the process of attempting to
send an indication (if the evaluation of the instance against the 
indication filter is true, of course). 

For example, if the monitor function was passing these
values through its parameter:

0,3,3,3,3,3,0,1,1,1,1,1,0,0,0
  ^^^^^^^^^   ^^^^^^^^^ 
  5x event    5x event

That means that there would be 10 indications sent (any value
that is not zero is considered as the go for the indication
creation).

Note that there is no need to have a previous state by the 
"check" function (as opposed to the LEVEL type behaviour). The
library assumes it is 0 (any of the integer values).

Usually the check functions look something akin to this:

IndErrorT check(CMPIData *v)
{

  ind_new = check_OperationalStatus(&ind_OperationalStatus);

  v->state = CMPI_goodValue;
  v->type = CMPI_uint16;
  v->value.uint16 = ind_new;

  // Resetting it to no-event state.
  ind_new = 0;
  return IND_OK;

}

*/

#define IND_BEHAVIOR_AGAINST_ZERO 64
/**
 * Return codes.
 */
typedef Uint32 IndErrorT;

/**
 * OK
 */
#define IND_OK                (IndErrorT)0x0000

#define IND_BASE              -1000

/**
 * Return codes for all of the functions
 */
#define IND_INVALID_ARGS      (IndErrorT)(IND_BASE - 1)

#define IND_NOT_FOUND         (IndErrorT)(IND_BASE - 2)

#define IND_DUPLICATE         (IndErrorT)(IND_BASE - 3)

#define IND_NOT_TIED_TOGETHER (IndErrorT)(IND_BASE - 4)

#define IND_NOT_REGISTERED    (IndErrorT)(IND_BASE - 5)

#define IND_LOCKED            (IndErrorT)(IND_BASE - 6)

#define IND_MUTEX             (IndErrorT)(IND_BASE - 7)

#define IND_NO_MEMORY         (IndErrorT)(IND_BASE - 8)

#define IND_CMPI_OPERATION    (IndErrorT)(IND_BASE - 9)

/**
 * Register the CMPI broker. This can be done before or 
 * after registering properties to monitor and properties
 * for the the indication class. After calling this routine
 * you must call "ind_start()" to start the polling mechanism.
 *
 * @param broker CMPIBroker and not null.
 *
 * @return IND_DUPLICATE The broker has already been registered.
 * @return IND_NO_MEMORY Not enough memory to fulfill the operation.
 */
IndErrorT ind_reg (const CMPIBroker *, const CMPIContext *);

/**
 * Start the library. This will start the polling activity.
 * To stop the polling activity use 'ind_stop' facility.
 *
 * @return IND_MUTEX Problem locking the internal data structure. This
 * is a serious issue, please look at the mailing list.
 */
IndErrorT ind_start(void );

/**
 * Stop the library. This will stop any polling activity. To
 * re-start the polling activity use 'ind_start' facility.
 *
 * @return IND_MUTEX Problem locking the internal data structure. This
 * is a serious issue, please look at the mailing list.
 */
IndErrorT ind_stop(void );


/**
 * Shuts down the library. De-registers any registerd functions
 * properties for monitored class or indication class. You must
 * call this function to stop the threading activity.
 *
 * @return IND_MUTEX Problem locking the internal data structure. This
 * is a serious issue, please look at the mailing list.
 */
IndErrorT ind_shutdown(void );

/**
 * +-----------------------------------
 * + 
 * +    Monitored class operations.
 * +
 * +-----------------------------------
 */


/**
 * Register the polling function. This routine will call
 * the registered function right away, so make sure that
 * there are no dead-lock conditions. For situations in 
 * which this class is for different namespaces, used
 * the 'ind_reg_pollfnc_ns' function.
 *
 * @param cn The class name in which the property exist.
 * @param pn The name of the property which is being polled
 * against.
 * @param check User function performing the check. The
 * new value of the property, along with the type, are 
 * returned as arguments. The return value should be IND_OK
 * or any other matching error code.
 * @param timer Polling interval in seconds. If you have no
 * need for fine-grain optimization use the 'IND_TIME_DEFAULT'
 * value.
 * @param logic The logic for polling function. There are currently
 * two logics: IND_BEHAVIOUR_LEVEL_EDGE and IND_BEHAVIOUR_AGAINST_ZERO.
 * Please see the comments for those defines to find the full description.
 *
 * @return IND_INVALID_ARGS The 'cn' or 'pn' values are zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_DUPLICATE The entry already has been registered. Use 
 * ind_set_pollfnc instead.
 * @return IND_NO_MEMORY Not enough memory to fulfill the operation.
 * @return IND_CMPI_OPERATION Could not translate the value returned
 * by 'check' function into a valid one.
 */

IndErrorT ind_reg_pollfnc(const char *cn,
			  const char *pn, 
			  IndErrorT (*check) (CMPIData *),
			  IndTimeT timer, 
			  IndCheckBehaviour logic);

/**
 * Register the polling function. This routine will call
 * the registered function right away, so make sure that
 * there are no dead-lock conditions. 
 *
 * @param ns The namespace in which the class exist.
 * @param cn The class name in which the property exist.
 * @param pn The name of the property which is being polled
 * against.
 * @param check User function performing the check. The
 * new value of the property, along with the type, are 
 * returned as arguments. The return value should be IND_OK
 * or any other matching error code.
 * @param timer Polling interval in seconds. If you have no
 * need for fine-grain optimization use the 'IND_TIME_DEFAULT'
 * value.
 * @param logic The logic for polling function. There are currently
 * two logics: IND_BEHAVIOUR_LEVEL_EDGE and IND_BEHAVIOUR_AGAINST_ZERO.
 * Please see the comments for those defines to find the full description.
 *
 * @return IND_INVALID_ARGS The 'cn' or 'pn' values are zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_DUPLICATE The entry already has been registered. Use 
 * ind_set_pollfnc instead.
 * @return IND_NO_MEMORY Not enough memory to fulfill the operation.
 * @return IND_CMPI_OPERATION Could not translate the value returned
 * by 'check' function into a valid one.
 */

IndErrorT ind_reg_pollfnc_ns(const char *ns,
			     const char *cn,
			     const char *pn, 
			     IndErrorT (*check) (CMPIData *),
			     IndTimeT timer,
			     IndCheckBehaviour logic);


/**
 * Function to unregister the poll function. For situation in which
 * 'ind_reg_pollfnc_ns' was used, use the 'ind_unreg_poll_ns'.
 *
 * @param cn The class name in which the property exist.
 * @param pn The property name.
 *
 * @return IND_INVALID_ARGS The 'cn' or 'p' is zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_LOCKED The entry cannot be deleted because it is currently
 * being serviced. Wait until it is done and retry again.
 */
IndErrorT ind_unreg_pollfnc( const char *cn,
			     const char *p );


/**
 * Function to unregister the poll function. For situation in which
 * 'ind_reg_pollfnc_ns' was used, use the 'ind_unreg_poll_ns'.
 *
 * @param ns The namespace in which the property exist.
 * @param cn The class name in which the property exist.
 * @param pn The property name.
 *
 * @return IND_INVALID_ARGS The 'cn' or 'p' is zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_LOCKED The entry cannot be deleted because it is currently
 * being serviced. Wait until it is done and retry again.
 * @return IND_NOT_FOUND The specified property could not be found.
 */
IndErrorT ind_unreg_pollfnc_ns(const char *ns, 
			       const char *cn,
			       const char *p );


/**
 * Function to set the timer for the polling function. For situations
 * in which you use the class for many namespaces, use the 'ind_set_pollfnc_ns'
 * instead.
 *
 * @param cn The class name in which the property exist.
 * @param pn The property name. 
 *
 * @return IND_INVALID_ARGS Either the class name or property is zero length, 
 * or the check function is null.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_LOCKED The entry cannot be deleted because it is currently
 * being serviced. Wait until it is done and retry again.
 */
IndErrorT ind_set_pollfnc(const char *cn,
			  const char *pn,
			  IndErrorT (*check) (CMPIData *));


/**
 * Function to set the timer for the polling function. For situations
 * in which you use the class for many namespaces, use the 'ind_set_pollfnc_ns'
 * instead.
 *
 * @param ns The namespace in which the class exist.
 * @param cn The class name in which the property exist.
 * @param pn The property name. 
 *
 * @return IND_INVALID_ARGS Either the class name or property is zero length, 
 * or the check function is null.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_LOCKED The entry cannot be deleted because it is currently
 * being serviced. Wait until it is done and retry again.
 */
IndErrorT ind_set_pollfnc_ns(const char *ns,
			     const char *cn,
			     const char *pn,
			     IndErrorT (*check) (CMPIData *));

/**
 * Set the polling inteval timer. If you do not specify a property
 * (pass in "") then the timer will be set for all of the properties
 * if the specified class. For classes which are in multiple namespaces,
 * use 'ind_set_timer_ns'.
 *
 * @param cn The class name in which the property exist.
 * @param pn The property name.
 * @param timer The timer in seconds. Use IND_TIME_DEFUALT
 * for implementation default value.
 *
 * @return IND_INVALID_ARGS The class name is zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_LOCKED The entry cannot be deleted because it is currently
 * being serviced. Wait until it is done and retry again.
 */
IndErrorT ind_set_timer(const char *cn,
			const char *pn,
			IndTimeT timer);

/**
 * Set the polling inteval timer. If you do not specify a property
 * (pass in "") then the timer will be set for all of the properties
 * if the specified class. For classes which are in multiple namespaces,
 * use 'ind_set_timer_ns'.
 *
 * @param cn The class name in which the property exist.
 * @param pn The property name.
 * @param timer The timer in seconds. Use IND_TIME_DEFUALT
 * for implementation default value.
 *
 * @return IND_INVALID_ARGS The class name is zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_LOCKED The entry cannot be deleted because it is currently
 * being serviced. Wait until it is done and retry again.
 */
IndErrorT ind_set_timer_ns(const char *ns,
			   const char *cn,
			   const char *pn,
			   IndTimeT timer);

/**
 * +-----------------------------------
 * + 
 * +    Indication class 
 * +
 * +-----------------------------------
 */

/**
 * Register the classname and properties of the indication 
 * class to be used for the specified monitored class. You
 * have to class 'ind_set_classes' when you are done setting
 * these values. Be aware that this routine will call the 
 * passed in function, so make sure there are no dead-locks.
 * <br><br>
 *
 * Note: If you have pre-set values use 'ind_set_property_d'
 * or 'ind_set_properties_d' instead.<br><br>
 * 
 * Example:<br>
 * <pre>
 *   IndErrorT hostname(CMPIData *d) {
 *     struct hostent *host;
 * 
 *     host = gethostbyname("localhost");
 *
 *     d->type = CMPI_chars;
 *     d->value.chars = strdup(host->h_name);
 *
 *     return IND_OK;
 *   }
 *
 *   ind_set_property_f("root/cimv2","CIM_AlertIndication",
 *                      "SystemName", hostname);
 *
 * </pre>
 *
 * @param ns The namespace of the indication class.
 * @param ci The indication class which the library should
 * dispatch when a change occurs.
 * @param p The name of the property which exist in the 
 * indication class.
 * @param check User function performing assembling the
 * property. The new value of the property, along with the 
 * type, are returned as arguments. The return value of 
 * set function should be IND_OK or any other matching 
 * error code if an unexpected error occured.
 *
 * @return IND_INVALID_ARGS The 'ns' or 'ci' or 'p' is zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_DUPLICATE The entry already has been registered. Use 
 * ind_set_property instead.
 * @return IND_NO_MEMORY Not enough memory to fulfill the operation.
 * @return IND_CMPI_OPERATION Could not translate the value returned
 * by 'check' function into a valid one.
 */

IndErrorT ind_set_property_f(const char *ns,
			     const char *ci,
			     const char *p, 
			     IndErrorT (*getAlertProperty) (CMPIData *));

/**
 * Set the default value assigned to the property of the indication 
 * class to be used by this library when sending indications.
 *
 * @param ns The namespace of the indication class.
 * @param ci The indication class which the library should
 * dispatch when a change occurs.
 * @param p The name of the property which exist in the 
 * indication class.
 * @param d The data  of the property.
 */
IndErrorT ind_set_property_d(const char *ns,
			     const char *ci,
			     const char *p,
			     const CMPIData *d);

/**
 * Unregisters the default value or function assigned to a property
 * of the indication class.
 *
 * @param ns The namespace of the indication class.
 * @param ci The indication class which the library should
 * dispatch when a change occurs.
 * @param p The name of the property which exist in the 
 * indication class.
 *
 * @return IND_INVALID_ARGS The 'cn' or 'p' is zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_LOCKED The entry cannot be deleted because it is currently
 * being serviced. Wait until it is done and retry again.
 * @return IND_NOT_FOUND The specified property could not be found.
 */

IndErrorT ind_unreg_property(const char *ns,
			     const char *ci,
			     const char *p);
/**
 * Set the array of  properties of the indication class to be used by
 * this library when sending indications. The properties and their
 * respective values are assumed to be an pointer array, of specified length.
 * <br><br>
 * Example:<br>
 * <pre>
 *   const char *pn[10];
 *   char temp[10];
 *   const CMPIData *data[10];
 *   int i;
 *  
 *   for (i = 0; i < 10; i++) {
 *      data[i] = (CMPIData *)malloc(sizeof(CMPIData));
 *      data[i]->type = CMPI_uint32;
 *      data[i]->value.uint16 = i;
 *
 *      memset(&temp, 0x00, 10);
 *      snprintf(temp, 10, "%s_%d","Count",i);
 *      pn[i] = (char *) strdump(temp);
 *   }
 *   ind_set_properties_d("root/cimv2","CIM_someclass",
 *                        pn, data, 10);
 * </pre>
 *
 * @param ns The namespace of this indication class.
 * @param ci The indication class which the library should
 * dispatch when a change occurs. 
 * @param p The array of the properties which exist in the 
 * indication class.
 * @param d The array of data of the properties.
 * @param l The length of the array.
 */
IndErrorT ind_set_properties_d(const char *ns,
			       const char *ci,
			       const char *p[],
			       const CMPIData *d[],
			       size_t l);

/**
 * Set the array of  properties of the indication class to be used by
 * this library when sending indications. The properties and their
 * respective values are assumed to be an static array, of specified length.
 * <br><br>
 * Example:<br>
 * <pre>
 *   #define NUMBER_OF_PROPERTIES 2
 *   char *CIM_AlertIndication[] = 
 *    {"Description",
 *      ...
 *    {"ProviderName"};
 *
 *   CMPIData CIM_AlertIndication_DATA[] =
 *   {{.type = CMPI_chars,
 *     .state = CMPI_goodValue,
 *     .value = {
 *       .chars = "Startup indication"
 *     }},
 *   ....
 *    {.type = CMPI_chars, .state=CMPI_goodValue,.value=
 *     {.chars = __FILE__}}};
 *
 *   ind_set_properties_da("root/cimv2","CIM_AlertIndication",
 *                         CIM_AlertIndication,
 *                         &CIM_AlertIndication_DATA, 
 *                         NUMBER_OF_PROPERTIES);
 * </pre>
 *
 * @param ns The namespace of this indication class.
 * @param ci The indication class which the library should
 * dispatch when a change occurs. 
 * @param p The array of the properties which exist in the 
 * indication class.
 * @param d The array of data of the properties staticly defined.
 * @param l The length of the array.
 */

IndErrorT ind_set_properties_da(const char *ns,
				const char *ci,
				const char *p[],
				const CMPIData *d,
				size_t l);

/**
 * Set the array of properties of the indication class to be used by
 * this library when sending indications. The properties and their
 * respective check functions are assumed to be an array, of specified length
 *
 * @param ns The namespace of this indication class.
 * @param ci The indication class which the library should
 * dispatch when a change occurs. 
 * @param p The array of the properties which exist in the 
 * indication class.
 * @param getAlertPropert The array of functions which provides the value
 * for the property.
 * @param l The length of the array.
 */
IndErrorT ind_set_properties_f(const char *ns,
			       const char *ci,
			       const char *p[],
			       IndErrorT (**getAlertProperty) (CMPIData *),
			       size_t l);



/**
 * Unregisters the default values or functions assigned to
 * properties  of the indication class.
 *
 * @param ns The namespace of the indication class.
 * @param ci The indication class which the library should
 * dispatch when a change occurs.
 * @param p The array of  properties which exist in the 
 * indication class.
 * @param l The length of the array.
 *
 * @return IND_INVALID_ARGS The 'cn' or 'ns' is zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_LOCKED The entry cannot be deleted because it is currently
 * being serviced. Wait until it is done and retry again.
 */

IndErrorT ind_unreg_properties(const char *ns,
			       const char *ci,
			       const char *p[],
			       size_t l);


/**
 * +-----------------------------------
 * + 
 * +    Indication and monitored class shared function.
 * +
 * +-----------------------------------
 */

/**
 * Specify which indication class is tied in with the monitored
 * class. This <b>MUST</b> be called after all of the properties
 * have been set, otherwise the library won't know what indication
 * to generate.
 *
 * @param ns The namespace in which the indication class resides
 * @param cn The indication class name.
 * @param ci The class name of the monitored properties.
 *
 * @return IND_NOT_FOUND Could not find any of the classes. Make sure you are passing 
 * the monitor class name and then the indication class name.
 * @return IND_INVALID_ARGS The 'cn' or 'ci' or 'ns' values are zero length.
 * @return IND_MUTEX Problem locking the internal list. This is a serious
 * problem. Consult the mailing list.
 * @return IND_NO_MEMORY Not enough memory to fulfill the operation.
 */
IndErrorT ind_set_classes(const char *ns,
			  const char *cn,
			  const char *ci);


/**
 * +-----------------------------------
 * + 
 * +    Provider specific operation
 * +
 * +-----------------------------------
 */

/**
 * Set the query select on the Indication class. This is
 * done usually in ActivateFilter.
 *
 * @param ns The namespace of the class.
 * @param ci The indication class.
 * @param filter The query select statement for the indication
 * class
 */

IndErrorT ind_set_select(const char *ns,
			 const char *ci,
			 const CMPISelectExp *filter);

/**
 * Unregisters the query select on the Indication class. This is
 * done usually in DeActivateFilter.
 *
 * @param ns The namespace of the class.
 * @param ci The indication class.
 * @param filter The query select statement for the indication
 * class
 */

IndErrorT ind_unreg_select(const char *ns,
			   const char *ci,
			   const CMPISelectExp *filter);
