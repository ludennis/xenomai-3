// Copyright 2011 National Instruments
// License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
//   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef ___tAI_h___
#include "tAI.h"
#endif

tAI::tAI(tBusSpaceReference addrSpace, nMDBG::tStatus2* s)
 : AI_Timer()

{
   _addressOffset = 0;
   _addrSpace = addrSpace;

   _initialize(s);
}

tAI::tAI()
 : AI_Timer()

{
   _addressOffset = 0;

}

void tAI::initialize(tBusSpaceReference addrSpace, u32 addressOffset, nMDBG::tStatus2* s)
{

   _addrSpace = addrSpace;
   _addressOffset = addressOffset;

   _initialize(s);
}

void tAI::reset(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   AI_Config_FIFO_Status_Register.setRegister(u32(0x0), s);
   AI_Data_FIFO_Status_Register.setRegister(u32(0x0), s);
   AI_Config_FIFO_Data_Register.setRegister(u16(0x0), s);
   AI_Config_FIFO_Data_Register.markDirty(s);
   AI_Data_Mode_Register.setRegister(u32(0x0), s);
   AI_Data_Mode_Register.markDirty(s);
   AI_Trigger_Select_Register.setRegister(u32(0x0), s);
   AI_Trigger_Select_Register.markDirty(s);
   AI_Trigger_Select_Register2.setRegister(u32(0x0), s);
   AI_Trigger_Select_Register2.markDirty(s);
}

void tAI::_initialize(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   AI_Timer.initialize(_addrSpace, 0x40 + getAddressOffset(s), s);



   //----------------------------------------
   // set register maps of all registers
   //----------------------------------------
   AI_Config_FIFO_Status_Register.setRegisterMap(this);
   AI_Data_FIFO_Status_Register.setRegisterMap(this);
   AI_FIFO_Data_Register.setRegisterMap(this);
   AI_FIFO_Data_Register16.setRegisterMap(this);
   AI_Config_FIFO_Data_Register.setRegisterMap(this);
   AI_Data_Mode_Register.setRegisterMap(this);
   AI_Trigger_Select_Register.setRegisterMap(this);
   AI_Trigger_Select_Register2.setRegisterMap(this);

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

tAI::~tAI()
{
}



// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

