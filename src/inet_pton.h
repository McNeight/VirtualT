#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#endif

#ifndef IN6ADDRSZ
#define IN6ADDRSZ       16
#endif

#ifndef INT16SZ
#define INT16SZ         2
#endif
  /*
   * End of Net-SNMP Win32 additions
   */

#ifndef INADDRSZ
#define INADDRSZ        4
#endif


typedef unsigned char u_char;
int inet_pton(int af, char *src, void *dst);
