
ifeq ($(ARC),x86_64)
SUBDIRS = Linux_x86_64
ARC=x86_64
else 
ifeq ($(ARC),x86_32)
SUBDIRS = Linux_x86_32
ARC=x86_32
else 
ifeq (${shell arch}, x86_64)
SUBDIRS = Linux_x86_64
ARC=x86_64
else
SUBDIRS = Linux_x86_32
ARC = x86_32
endif
endif
endif

export ARC

# Add all subdirs
all: subdirs

subdirs:
	@echo -e "\n***********************\nBuilding For $(ARC) architecture\n***********************"
	 for n in $(SUBDIRS); do $(MAKE) -C $$n all  || exit 1; done

clean:
	@echo -e "\n***********************\nBuilding For $(ARC) architecture\n***********************"
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done

