//
// VMA netmap
//

#ifndef NETMAP_VMA_H
#define NETMAP_VMA_H

#ifdef NETMAP_VMA
#define	nm_open		nm_open_vma
#define	nm_close	nm_close_vma
#define	nm_nextpkt	nm_nextpkt_vma
#define	nm_dispatch	nm_dispatch_vma
struct nm_desc *nm_open_vma(const char *ifname, const struct nmreq *req, uint64_t flags, const struct nm_desc *arg);
int nm_close_vma(struct nm_desc *);
u_char * nm_nextpkt_vma(struct nm_desc *d, struct nm_pkthdr *hdr);
int nm_dispatch_vma(struct nm_desc *d, int cnt, nm_cb_t cb, u_char *arg);
#endif

#endif /* NETMAP_VMA_H */
