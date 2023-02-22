TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += WinDDE

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install WinDDE.dbd into <top>/dbd
DBD += WinDDE.dbd
INC += WinDDEMsg.h

# specify all source files to be compiled and added to the library
WinDDE_SRCS += WinDDEDriver.cxx
WinDDE_LIBS += asyn
WinDDE_LIBS += $(EPICS_BASE_IOC_LIBS)
WinDDE_SYS_LIBS_WIN32 += user32

PROD_IOC += WinDDEClient
WinDDEClient_SRCS += WinDDEDriverClient.cxx
WinDDEClient_SYS_LIBS_WIN32 += user32

BIN_INSTALLS += ../run_windde.bat ../README.txt
BIN_INSTALLS += $(EPICS_BASE)/bin/$(EPICS_HOST_ARCH)/caRepeater.exe
BIN_INSTALLS += $(EPICS_BASE)/bin/$(EPICS_HOST_ARCH)/caput.exe
ifeq ($(STATIC_BUILD),NO)
BIN_INSTALLS += $(EPICS_BASE)/bin/$(EPICS_HOST_ARCH)/ca.dll
BIN_INSTALLS += $(EPICS_BASE)/bin/$(EPICS_HOST_ARCH)/Com.dll
endif

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

