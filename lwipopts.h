#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

#define NO_SYS                      1
#define LWIP_TIMEVAL_PRIVATE        0
#define LWIP_RAW                    1
#define LWIP_DNS                    1
#define LWIP_DHCP                   1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETIF_STATUS_CALLBACK  1
#define MEM_ALIGNMENT               4
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_ALTCP                  0
#define LWIP_ALTCP_TLS              0
#define LWIP_ALTCP_TLS_MBEDTLS      0
#define LWIP_TCPIP_CORE_LOCKING     1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

#endif
