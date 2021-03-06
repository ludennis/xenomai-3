// Copyright 2011 National Instruments
// License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
//   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef ___tDO_h___
#include "tDO.h"
#endif

tDO::tDO(tBusSpaceReference addrSpace, nMDBG::tStatus2* s)
 : DO_Timer()

{
   _addressOffset = 0;
   _addrSpace = addrSpace;

   _initialize(s);
}

tDO::tDO()
 : DO_Timer()

{
   _addressOffset = 0;

}

void tDO::initialize(tBusSpaceReference addrSpace, u32 addressOffset, nMDBG::tStatus2* s)
{

   _addrSpace = addrSpace;
   _addressOffset = addressOffset;

   _initialize(s);
}

void tDO::reset(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   DO_FIFO_St_Register.setRegister(u32(0x0), s);
   SCXI_DIO_Enable_Register.setRegister(u8(0x0), s);
   SCXI_DIO_Enable_Register.markDirty(s);
   Static_Digital_Output_Register.setRegister(u32(0x0), s);
   Static_Digital_Output_Register.markDirty(s);
   DIO_Direction_Register.setRegister(u32(0x0), s);
   DIO_Direction_Register.markDirty(s);
   DO_Mask_Enable_Register.setRegister(u32(0x0), s);
   DO_Mask_Enable_Register.markDirty(s);
   DO_Mode_Register.setRegister(0x80, s);
   DO_Mode_Register.markDirty(s);
   DO_Trigger_Select_Register.setRegister(u32(0x0), s);
   DO_Trigger_Select_Register.markDirty(s);
   DO_WDT_SafeStateRegister.setRegister(u32(0x0), s);
   DO_WDT_SafeStateRegister.markDirty(s);
   DO_WDT_ModeSelect1_Register.setRegister(u32(0x0), s);
   DO_WDT_ModeSelect1_Register.markDirty(s);
   DO_WDT_ModeSelect2_Register.setRegister(u32(0x0), s);
   DO_WDT_ModeSelect2_Register.markDirty(s);
}

void tDO::_initialize(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   DO_Timer.initialize(_addrSpace, 0x34 + getAddressOffset(s), s);



   //----------------------------------------
   // set register maps of all registers
   //----------------------------------------
   DO_FIFO_St_Register.setRegisterMap(this);
   SCXI_DIO_Enable_Register.setRegisterMap(this);
   Static_Digital_Output_Register.setRegisterMap(this);
   DIO_Direction_Register.setRegisterMap(this);
   CDO_FIFO_Data_Register.setRegisterMap(this);
   DO_Mask_Enable_Register.setRegisterMap(this);
   DO_Mode_Register.setRegisterMap(this);
   DO_Trigger_Select_Register.setRegisterMap(this);
   DO_DirectDataRegister.setRegisterMap(this);
   DO_WDT_SafeStateRegister.setRegisterMap(this);
   DO_WDT_ModeSelect1_Register.setRegisterMap(this);
   DO_WDT_ModeSelect2_Register.setRegisterMap(this);

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

tDO::~tDO()
{
}



// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

