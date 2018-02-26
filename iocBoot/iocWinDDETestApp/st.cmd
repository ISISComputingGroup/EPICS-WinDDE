#!../../bin/windows-x64/WinDDETest

## You may have to change WinDDETest to something else
## everywhere it appears in this file

# Increase this if you get <<TRUNCATED>> or discarded messages warnings in your errlog output
errlogInit2(65536, 256)

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/WinDDETest.dbd"
WinDDETest_registerRecordDeviceDriver pdbbase

## arg1: asyn port name  arg2: DDE service name
WinDDEConfigure("L0","IOC")

## Load our record instances
dbLoadRecords("db/WinDDETest.db","P=$(MYPVPREFIX),R=DDE:,PORT=L0")

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59"
