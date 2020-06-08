// Copyright 2011 National Instruments
// License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
//   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef ___tAO_h___
#include "tAO.h"
#endif

tAO::tAO(tBusSpaceReference addrSpace, nMDBG::tStatus2* s)
 : AO_Timer()

{
   _addressOffset = 0;
   _addrSpace = addrSpace;

   _initialize(s);
}

tAO::tAO()
 : AO_Timer()

{
   _addressOffset = 0;

}

void tAO::initialize(tBusSpaceReference addrSpace, u32 addressOffset, nMDBG::tStatus2* s)
{

   _addrSpace = addrSpace;
   _addressOffset = addressOffset;

   _initialize(s);
}

void tAO::reset(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   unsigned int i;
   for(i = 0; i < 8; ++i) {
      AO_DacShadow[i].setRegister(u32(0x0), s);
   }
   AO_Trigger_Select_Register.setRegister(u32(0x0), s);
   AO_Trigger_Select_Register.markDirty(s);
   for(i = 0; i < 8; ++i) {
      AO_Config_Bank[i].setRegister(0xbf, s);
      AO_Config_Bank[i].markDirty(s);
   }
   AO_FIFO_Status_Register.setRegister(u32(0x0), s);
}

void tAO::_initialize(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   AO_Timer.initialize(_addrSpace, 0x70 + getAddressOffset(s), s);

   {
      unsigned int i;
      for (i=0; i<8; ++i) {
         AO_DacShadow[i].initialize(0x0 + (0x4 * i), 0x0 + i);
      }
      for (i=0; i<16; ++i) {
         AO_Direct_Data[i].initialize(0x0 + (0x4 * i), 0x8 + i);
      }
      for (i=0; i<8; ++i) {
         AO_Config_Bank[i].initialize(0x4c + (0x1 * i), 0x1b + i);
      }
   }


   //----------------------------------------
   // set register maps of all registers
   //----------------------------------------
   {
      unsigned int i;
      for(i = 0; i < 8; ++i) {
         AO_DacShadow[i].setRegisterMap(this);
      }
   }
   {
      unsigned int i;
      for(i = 0; i < 16; ++i) {
         AO_Direct_Data[i].setRegisterMap(this);
      }
   }
   AO_Order_Config_Data_Register.setRegisterMap(this);
   AO_Config_Control_Register.setRegisterMap(this);
   AO_Trigger_Select_Register.setRegisterMap(this);
   {
      unsigned int i;
      for(i = 0; i < 8; ++i) {
         AO_Config_Bank[i].setRegisterMap(this);
      }
   }
   AO_FIFO_Data_Register.setRegisterMap(this);
   AO_FIFO_Status_Register.setRegisterMap(this);

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

tAO::~tAO()
{
}



// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
