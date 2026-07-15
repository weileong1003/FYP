#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define ENERGEST_CONF_ON 1

#define LOG_CONF_LEVEL_IPV6   LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_RPL    LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_TCPIP  LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_MAC    LOG_LEVEL_NONE

#define RPL_CONF_DIO_INTERVAL_MIN 8

#define NETSTACK_CONF_WITH_IPV6 1
#define UIP_CONF_IPV6 1
#define UIP_CONF_ROUTER 1

/* ======== MRHOF ======== */
 #define RPL_CONF_OF_OCP             RPL_OCP_MRHOF


/* ======== OF0 ======== */
/*#define RPL_CONF_OF_OCP            RPL_OCP_OF0 */

#define RPL_CONF_SUPPORTED_OFS     { &rpl_of0, &rpl_mrhof }

#endif 
