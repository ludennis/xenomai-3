// Copyright 2011 National Instruments
// License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
//   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef ___tSimultaneousControl_h___
#include "tSimultaneousControl.h"
#endif

tSimultaneousControl::tSimultaneousControl(tBusSpaceReference addrSpace, nMDBG::tStatus2* s)

{
   _addressOffset = 0;
   _addrSpace = addrSpace;

   _initialize(s);
}

tSimultaneousControl::tSimultaneousControl()

{
   _addressOffset = 0;

}

void tSimultaneousControl::initialize(tBusSpaceReference addrSpace, u32 addressOffset, nMDBG::tStatus2* s)
{

   _addrSpace = addrSpace;
   _addressOffset = addressOffset;

   _initialize(s);
}

void tSimultaneousControl::reset(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   AITriggerConfigCtrlStat.setRegister(u8(0x0), s);
   AITriggerConfigCtrlStat.markDirty(s);
   LoopbackSourceSel.setRegister(u8(0x0), s);
   LoopbackSourceSel.markDirty(s);
   SignatureYear.setRegister(u8(0x0), s);
   SignatureMonth.setRegister(u8(0x0), s);
   SignatureDay.setRegister(u8(0x0), s);
   SignatureHour.setRegister(u8(0x0), s);
   Scratch.setRegister(u8(0x0), s);
   Scratch.markDirty(s);
   TempSensorDataHi.setRegister(u8(0x0), s);
   TempSensorDataLo.setRegister(u8(0x0), s);
}

void tSimultaneousControl::_initialize(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;



   //----------------------------------------
   // set register maps of all registers
   //----------------------------------------
   AISetChannelOrder.setRegisterMap(this);
   AIChanConfigCtrlStat.setRegisterMap(this);
   AIClearChannelOrder.setRegisterMap(this);
   AITriggerConfigCtrlStat.setRegisterMap(this);
   AcquisitionCtrl.setRegisterMap(this);
   AiFifoCtrlStat.setRegisterMap(this);
   LoopbackCtrlStat.setRegisterMap(this);
   LoopbackSourceSel.setRegisterMap(this);
   DcmCtrlStat.setRegisterMap(this);
   InterruptControl.setRegisterMap(this);
   InterruptStatus.setRegisterMap(this);
   SignatureYear.setRegisterMap(this);
   SignatureMonth.setRegisterMap(this);
   SignatureDay.setRegisterMap(this);
   SignatureHour.setRegisterMap(this);
   Scratch.setRegisterMap(this);
   TempSensorCtrlStat.setRegisterMap(this);
   TempSensorDataHi.setRegisterMap(this);
   TempSensorDataLo.setRegisterMap(this);

   //----------------------------------------
   // initialize dirty flags
   //----------------------------------------
   for (unsigned int i = 0; i < sizeof(_dirtyVector)/sizeof(_dirtyVector[0]); i++) {
      _dirtyVector[i] = 0;
   }

   //----------------------------------------
   // reset registers
   //----------------------------------------
   reset(s);
}

tSimultaneousControl::~tSimultaneousControl()
{
}



// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

