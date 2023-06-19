CC = gcc
TARGET_DIR = ../../obj/Linux_x86_32

INCDIR			= ../../../../Build/include
LIBPATH			= ../../../lib/libipmi/Linux_x86_32
SRCDIR			= ../../Common

#--------------------Include files-----------------------------------------
CFLAGS      += -I$(INCDIR)/
CFLAGS      += -I$(INCDIR)/ipmi
CFLAGS      += -I$(INCDIR)/libipmi
CFLAGS      += -I$(INCDIR)/global
CFLAGS      += -I$(INCDIR)/fmh
CFLAGS      += -I$(INCDIR)/flash
CFLAGS      += -I$(INCDIR)/network
CFLAGS      += -I$(INCDIR)/hostname
CFLAGS      += -I$(INCDIR)/encryption
CFLAGS	    += -I../../Common/Include
CFLAGS	    += -I../../Common/Support/Include

LIBS = $(LIBPATH)/libipmi.a
LIBS += -ldl

#-------------------- YafuFlash --------------------------------------
SRC     += $(SRCDIR)/meupdate.c
SRC     += $(SRCDIR)/kcs.c 
SRC     += $(SRCDIR)/bmc_ifc.c 
SRC     += $(SRCDIR)/utils.c 
SRC     += $(SRCDIR)/ComLine.c
SRC     += $(SRCDIR)/flasher.c
SRC     += $(SRCDIR)/flashcmds.c
SRC     += $(SRCDIR)/main.c

#-------------------- Support -----------------------------------------
SRC	+= $(SRCDIR)/Support/Hash/hashfns.c
SRC	+= $(SRCDIR)/Support/Hash/hashtable.c
SRC	+= $(SRCDIR)/Support/IniParse/iniparse.c
SRC	+= $(SRCDIR)/Support/RexStr/rexstr.c

OBJ = *.o
OUTPUT = $(TARGET_DIR)/Yafuflash

ARC_FLAG=-m32

ifndef ARC
	ARC=x86_32
endif

all:
ifeq ($(ARC),x86_32)
	mkdir -p $(TARGET_DIR)
	$(CC) $(ARC_FLAG)  -c $(SRC) $(CFLAGS)   
	$(CC) $(ARC_FLAG) -o $(OUTPUT) $(OBJ) $(LIBS)
	@echo -e "\n********************************************************************************"
	@echo -e "output file  stored in /obj/linux_x86_32"
	@echo -e "********************************************************************************"
else
	@echo "Architecture Not Supported"
endif
clean:
	rm -rf $(OUTPUT)
	rm -rf $(OBJ)
