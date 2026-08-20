#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

#define NO_SYS                     1
#define LWIP_SOCKET                0
#define LWIP_NETCONN               0

#define MEM_ALIGNMENT              4
#define MEM_SIZE                   (16 * 1024)

#define MEMP_NUM_PBUF              16
#define MEMP_NUM_TCP_PCB           4
#define MEMP_NUM_TCP_PCB_LISTEN    4
#define MEMP_NUM_UDP_PCB           4
#define MEMP_NUM_SYS_TIMEOUT       10

#define PBUF_POOL_SIZE             8
#define PBUF_POOL_BUFSIZE          1520

#define TCP_MSS                    1460
#define TCP_SND_BUF                (2 * TCP_MSS)
#define TCP_WND                    (2 * TCP_MSS)

#define LWIP_DHCP                  1
#define LWIP_DNS                   1
#define LWIP_ARP                   1

#define LWIP_NETIF_STATUS_CALLBACK 1

#endif /* _LWIPOPTS_H */

