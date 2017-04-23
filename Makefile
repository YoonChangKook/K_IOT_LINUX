CC = gcc
OBJ = k_pc.o
LINK_OBJ = k_pc.o

############################################
# User's Conf BEGIN
INCS     = -I"C:\libevent4_1\gigaiotplatform_1.0\include" -I"C:\libevent4_1\include"
LIBS = -L"C:\libevent4_1\gigaiotplatform_1.0\lib" -L"C:\libevent4_1\lib"
# User's Conf END
###########################################

ifeq ($(OS),Windows_NT)
# Windows Version
	CC_PROP += -lws2_32
	RM = rm -rf
else
# Linux Version
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CC_PROP += -lrt
		RM = rm -rf
	endif
	
endif

CFLAGS = $(INCS) -g3 

BIN = k_pc.exe

.PHONY: all clean

clean:
	$(RM) $(OBJ) $(BIN)

all: $(OBJ)
	$(CC) -o $(BIN) $(LINK_OBJ) $(LIBS) -lgigaiotplatform -levent_core -lpthread $(CC_PROP)
	$(RM) $(OBJ)

k_pc.o: k_pc.c
	$(CC) $(CFLAGS) -c k_pc.c -o k_pc.o