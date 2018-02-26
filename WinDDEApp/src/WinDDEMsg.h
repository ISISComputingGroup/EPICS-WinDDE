/*************************************************************************\ 
* Copyright (c) 2018 Science and Technology Facilities Council (STFC), GB. 
* All rights reverved. 
* This file is distributed subject to a Software License Agreement found 
* in the file LICENSE.txt that is included with this distribution. 
\*************************************************************************/ 

#ifndef WINDDEMSG_H
#define WINDDEMSG_H

/// private windows message for notifying DDE of epics value change
#define WM_EPICSDDE WM_USER

/// callback function type passed in wParam to message pump via WM_EPICSDDE
typedef void (*dde_cb_func_t)(LPARAM);

#endif /* WINDDEMSG_H */
