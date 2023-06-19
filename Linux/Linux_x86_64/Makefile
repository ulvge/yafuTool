CC = gcc
TARGET_DIR = ../../obj/Linux_x86_64

INCDIR			= ../../../../Build/include
LIBPATH			= ../../../lib/libipmi/Linux_x86_64
SRCPATH			= ../../Common

#--------------------Include files-----------------------------------------
CFLAGS      += -I$(INCDIR)/
CFLAGS	    += -I../../Common/Include
CFLAGS	    += -I../../Common/Support/Include
CFLAGS      += -I$(INCDIR)/ipmi
CFLAGS      += -I$(INCDIR)/libipmi
CFLAGS      += -I$(INCDIR)/global
CFLAGS      += -I$(INCDIR)/fmh
CFLAGS      += -I$(INCDIR)/flash
CFLAGS      += -I$(INCDIR)/network
CFLAGS      += -I$(INCDIR)/hostname
CFLAGS      += -I$(INCDIR)/encryption

LIBS = $(LIBPATH)/libipmi.a
LIBS += -ldl

#-------------------- YafuFlash --------------------------------------
SRC     += $(SRCPATH)/meupdate.c
SRC     += $(SRCPATH)/kcs.c
SRC     += $(SRCPATH)/bmc_ifc.c
SRC     += $(SRCPATH)/utils.c
SRC     += $(SRCPATH)/ComLine.c
SRC     += $(SRCPATH)/flasher.c
SRC     += $(SRCPATH)/flashcmds.c
SRC     += $(SRCPATH)/main.c

#-------------------- Support -----------------------------------------
SRC	+= $(SRCPATH)/Support/Hash/hashfns.c
SRC	+= $(SRCPATH)/Support/Hash/hashtable.c
SRC	+= $(SRCPATH)/Support/IniParse/iniparse.c
SRC	+= $(SRCPATH)/Support/RexStr/rexstr.c

OBJ = *.o
OUTPUT = $(TARGET_DIR)/Yafuflash

ARC_FLAG=-m64

ifndef ARC
	ARC=x86_64
endif

all:
ifeq ($(ARC),x86_64)
	mkdir -p $(TARGET_DIR)
	$(CC) $(ARC_FLAG) -c $(SRC) $(CFLAGS)
	$(CC) $(ARC_FLAG) -o $(OUTPUT) $(OBJ) $(LIBS)
	@echo -e "\n********************************************************************************"
	@echo -e "output file  stored in /obj/linux_x86_64"
	@echo -e "********************************************************************************"
else
	@echo "Architecture Not Supported"
endif
clean:
	rm -rf $(OUTPUT)
	rm -rf $(OBJ)
