//////////////////////////////////////////////////////////////////
//
// rtss_code.h - header file
//
// This file was generated using the RTX64 Application Template for Visual Studio.
//
// Created: 2/12/2020 11:42:49 AM 
// User: d400
//
//////////////////////////////////////////////////////////////////

#pragma once
//This define will deprecate all unsupported Microsoft C-runtime functions when compiled under RTSS.
//If using this define, #include <rtapi.h> should remain below all windows headers
//#define UNDER_RTSS_UNSUPPORTED_CRT_APIS

#include <SDKDDKVer.h>

//#include <stdio.h>
//#include <string.h>
//#include <ctype.h>
//#include <conio.h>
//#include <stdlib.h>
//#include <math.h>
//#include <errno.h>
#include <windows.h>
#include <tchar.h>
#include <rtapi.h>    // RTX64 APIs that can be used in real-time and Windows applications.

#ifdef UNDER_RTSS
#include <rtssapi.h>  // RTX64 APIs that can only be used in real-time applications.
#endif // UNDER_RTSS

  
