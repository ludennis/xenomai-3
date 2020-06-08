// Copyright 2011 National Instruments
// License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
//   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef ___tCHInCh_h___
#include "tCHInCh.h"
#endif

tCHInCh::tCHInCh(tBusSpaceReference addrSpace, nMDBG::tStatus2* s)
 : AI_DMAChannel()
,
   Counter0DmaChannel()
,
   Counter1DmaChannel()
,
   Counter2DmaChannel()
,
   Counter3DmaChannel()
,
   DI_DMAChannel()
,
   AO_DMAChannel()
,
   DO_DMAChannel()

{
   _addressOffset = 0;
   _addrSpace = addrSpace;

   _initialize(s);
}

tCHInCh::tCHInCh()
 : AI_DMAChannel()
,
   Counter0DmaChannel()
,
   Counter1DmaChannel()
,
   Counter2DmaChannel()
,
   Counter3DmaChannel()
,
   DI_DMAChannel()
,
   AO_DMAChannel()
,
   DO_DMAChannel()

{
   _addressOffset = 0;

}

void tCHInCh::initialize(tBusSpaceReference addrSpace, u32 addressOffset, nMDBG::tStatus2* s)
{

   _addrSpace = addrSpace;
   _addressOffset = addressOffset;

   _initialize(s);
}

void tCHInCh::reset(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   CHInCh_Identification_Register.setRegister(u32(0x0), s);
   IO_Port_Resource_Description_Register.setRegister(u32(0x0), s);
   Interrupt_Mask_Register.setRegister(u32(0x0), s);
   Interrupt_Mask_Register.markDirty(s);
   Host_Bus_Resource_Control_Register.setRegister(u32(0x0), s);
   Host_Bus_Resource_Control_Register.markDirty(s);
   unsigned int i;
   for(i = 0; i < 1; ++i) {
      Scrap_Register[i].setRegister(u32(0x0), s);
      Scrap_Register[i].markDirty(s);
   }
   PCI_SubSystem_ID_Access_Register.setRegister(u32(0x0), s);
}

void tCHInCh::_initialize(nMDBG::tStatus2* s)
{
   if (s && s->isFatal()) return;

   AI_DMAChannel.initialize(_addrSpace, 0x2000 + getAddressOffset(s), s);
   Counter0DmaChannel.initialize(_addrSpace, 0x2100 + getAddressOffset(s), s);
   Counter1DmaChannel.initialize(_addrSpace, 0x2200 + getAddressOffset(s), s);
   Counter2DmaChannel.initialize(_addrSpace, 0x2300 + getAddressOffset(s), s);
   Counter3DmaChannel.initialize(_addrSpace, 0x2400 + getAddressOffset(s), s);
   DI_DMAChannel.initialize(_addrSpace, 0x2500 + getAddressOffset(s), s);
   AO_DMAChannel.initialize(_addrSpace, 0x2600 + getAddressOffset(s), s);
   DO_DMAChannel.initialize(_addrSpace, 0x2700 + getAddressOffset(s), s);

   {
      unsigned int i;
      for (i=0; i<1; ++i) {
         Scrap_Register[i].initialize(0x200 + (0x4 * i), 0x9 + i);
      }
   }


   //----------------------------------------
   // set register maps of all registers
   //----------------------------------------
   CHInCh_Identification_Register.setRegisterMap(this);
   IO_Port_Resource_Description_Register.setRegisterMap(this);
   Interrupt_Mask_Register.setRegisterMap(this);
   Interrupt_Status_Register.setRegisterMap(this);
   Volatile_Interrupt_Status_Register.setRegisterMap(this);
   Host_Bus_Resource_Control_Register.setRegisterMap(this);
   EEPROM_Window_Register.setRegisterMap(this);
   Simultaneous_Window_Register.setRegisterMap(this);
   Window_Control_Register.setRegisterMap(this);
   {
      unsigned int i;
      for(i = 0; i < 1; ++i) {
         Scrap_Register[i].setRegisterMap(this);
      }
   }
   Configuration_Register.setRegisterMap(this);
   EEPROM_Register_0.setRegisterMap(this);
   EEPROM_Register_1.setRegisterMap(this);
   EEPROM_Register_2.setRegisterMap(this);
   SMIO_Register_0.setRegisterMap(this);
   SMIO_Register_1.setRegisterMap(this);
   SMIO_Register_2.setRegisterMap(this);
   SMIO_Register_3.setRegisterMap(this);
   PCI_SubSystem_ID_Access_Register.setRegisterMap(this);

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

tCHInCh::~tCHInCh()
{
}



// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

