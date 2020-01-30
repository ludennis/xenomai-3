/* Copyright 1990-2017 The MathWorks, Inc. */

/******************************************************************************
 *                                                                            *
 * File    : RTWCG_FMU1_target.c                                              *
 * Abstract:                                                                  *
 *      Wrapper functions to dynamic call libraries of FMU 1.0                *
 *      This File define functions called by CGIR code                        *
 *      Also handle errors, and logger                                        *
 *                                                                            *
 * Author : Brayden Xia, Nov/2017                                             *
 *                                                                            *
 * TODO:                                                                      *
 *      Support String Parameters                                             *
 *      test all parameters, test err conditions                              *
 *      Support FMU ME                                                        *
 *      *Model reference                                                      *
 ******************************************************************************/

#include "RTWCG_FMU1ME_target.h"
#define FMU1_MESSAGE_SIZE 1024

/*
  will do nothing but return a error status
  Whenever a default function is called, it means a functions is called without successful load,
  return a fmiError;
*/
static fmiStatus defaultfcn1(fmiComponent c, ...){
    if(c != NULL){ return fmiFatal;}
    return fmiError;
}

static fmiBoolean CheckStatus(struct FMU1_ME_RTWCG * fmustruct, fmiStatus status, fmiString fcnName) {
    SimStruct* ss = fmustruct->ssPtr;
    if(status == fmiError || status == fmiFatal){
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMUSimFunctionErrorDebugToDisplayOn", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fcnName,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        fmustruct->FMUErrorStatus = status;
        rt_ssSet_slErrMsg(ss, diagnostic);
        ssSetStopRequested(ss, 1);
    }
    if(status == fmiOK)
        return fmiTrue;
    else
        return fmiFalse;
}
/*This function is required by FMI standard
  FMU loggin is currently not enabled, so it will not be called
  reportasInfo API is currently not avaiable for Rapid accelerator
*/

void fmu1MELogger(fmiComponent c, fmiString instanceName, fmiStatus status,
                fmiString category, fmiString message, ...) {
    (void) c, instanceName, status, category, message;
    /*
    int prefixLength;
    va_list args;
    void * diagnostic;
    
    static const char* strStatus[] = {
        "fmiOK", "fmiWarning", "fmiDiscard", "fmiError", "fmiFatal", "fmiPending" };
    static char translatedMsg[FMU1_MESSAGE_SIZE];
    static char temp[FMU1_MESSAGE_SIZE];
    
    prefixLength = snprintf(translatedMsg, FMU1_MESSAGE_SIZE, "Log from FMU: [category:%s, status:%s] ",
                            strStatus[status], category); 
    va_start (args, message);
    vsnprintf(temp, FMU1_MESSAGE_SIZE, message, args);
    va_end(args);
    
    strncat(translatedMsg, temp, FMU1_MESSAGE_SIZE-prefixLength - 1);
    diagnostic = CreateDiagnosticAsVoidPtr("SL_SERVICES:utils:PRINTFWRAPPER", 1,
                                           CODEGEN_SUPPORT_ARG_STRING_TYPE, translatedMsg);
                                           rt_ssReportDiagnosticAsWarning(NULL, diagnostic); 
    */
}

static _fmi_default_fcn_type LoadFMUFcn(struct FMU1_ME_RTWCG * fmustruct, const char * fcnName)
{
    _fmi_default_fcn_type fcn = NULL;

    static char fullFcnName[FULL_FCN_NAME_MAX_LEN];
    memset(fullFcnName, 0, FULL_FCN_NAME_MAX_LEN);
    strncpy(fullFcnName, fmustruct->fmuname, sizeof(fullFcnName));
    strncat(fullFcnName, "_", 1);
    strncat(fullFcnName, fcnName, sizeof(fullFcnName)-sizeof(fcnName)-1);
        
#ifdef _WIN32
    fcn = (_fmi_default_fcn_type)LOAD_FUNCTION(fmustruct->Handle, fullFcnName);
#else
    *((void **)(&fcn)) = LOAD_FUNCTION(fmustruct->Handle, fullFcnName);
#endif

    if (fcn == NULL) {
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMULoadLibFunctionError", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fcnName,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        fmustruct->FMUErrorStatus = fmiWarning;
        /*A loading failure will cause a warning, ANY CALL to defualt Fcn will result in an Error and Stop*/
        rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
        fcn = (_fmi_default_fcn_type) defaultfcn1; 
    }
    return fcn;
}

/* fmuPrefix is instanceName */
void* FMU1_fmuInitializeME( const char * lib, fmiString instanceName, fmiString fmuGUID, fmiString fmuLocation, void* ssPtr) {

    (void) fmuLocation;
    struct FMU1_ME_RTWCG * fmustruct;

    /*parameters to instantiateSlave()*/
    fmiCallbackFunctions callbacks;
    fmiReal timeout;           /* wait period in milli seconds, 0 for unlimited wait period"*/
    fmiBoolean visible;        /* no simulator user interface*/
    fmiBoolean interactive;    /* simulation run without user interaction*/

    fmustruct = (struct FMU1_ME_RTWCG *)calloc(1, sizeof(struct FMU1_ME_RTWCG));
    fmustruct->ssPtr = (SimStruct*) ssPtr;
    fmustruct->fmuname = (char *)instanceName;
    fmustruct->dllfile = (char *)lib;
    fmustruct->FMUErrorStatus = fmiOK;
    
    if (strlen(instanceName)+ FCN_NAME_MAX_LEN + 1 >= FULL_FCN_NAME_MAX_LEN){
        /*FMU name is longer than 200+, rarely happens*/
        void * diagnostic = CreateDiagnosticAsVoidPtr("SL_SERVICES:utils:PRINTFWRAPPER", 1,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, "FMU Name is too long.");
        fmustruct->FMUErrorStatus = fmiFatal;
        rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
        ssSetStopRequested(fmustruct->ssPtr, 1);
        return NULL;
    }

    fmustruct->Handle = LOAD_LIBRARY(lib);
    if (NULL == fmustruct->Handle) {
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMULoadLibraryError", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->dllfile,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        /*loading lib failure will cause an Fatal and Stop*/
        fmustruct->FMUErrorStatus = fmiFatal;
        rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
        ssSetStopRequested(fmustruct->ssPtr, 1);
        CLOSE_LIBRARY(fmustruct->Handle);
        return NULL;
    }

    LoadFMU1MEFunctions(fmustruct);
    
    /*instantiateModel()*/
    timeout = 0;            /* wait period in milli seconds, 0 for unlimited wait period"*/
    visible = fmiFalse;     /* no simulator user interface*/
    interactive = fmiFalse; /* simulation run without user interaction*/

    callbacks.logger = fmu1MELogger;
    callbacks.allocateMemory = calloc;
    callbacks.freeMemory = free;

    fmustruct->mFMIComp = fmustruct->instantiateModel(instanceName,
                                                      fmuGUID,
                                                      callbacks,
                                                      fmiFalse);                      /*Logging feature OFF*/

    
    if (NULL == fmustruct->mFMIComp ){
        CheckStatus(fmustruct, fmiError, "fmiInstantiateModel");
        return NULL;
    }
    return (void *) fmustruct;
}


fmiBoolean FMU1_terminateModel(void **fmuv){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    if(fmustruct->FMUErrorStatus != fmiFatal){
        fmiStatus fmiFlag = fmustruct->terminate(fmustruct->mFMIComp);
        CheckStatus(fmustruct, fmiFlag, "fmiTerminate");
        if(fmustruct->FMUErrorStatus != fmiError)
            fmustruct->freeModelInstance(fmustruct->mFMIComp);
    }
    CLOSE_LIBRARY(fmustruct->Handle);
    free(fmustruct);
    return fmiTrue;
}

void checkTerminateStatus(void **fmuv, const char* blkPath, fmiReal time){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    if(fmustruct->eventInfo.terminateSimulation == fmiTrue){
        /* terminate the simulation (successfully) */
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMUSimEventUpdateTerminated", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, blkPath,
                                                      CODEGEN_SUPPORT_ARG_REAL_TYPE, time);
        rt_ssReportDiagnosticAsInfo(fmustruct->ssPtr, diagnostic); /*todo: add test case*/
        ssSetStopRequested(fmustruct->ssPtr, 1);
    }
}

fmiStatus FMU1_initialize(void **fmuv, fmiReal isToleranceUsed, fmiReal toleranceValue){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmustruct->initialize(fmustruct->mFMIComp, (fmiBoolean) isToleranceUsed, toleranceValue, &(fmustruct->eventInfo));
    return CheckStatus(fmustruct, fmiFlag, "fmiInitialize");
}

fmiStatus FMU1_setTime(void **fmuv, fmiReal time){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmustruct->setTime(fmustruct->mFMIComp, time);
    return CheckStatus(fmustruct, fmiFlag, "fmisetTime");
}

fmiStatus FMU1_completedIntegratorStep(void **fmuv, int8_T* hasStepEvent){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmustruct->completedIntegratorStep(fmustruct->mFMIComp, (fmiBoolean *)hasStepEvent);
    return CheckStatus(fmustruct, fmiFlag, "completedIntegratorStep");
}

fmiStatus FMU1_setContinuousStates(void **fmuv, fmiReal states[], size_t nx){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmustruct->setContinuousStates(fmustruct->mFMIComp, states, nx);
    return CheckStatus(fmustruct, fmiFlag, "setContinuousStates");
}
    
fmiStatus FMU1_getContinuousStates(void **fmuv, fmiReal states[], size_t nx){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmustruct->getContinuousStates(fmustruct->mFMIComp, states, nx);
    return CheckStatus(fmustruct, fmiFlag, "getContinuousStates");
}

fmiStatus FMU1_getDerivatives(void **fmuv, fmiReal derivatives[], size_t nx){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmustruct->getDerivatives(fmustruct->mFMIComp, derivatives, nx);
    return CheckStatus(fmustruct, fmiFlag, "getDerivatives");
}

fmiStatus FMU1_getEventIndicators(void **fmuv, fmiReal eventIndicators[], size_t nx){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmustruct->getEventIndicators(fmustruct->mFMIComp, eventIndicators, nx);
    return CheckStatus(fmustruct, fmiFlag, "getEventIndicators");
}

/* event helper functions*/
fmiStatus FMU1_eventIteration(void **fmuv, const char* blkPath, fmiReal time){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmiOK;
    int iterationNumber = 0;
    while(fmustruct->eventInfo.iterationConverged == fmiFalse){
        /*safe call to eventUpdate*/
        fmiFlag = fmustruct->eventUpdate(fmustruct->mFMIComp, fmiFalse, &(fmustruct->eventInfo));
        CheckStatus(fmustruct, fmiFlag, "eventUpdate");

        if(fmustruct->eventInfo.terminateSimulation == fmiTrue){
            /* terminate the simulation (successfully) */
            void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMUSimEventUpdateTerminated", 2,
                                                          CODEGEN_SUPPORT_ARG_STRING_TYPE, blkPath,
                                                          CODEGEN_SUPPORT_ARG_REAL_TYPE, time);
            rt_ssReportDiagnosticAsInfo(fmustruct->ssPtr, diagnostic); /*todo: check report info API*/
            ssSetStopRequested(fmustruct->ssPtr, 1);
        }

        if(iterationNumber >= 10000){
            void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMUSimEventUpdateNotConverge", 2,
                                                          CODEGEN_SUPPORT_ARG_REAL_TYPE, time,
                                                          CODEGEN_SUPPORT_ARG_INTEGER_TYPE, iterationNumber);
            rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
            break;
            
        } else
            iterationNumber ++;
    }
    return fmiFlag;
}

void FMU1_ifStateVRchanged(void **fmuv, int8_T* fmustateValueOrStateValueRefChanged){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    *fmustateValueOrStateValueRefChanged = (int8_T) (fmustruct->eventInfo.stateValuesChanged || fmustruct->eventInfo.stateValueReferencesChanged);
}

/*Refer to FMU1MEBlock_exec.cpp processEventUpdate begining part*/
/* void FMU1_setIterationConverged(void **fmuv, int fmuIsInitialized, fmiReal time){ */
void FMU1_setIterationConverged(void **fmuv, int fmuIsInitialized, fmiReal time){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    bool callEventUpdate;
    if(fmuIsInitialized != 0){
        fmustruct->eventInfo.iterationConverged = fmiFalse;
    } else {
        callEventUpdate = (fmustruct->eventInfo.upcomingTimeEvent == fmiTrue &&
                           fmustruct->eventInfo.nextEventTime <= time);
        fmustruct->eventInfo.iterationConverged = (callEventUpdate) ? fmiFalse : fmiTrue;
    }
}

void FMU1_getNextEventTime(void **fmuv, fmiReal* nextEventTime, int8_T* upcomingTimeEvent){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    *nextEventTime = fmustruct->eventInfo.nextEventTime;
    *upcomingTimeEvent = (int8_T) fmustruct->eventInfo.upcomingTimeEvent;
}

/*get set function*/
fmiStatus FMU1ME_setRealVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiReal dvalue){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiReal value = dvalue;
    fmiStatus fmiFlag = fmustruct->setReal(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetReal");
}

fmiStatus FMU1ME_setReal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiReal value[]){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->setReal(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetReal");
}

fmiStatus FMU1ME_getReal(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiReal value[]){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->getReal(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetReal");
}

fmiStatus FMU1ME_setIntegerVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiInteger dvalue){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiInteger value = dvalue;
    fmiStatus fmiFlag = fmustruct->setInteger(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetInteger");
}


fmiStatus FMU1ME_setInteger(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiInteger value[]){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->setInteger(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetInteger");

}

fmiStatus FMU1ME_getInteger(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiInteger value[]){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->getInteger(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetInteger");
}

fmiStatus FMU1ME_setBooleanVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiBoolean dvalue){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiBoolean value = dvalue;
    fmiStatus fmiFlag = fmustruct->setBoolean(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetBoolean");
}

fmiStatus FMU1ME_setBoolean(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiBoolean value[]){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->setBoolean(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetBoolean");
}

fmiStatus FMU1ME_getBoolean(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiBoolean value[]){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->getBoolean(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetBoolean");
}

fmiStatus FMU1ME_setStringVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiString dvalue){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiString value = dvalue;
    fmiStatus fmiFlag = fmustruct->setString(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetString");
}

fmiStatus FMU1ME_setString(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiString value[]){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->setString(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetString");
}

fmiStatus FMU1ME_getString(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiString value[]){
    struct FMU1_ME_RTWCG * fmustruct = (struct FMU1_ME_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->getString(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetString");
}


void LoadFMU1MEFunctions(struct FMU1_ME_RTWCG * fmustruct){
    fmustruct->getModelTypesPlatform      = (_fmiGetModelTypesPlatform) LoadFMUFcn(fmustruct, "fmiGetModelTypesPlatform");
    fmustruct->getVersion                 = (_fmiGetVersion)         LoadFMUFcn(fmustruct, "fmiGetVersion");
    fmustruct->setDebugLogging            = (_fmiSetDebugLogging)    LoadFMUFcn(fmustruct, "fmiSetDebugLogging"); 
   
    fmustruct->instantiateModel           = (_fmiInstantiateModel)   LoadFMUFcn(fmustruct, "fmiInstantiateModel");
    fmustruct->freeModelInstance          = (_fmiFreeModelInstance)  LoadFMUFcn(fmustruct, "fmiFreeModelInstance");
    fmustruct->setTime                    = (_fmiSetTime)            LoadFMUFcn(fmustruct, "fmiSetTime");
    
    fmustruct->setContinuousStates        = (_fmiSetContinuousStates)      LoadFMUFcn(fmustruct, "fmiSetContinuousStates");
    fmustruct->completedIntegratorStep    = (_fmiCompletedIntegratorStep)  LoadFMUFcn(fmustruct, "fmiCompletedIntegratorStep");
    fmustruct->initialize                 = (_fmiInitialize)         LoadFMUFcn(fmustruct, "fmiInitialize");
    fmustruct->getDerivatives             = (_fmiGetDerivatives)     LoadFMUFcn(fmustruct, "fmiGetDerivatives");
    fmustruct->getEventIndicators         = (_fmiGetEventIndicators) LoadFMUFcn(fmustruct, "fmiGetEventIndicators");
    fmustruct->eventUpdate                = (_fmiEventUpdate)        LoadFMUFcn(fmustruct, "fmiEventUpdate");
    fmustruct->getContinuousStates        = (_fmiGetContinuousStates)         LoadFMUFcn(fmustruct, "fmiGetContinuousStates");
    fmustruct->getNominalContinuousStates = (_fmiGetNominalContinuousStates)  LoadFMUFcn(fmustruct, "fmiGetNominalContinuousStates");
    fmustruct->getStateValueReferences    = (_fmiGetStateValueReferences)     LoadFMUFcn(fmustruct, "fmiGetStateValueReferences");
    fmustruct->terminate                  = (_fmiTerminate)                   LoadFMUFcn(fmustruct, "fmiTerminate");
    
    fmustruct->setReal                    = (_fmiSetReal)                   LoadFMUFcn(fmustruct, "fmiSetReal");
    fmustruct->setInteger                 = (_fmiSetInteger)                LoadFMUFcn(fmustruct, "fmiSetInteger");
    fmustruct->setBoolean                 = (_fmiSetBoolean)                LoadFMUFcn(fmustruct, "fmiSetBoolean");
    fmustruct->setString                  = (_fmiSetString)                 LoadFMUFcn(fmustruct, "fmiSetString");
    fmustruct->getReal                    = (_fmiGetReal)                   LoadFMUFcn(fmustruct, "fmiGetReal");
    fmustruct->getInteger                 = (_fmiGetInteger)                LoadFMUFcn(fmustruct, "fmiGetInteger");
    fmustruct->getBoolean                 = (_fmiGetBoolean)                LoadFMUFcn(fmustruct, "fmiGetBoolean");
    fmustruct->getString                  = (_fmiGetString)                 LoadFMUFcn(fmustruct, "fmiGetString");
}
