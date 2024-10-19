#include <sys/utsname.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>


int main(void)
{
    struct utsname uts;
    int z = uname(&uts);

    printf(" sysname [] = %s\n", uts.sysname);
    printf(" nodename [] = %s\n", uts.nodename);
    printf(" release [] = %s\n", uts.release);
    printf(" version [] = %s\n", uts.version);
    printf(" machine [] = %s\n", uts.machine);
    // printf(" domainname [] = %s\n", uts.domainname);

    z = gethostname(uts.nodename, sizeof(uts.nodename));
    printf(" nodename [] = %s\n", uts.nodename);

    // z = getdomainname(uts.domainname, sizeof(uts.domainname));
    // printf(" domainname [] = %s\n", uts.domainname);

    return 0;
}