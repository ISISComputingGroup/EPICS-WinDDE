## WinDDEDriver - an EPICS driver to share process variables via the Windows DDE protocol

This is an asyn based driver that you can use in two ways:
* you can add DDE support to an existing IOC
* you can create a standalone application to monitor process variables defined elsewhere and expose these via DDE

Only integer, double and string (plus char waveforms) data types are supported

Requires EPICS asyn 4-32 or higher - edit configure/RELEASE

The driver works by exposing asyn parameters as DDE items in a "getPV" topic. The name of the DDE
service is specified via the WinDDEConfigure() command in st.cmd  
 
The asyn parameters are created dynamically, there is no need to have these specified in the driver
source code itself. Instead just reference the parameter name in the EPICS DB file you load from st.cmd 
and it will get created automatically at init time. The onlt caveat is that you need to make sure the 
asyn "address" parameter is correct as this is used to decide on data type for the parameter. So you would use:
```
    field(OUT, "@asyn($(PORT),1,0)double1") 
```
within e.g. an EPICS ao record to tie it to the "double1" DDE item, note the "1" passed as the asyn address
is to indicate a double data type. See WinDDETest.db for more explanantion and an example.

When using the driver either standalone or incorporated into another driver, you would create additional 
PVs that mapped to the DDE item names and then push updates to (or monitor) these PV to form the DDE link.   

If you incorporate the driver into an existing application, you need to modify the ioc main program
as done in   WinDDETestMain.cpp   to add a message loop to pump DDE messages.

### Tested with Microsoft Excel

Create a cell with the formula      =IOC|getPV!long1      you should then see changes written to the 
process variable $(P)$(R)LONGTEST:SP defined in WinDDETest.db  when running the WinDDETest IOC

### Tested with National Instruments LabVIEW

You need to load  dde.llb  from the  vi.lib/Platform  folder in the LabVIEW distribution
Setting a value from here was reflected in the $(P)$(R)LONGTEST PV as well as in Excel

Any comments/problems, contact: freddie.akeroyd@stfc.ac.uk