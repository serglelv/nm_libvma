#
#

TARGET = netmap_vma_ex1

VMA_DIR = /your-path/libvma

LIBS = -lpthread  -lrt
CC = gcc
CFLAGS += -g -O3 -Wall -c -fmessage-length=0
CFLAGS += -DNDEBUG -DNETMAP_VMA -I$(VMA_DIR)/src/vma

LIBS += -L $(VMA_DIR)/src/vma/.libs/  -lvma ../netmap_vma/netmap_vma.so
LIBS += -lpthread  -lrt

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBDIR) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
