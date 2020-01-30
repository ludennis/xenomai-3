/* Copyright 1990-2017 The MathWorks, Inc. */
/*
 * @file: RTWCG_FMU1_target.h
 *  
 * @brief fmustruct 
 *
 * Abstract:
 *      FMI 1.0 data types, function signatures and FMU2_CS_RTWCG(struct)
 *
 *      FMU1_CS_RTWCG is a data struct to store FMU info, handle all 
 *      dynamic function calls to FMU lib 
 *      
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <simstruc.h>
#include <slsv_diagnostic_codegen_c_api.h>
#include "fmiFunctions.h"

/*raccel_utils.h is needed by win plateform to mute warning warning C4013*/
#include <raccel_utils.h>

#ifndef __FMU1_COMMON
#define __FMU1_COMMON

#ifdef _WIN32
#include "windows.h"
#define FMUHANDLE                      HMODULE
#define LOAD_LIBRARY(LIBRARY_LOC)      LoadLibrary(LIBRARY_LOC)
#define LOAD_FUNCTION                  GetProcAddress
#define CLOSE_LIBRARY                  FreeLibrary

#else
#include <dlfcn.h>
#define FMUHANDLE                      void*
#define LOAD_LIBRARY(LIBRARY_LOC)      dlopen(LIBRARY_LOC, RTLD_NOW)
#define LOAD_FUNCTION                  dlsym
#define CLOSE_LIBRARY                  dlclose
#endif

#define fmiTrue  1
#define fmiFalse 0

#define FULL_FCN_NAME_MAX_LEN 256
#define FCN_NAME_MAX_LEN      28

/*typedef default function type*/
typedef fmiStatus (*_fmi_default_fcn_type) (fmiComponent, ...);

/***************************************************
Types for FMI 1.0 Common Functions
****************************************************/
typedef const char* (*_fmiGetVersion)(void);
typedef fmiStatus (*_fmiSetDebugLogging)    (fmiComponent c, fmiBoolean loggingOn);
typedef fmiStatus (*_fmiSetReal)   (fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiReal    value[]);
typedef fmiStatus (*_fmiSetInteger)(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiInteger value[]);
typedef fmiStatus (*_fmiSetBoolean)(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiBoolean value[]);
typedef fmiStatus (*_fmiSetString) (fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiString  value[]);
typedef fmiStatus (*_fmiGetReal)   (fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiReal    value[]);
typedef fmiStatus (*_fmiGetInteger)(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiInteger value[]);
typedef fmiStatus (*_fmiGetBoolean)(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiBoolean value[]);
typedef fmiStatus (*_fmiGetString) (fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiString  value[]);
#endif

/*Co-Simulation*/
typedef const char* (*_fmiGetTypesPlatform)(void);
typedef fmiComponent (*_fmiInstantiateSlave)  (fmiString  instanceName, fmiString  fmuGUID, fmiString  fmuLocation, 
                                               fmiString  mimeType, fmiReal timeout, fmiBoolean visible, fmiBoolean interactive, 
                                               fmiCallbackFunctions functions, fmiBoolean loggingOn);
typedef fmiStatus (*_fmiInitializeSlave)(fmiComponent c, fmiReal tStart, fmiBoolean StopTimeDefined, fmiReal tStop);
typedef fmiStatus (*_fmiTerminateSlave)   (fmiComponent c);
typedef fmiStatus (*_fmiResetSlave)       (fmiComponent c);
typedef void      (*_fmiFreeSlaveInstance)(fmiComponent c);
typedef fmiStatus (*_fmiSetRealInputDerivatives)(fmiComponent c, const  fmiValueReference vr[], size_t nvr,
                                                 const fmiInteger order[], const  fmiReal value[]);                                                  
typedef fmiStatus (*_fmiGetRealOutputDerivatives)(fmiComponent c, const fmiValueReference vr[], size_t  nvr,
                                                  const fmiInteger order[], fmiReal value[]);                                              
typedef fmiStatus (*_fmiCancelStep)(fmiComponent c);
typedef fmiStatus (*_fmiDoStep)(fmiComponent c, fmiReal currentCommunicationPoint, 
                                fmiReal communicationStepSize, fmiBoolean newStep);
typedef fmiStatus (*_fmiGetStatus)       (fmiComponent c, const fmiStatusKind s, fmiStatus*  value);
typedef fmiStatus (*_fmiGetRealStatus)   (fmiComponent c, const fmiStatusKind s, fmiReal*    value);
typedef fmiStatus (*_fmiGetIntegerStatus)(fmiComponent c, const fmiStatusKind s, fmiInteger* value);
typedef fmiStatus (*_fmiGetBooleanStatus)(fmiComponent c, const fmiStatusKind s, fmiBoolean* value);
typedef fmiStatus (*_fmiGetStringStatus) (fmiComponent c, const fmiStatusKind s, fmiString*  value);

struct FMU1_CS_RTWCG {
    _fmiGetTypesPlatform getTypesPlatform;
    _fmiGetVersion getVersion;
    _fmiSetDebugLogging setDebugLogging;
    _fmiSetReal setReal;
    _fmiSetInteger setInteger;
    _fmiSetBoolean setBoolean;
    _fmiSetString setString;
    _fmiGetReal getReal;
    _fmiGetInteger getInteger;
    _fmiGetBoolean getBoolean;
    _fmiGetString getString;
    _fmiInstantiateSlave instantiateSlave;
    _fmiInitializeSlave initializeSlave;
    _fmiTerminateSlave terminateSlave;
    _fmiResetSlave resetSlave;
    _fmiFreeSlaveInstance freeSlaveInstance;
    _fmiGetRealOutputDerivatives getRealOutputDerivatives;
    _fmiSetRealInputDerivatives setRealInputDerivatives;
    _fmiDoStep doStep;
    _fmiCancelStep cancelStep;
    _fmiGetStatus getStatus;
    _fmiGetRealStatus getRealStatus;
    _fmiGetIntegerStatus getIntegerStatus;
    _fmiGetBooleanStatus getBooleanStatus;
    _fmiGetStringStatus getStringStatus;

    char * fmuname;
    char * dllfile;
    FMUHANDLE Handle;
    fmiComponent mFMIComp;
    SimStruct *ssPtr;
    fmiStatus FMUErrorStatus;
};

void fmu1Logger(fmiComponent c, fmiString instanceName, fmiStatus status,
                      fmiString category, fmiString message, ...);
void * FMU1_fmuInitializeCS( const char * lib, fmiString instanceName, fmiString fmuGUID, fmiString fmuLocation, void* ssPtr);

fmiStatus  FMU1_doStep(void **fmuv, double currentCommunicationPoint,double communicationStepSize);

fmiBoolean FMU1_terminateSlave(void **fmuv);
fmiBoolean FMU1_initializeSlave(void **fmuv, fmiReal tStart);

fmiStatus FMU1_setRealVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiReal dvalue);
fmiStatus FMU1_setReal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiReal value[]);
fmiStatus FMU1_getReal(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiReal value[]);
fmiStatus FMU1_setIntegerVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiInteger dvalue);
fmiStatus FMU1_setInteger(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiInteger value[]);
fmiStatus FMU1_getInteger(void **fmuv, const fmiValueReference vr, size_t nvr, fmiInteger value[]);
fmiStatus FMU1_setBooleanVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiBoolean dvalue);
fmiStatus FMU1_setBoolean(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiBoolean value[]);
fmiStatus FMU1_getBoolean(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiBoolean value[]);
fmiStatus FMU1_setString(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiString value[]);
fmiStatus FMU1_setStringVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiString);
fmiStatus FMU1_getString(void **fmuv, const fmiValueReference vr, size_t nvr, fmiString value[]);

void* FMU1_fmuInitialize( const char * lib, fmiString instanceName, fmiString fmuGUID, fmiString fmuLocation, void* ssPtr);

