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

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

