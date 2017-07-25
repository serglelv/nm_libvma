//
//

#include <stdio.h>
#include <poll.h>
#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>

#include "netmap_vma.h"

#define	DEV_IF	"ens4f0"

int print_period_cnt = 0;

void consume_pkt(u_char *buf, int len)
{
	if (print_period_cnt++ > 1000) {
		print_period_cnt  = 0;
		printf("len=%d\n", len);
	}
}

int main(void)
{
    struct nm_desc *d;
    u_char *buf;
    struct nm_pkthdr h;

    d = nm_open(DEV_IF, NULL, 0, 0);
    if (d == NULL) {
	printf("Unable to open %s\n", DEV_IF);
	exit(1);
    }
    
    for (;;) {
        while ( (buf = nm_nextpkt(d, &h)) ) {
            consume_pkt(buf, h.len);
        }
    }
    nm_close(d);

    return(0);
}


