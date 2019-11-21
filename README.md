## WinDDEDriver - an EPICS driver to share process variables via the Windows DDE protocol

This is an asyn based driver that you can use in two ways:
* you can add DDE support to an existing IOC
* you can create a standalone application to monitor process variables defined elsewhere and expose these via DDE

Only integer, double and string (plus char waveforms) data types are supported

Requires EPICS asyn 4-32 or higher - edit configure/RELEASE

The driver works by exposing asyn parameters as DDE items in a "getPV" topic. The name of the DDE
service is specified via the WinDDEConfigure() command in st.cmd  
 
The asyn parameters are created dynamically, there is no need to have these specified and compiled in the driver
source code itself. Instead just reference the parameter name in the EPICS DB file you load from st.cmd 
and it will get created automatically at init time. The only caveat is that you need to make sure the 
asyn "address" parameter is specified correctly as this is used to decide on the data type for the parameter. 
So you would use:
```
    field(OUT, "@asyn($(PORT),1,0)double1") 
```
within e.g. an EPICS ao record to tie it to the "double1" DDE item, note the "1" passed as the asyn address
in this case is to indicate a double data type. See *WinDDETest.db* for more explanation and an example. To see changes 
pushed via DDE from elsewhere you could also either create an ai record mapped to the double1 parameter scanning at "I/O Intr", or you could use the *asyn:READBACK* info record on the original ao record.  

When using the driver either standalone or incorporated into an existing driver, you would normally create  
PVs that are mapped to the DDE item names and then push updates to (or monitor) these PV from other PVs in your application to create the complete DDE link.   

If you incorporate the driver into an existing application, you need to modify the ioc main program
as done in   *WinDDETestMain.cpp*   to add a message loop to pump DDE messages.

### Tested with Microsoft Excel

When the WinDDETest IOC is running, create a cell in the spreadsheet with the formula
```
=IOC|getPV!double1
```
you should then see this update when new values are written to the 
process variable *$(P)$(R)DOUBLETEST:SP* defined in *WinDDETest.db*. The "IOC" in =IOC is the name of the DDE service created, which is a parameter of *WinDDEConfigure()* in st.cmd. *getPV* is the name of the DDE topic (this is fixed) and *double1* is the name of the DDE item, which is the asyn driver parameter reference in the epics DB file for the PV as shown above.

### Tested with National Instruments LabVIEW

You need to load  *dde.llb*  from the  *vi.lib/Platform*  folder in the LabVIEW distribution
Setting a value from here was reflected in the *$(P)$(R)DOUBLETEST* PV as well as in Excel

There is an example *ddetest.vi* file in this repository

Any comments/problems, contact: freddie.akeroyd@stfc.ac.uk
