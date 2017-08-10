
TOP_DIR = ..
include $(TOP_DIR)/nm_vma.mk

TARGET=netmap_vma.so

CPPFLAGS = -Wall -Werror -Wunused -O3 -fPIC -m64 -g
CPPFLAGS += -DHAVE_MP_RQ -DNETMAP_VMA -I$(VMA_SRC_DIR)/src/vma -I$(VMA_SRC_DIR)/src

LDFLAGS = -shared
LIBS = -lpthread -lrt

.PHONY : clean

SOURCES = $(shell echo *.cpp)
HEADERS = $(shell echo *.h)
OBJECTS=$(SOURCES:.cpp=.o)

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)
