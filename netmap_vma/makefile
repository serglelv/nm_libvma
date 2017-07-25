

VMA_DIR = /your-path/libvma

CPPFLAGS = -DHAVE_MP_RQ -DNETMAP_VMA -I$(VMA_DIR)/src/vma

CPPFLAGS += -Wall -Werror -Wunused
CPPFLAGS += -O3 -fPIC -m64 -g  -I$(VMA_DIR)/src/vma -I$(VMA_DIR)/src 

LDFLAGS = -shared
LIBS = -lpthread  -lrt

.PHONY : clean

SOURCES = $(shell echo *.cpp)
HEADERS = $(shell echo *.h)
OBJECTS=$(SOURCES:.cpp=.o)

TARGET=netmap_vma.so

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)
    
$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)
