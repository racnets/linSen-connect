TARGET = linSen-connect

C_SRCS = main.c i2c.c linSen.c linSen-socket.c

INLCUDES = -I.

CFLAGS = -Wall
#CFLAGS += -pedantic -O0 -std=c99
DFLAGS =
LDFLAGS =
LDLIBS = 

GUILIB = gtk+-3.0
# add graphical output if supported by host system
ifeq  ($(shell pkg-config --exists $(GUILIB) && echo 1), 1)
	C_SRCS += gtk-viewer.c
	CFLAGS += `pkg-config --cflags $(GUILIB)` -D GTK_GUI
	LDLIBS += `pkg-config --libs $(GUILIB)`
endif	

C_OBJS = $(patsubst %.c, %.o, $(C_SRCS))

CC = gcc

all: $(TARGET)
	
$(TARGET): $(C_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $(TARGET) $(C_OBJS)

$(C_OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(INCLUDES) -c $< -o $@ 

clean:
	$(RM) $(TARGET) $(C_OBJS)
