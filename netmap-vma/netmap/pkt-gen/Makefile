
TOP_DIR = ../..
include $(TOP_DIR)/nm_vma.mk

PROGS = pkt-gen-vma pkt-gen

CFLAGS = -O2 -Wall -Wextra -g

CFLAGS += -I$(NETMAP_SRC_DIR)/sys/net -I$(NETMAP_SRC_DIR)/apps/include -I$(NETMAP_VMA_SRC_DIR)
CFLAGS += -I$(BITSTREAM_DIR)

LDLIBS_VMA += $(NETMAP_VMA_SRC_DIR)/netmap_vma.so
LDLIBS_VMA += -L $(VMA_SRC_DIR)/src/vma/.libs -lvma 

LDLIBS += -lpthread -lm -lrt

ifdef WITH_PCAP
LDLIBS += -lpcap
else
CFLAGS += -DNO_PCAP
endif

all: $(PROGS)

pkt-gen-vma: LDLIBS += $(LDLIBS_VMA)
pkt-gen-vma.o: pkt-gen.c
	$(CC) $(CFLAGS) -DNETMAP_VMA -c $^ -o $@

pkt-gen.o: pkt-gen.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f pkt-gen pkt-gen.o pkt-gen-vma pkt-gen-vma.o
