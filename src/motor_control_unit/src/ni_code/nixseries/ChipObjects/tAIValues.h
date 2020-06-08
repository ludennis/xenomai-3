// Copyright 2011 National Instruments
// License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
//   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef ___tAIValues_h___
#define ___tAIValues_h___

#include "tInTimerValues.h"

namespace nAI {
   using namespace nInTimer;
   typedef enum {
      kBank0                             = 0,
      kBank1                             = 1,
   } tAI_Config_Bank_t;

   typedef enum {
      kLoopback                          = 0,
      kDifferential                      = 1,
      kNRSE                              = 2,
      kRSE                               = 3,
      kInternal                          = 5,
   } tAI_Config_Channel_Type_t;

   typedef enum {
      kDisabled                          = 0,
      kEnabled                           = 1,
   } tAI_Disabled_Enabled_t;

   typedef enum {
      kGate_Disabled                     = 0,
      kGate_PFI0                         = 1,
      kGate_PFI1                         = 2,
      kGate_PFI2                         = 3,
      kGate_PFI3                         = 4,
      kGate_PFI4                         = 5,
      kGate_PFI5                         = 6,
      kGate_PFI6                         = 7,
      kGate_PFI7                         = 8,
      kGate_PFI8                         = 9,
      kGate_PFI9                         = 10,
      kGate_RTSI0                        = 11,
      kGate_RTSI1                        = 12,
      kGate_RTSI2                        = 13,
      kGate_RTSI3                        = 14,
      kGate_RTSI4                        = 15,
      kGate_RTSI5                        = 16,
      kGate_RTSI6                        = 17,
      kGate_PXIe_DStarA                  = 18,
      kGate_PXIe_DStarB                  = 19,
      kGate_Star_Trigger                 = 20,
      kGate_PFI10                        = 21,
      kGate_PFI11                        = 22,
      kGate_PFI12                        = 23,
      kGate_PFI13                        = 24,
      kGate_PFI14                        = 25,
      kGate_PFI15                        = 26,
      kGate_RTSI7                        = 27,
      kGate_Analog_Trigger               = 30,
      kGate_Low                          = 31,
      kGate_G0_Out                       = 32,
      kGate_G1_Out                       = 33,
      kGate_G2_Out                       = 34,
      kGate_G3_Out                       = 35,
      kGate_G0_Gate                      = 36,
      kGate_G1_Gate                      = 37,
      kGate_G2_Gate                      = 38,
      kGate_G3_Gate                      = 39,
      kGate_DI_Gate                      = 40,
      kGate_AO_Gate                      = 43,
      kGate_DO_Gate                      = 44,
      kGate_IntTriggerA0                 = 53,
      kGate_IntTriggerA1                 = 54,
      kGate_IntTriggerA2                 = 55,
      kGate_IntTriggerA3                 = 56,
      kGate_IntTriggerA4                 = 57,
      kGate_IntTriggerA5                 = 58,
      kGate_IntTriggerA6                 = 59,
      kGate_IntTriggerA7                 = 60,
   } tAI_External_Gate_Select_t;

   typedef enum {
      kTwoByteFifo                       = 0,
      kFourByteFifo                      = 1,
   } tAI_FifoWidth_t;

   typedef enum {
      kActive_High_Or_Rising_Edge        = 0,
      kActive_Low_Or_Falling_Edge        = 1,
   } tAI_Polarity_t;

   typedef enum {
      kStart1_SW_Pulse                   = 0,
      kStart1_PFI0                       = 1,
      kStart1_PFI1                       = 2,
      kStart1_PFI2                       = 3,
      kStart1_PFI3                       = 4,
      kStart1_PFI4                       = 5,
      kStart1_PFI5                       = 6,
      kStart1_PFI6                       = 7,
      kStart1_PFI7                       = 8,
      kStart1_PFI8                       = 9,
      kStart1_PFI9                       = 10,
      kStart1_RTSI0                      = 11,
      kStart1_RTSI1                      = 12,
      kStart1_RTSI2                      = 13,
      kStart1_RTSI3                      = 14,
      kStart1_RTSI4                      = 15,
      kStart1_RTSI5                      = 16,
      kStart1_RTSI6                      = 17,
      kStart1_PXIe_DStarA                = 18,
      kStart1_PXIe_DStarB                = 19,
      kStart1_Star_Trigger               = 20,
      kStart1_PFI10                      = 21,
      kStart1_PFI11                      = 22,
      kStart1_PFI12                      = 23,
      kStart1_PFI13                      = 24,
      kStart1_PFI14                      = 25,
      kStart1_PFI15                      = 26,
      kStart1_RTSI7                      = 27,
      kStart1_DIO_ChgDetect              = 28,
      kStart1_Analog_Trigger             = 30,
      kStart1_Low                        = 31,
      kStart1_G0_Out                     = 36,
      kStart1_G1_Out                     = 37,
      kStart1_G2_Out                     = 38,
      kStart1_G3_Out                     = 39,
      kStart1_DI_Start1                  = 40,
      kStart1_AO_Start1                  = 43,
      kStart1_DO_Start1                  = 44,
      kStart1_IntTriggerA0               = 53,
      kStart1_IntTriggerA1               = 54,
      kStart1_IntTriggerA2               = 55,
      kStart1_IntTriggerA3               = 56,
      kStart1_IntTriggerA4               = 57,
      kStart1_IntTriggerA5               = 58,
      kStart1_IntTriggerA6               = 59,
      kStart1_IntTriggerA7               = 60,
   } tAI_START1_Select_t;

   typedef enum {
      kStart2_SW_Pulse                   = 0,
      kStart2_PFI0                       = 1,
      kStart2_PFI1                       = 2,
      kStart2_PFI2                       = 3,
      kStart2_PFI3                       = 4,
      kStart2_PFI4                       = 5,
      kStart2_PFI5                       = 6,
      kStart2_PFI6                       = 7,
      kStart2_PFI7                       = 8,
      kStart2_PFI8                       = 9,
      kStart2_PFI9                       = 10,
      kStart2_RTSI0                      = 11,
      kStart2_RTSI1                      = 12,
      kStart2_RTSI2                      = 13,
      kStart2_RTSI3                      = 14,
      kStart2_RTSI4                      = 15,
      kStart2_RTSI5                      = 16,
      kStart2_RTSI6                      = 17,
      kStart2_PXIe_DStarA                = 18,
      kStart2_PXIe_DStarB                = 19,
      kStart2_Star_Trigger               = 20,
      kStart2_PFI10                      = 21,
      kStart2_PFI11                      = 22,
      kStart2_PFI12                      = 23,
      kStart2_PFI13                      = 24,
      kStart2_PFI14                      = 25,
      kStart2_PFI15                      = 26,
      kStart2_RTSI7                      = 27,
      kStart2_DIO_ChgDetect              = 28,
      kStart2_Analog_Trigger             = 30,
      kStart2_Low                        = 31,
      kStart2_G0_Out                     = 36,
      kStart2_G1_Out                     = 37,
      kStart2_G2_Out                     = 38,
      kStart2_G3_Out                     = 39,
      kStart2_DI_Start2                  = 40,
      kStart2_AO_Start1                  = 43,
      kStart2_DO_Start1                  = 44,
      kStart2_IntTriggerA0               = 53,
      kStart2_IntTriggerA1               = 54,
      kStart2_IntTriggerA2               = 55,
      kStart2_IntTriggerA3               = 56,
      kStart2_IntTriggerA4               = 57,
      kStart2_IntTriggerA5               = 58,
      kStart2_IntTriggerA6               = 59,
      kStart2_IntTriggerA7               = 60,
   } tAI_START2_Select_t;

   typedef enum {
      kStartCnv_InternalTiming           = 0,
      kStartCnv_PFI0                     = 1,
      kStartCnv_PFI1                     = 2,
      kStartCnv_PFI2                     = 3,
      kStartCnv_PFI3                     = 4,
      kStartCnv_PFI4                     = 5,
      kStartCnv_PFI5                     = 6,
      kStartCnv_PFI6                     = 7,
      kStartCnv_PFI7                     = 8,
      kStartCnv_PFI8                     = 9,
      kStartCnv_PFI9                     = 10,
      kStartCnv_RTSI0                    = 11,
      kStartCnv_RTSI1                    = 12,
      kStartCnv_RTSI2                    = 13,
      kStartCnv_RTSI3                    = 14,
      kStartCnv_RTSI4                    = 15,
      kStartCnv_RTSI5                    = 16,
      kStartCnv_RTSI6                    = 17,
      kStartCnv_DIO_ChgDetect            = 18,
      kStartCnv_G0_Out                   = 19,
      kStartCnv_Star_Trigger             = 20,
      kStartCnv_PFI10                    = 21,
      kStartCnv_PFI11                    = 22,
      kStartCnv_PFI12                    = 23,
      kStartCnv_PFI13                    = 24,
      kStartCnv_PFI14                    = 25,
      kStartCnv_PFI15                    = 26,
      kStartCnv_RTSI7                    = 27,
      kStartCnv_G1_Out                   = 28,
      kStartCnv_SCXI_Trig1               = 29,
      kStartCnv_Atrig                    = 30,
      kStartCnv_Low                      = 31,
      kStartCnv_PXIe_DStarA              = 32,
      kStartCnv_PXIe_DStarB              = 33,
      kStartCnv_G2_Out                   = 34,
      kStartCnv_G3_Out                   = 35,
      kStartCnv_G0_SampleClk             = 36,
      kStartCnv_G1_SampleClk             = 37,
      kStartCnv_G2_SampleClk             = 38,
      kStartCnv_G3_SampleClk             = 39,
      kStartCnv_DI_Convert               = 40,
      kStartCnv_AO_Update                = 43,
      kStartCnv_DO_Update                = 44,
      kStartCnv_IntTriggerA0             = 53,
      kStartCnv_IntTriggerA1             = 54,
      kStartCnv_IntTriggerA2             = 55,
      kStartCnv_IntTriggerA3             = 56,
      kStartCnv_IntTriggerA4             = 57,
      kStartCnv_IntTriggerA5             = 58,
      kStartCnv_IntTriggerA6             = 59,
      kStartCnv_IntTriggerA7             = 60,
   } tAI_StartConvertSelMux_t;

   namespace nAI_Config_FIFO_Status_Register {
      namespace nAI_Config_FIFO_Status {
         enum {
            kMask = 0xffffffff,
            kOffset = 0,
         };
      }

   }

   namespace nAI_Data_FIFO_Status_Register {
      namespace nAI_Data_FIFO_Status {
         enum {
            kMask = 0xffffffff,
            kOffset = 0,
         };
      }

   }

   namespace nAI_FIFO_Data_Register {
      namespace nAI_FIFO_Data {
         enum {
            kMask = 0xffffffff,
            kOffset = 0,
         };
      }

   }

   namespace nAI_FIFO_Data_Register16 {
      namespace nAI_FIFO_Data16 {
         enum {
            kMask = 0xffff,
            kOffset = 0,
         };
      }

   }

   namespace nAI_Config_FIFO_Data_Register {
      namespace nAI_Config_Channel {
         enum {
            kMask = 0xf,
            kOffset = 0,
         };
      }

      namespace nAI_Config_Bank {
         enum {
            kMask = 0x30,
            kOffset = 0x4,
         };
      }

      namespace nAI_Config_Channel_Type {
         enum {
            kMask = 0x1c0,
            kOffset = 0x6,
         };
      }

      namespace nAI_Config_Gain {
         enum {
            kMask = 0xe00,
            kOffset = 0x9,
         };
      }

      namespace nAI_Config_Dither {
         enum {
            kMask = 0x2000,
            kOffset = 0xd,
         };
      }

      namespace nAI_Config_Last_Channel {
         enum {
            kMask = 0x4000,
            kOffset = 0xe,
         };
      }

   }

   namespace nAI_Data_Mode_Register {
      namespace nAI_DoneNotificationEnable {
         enum {
            kMask = 0x80,
            kOffset = 0x7,
         };
      }

      namespace nAI_FifoWidth {
         enum {
            kMask = 0x100,
            kOffset = 0x8,
         };
      }

   }

   namespace nAI_Trigger_Select_Register {
      namespace nAI_START1_Select {
         enum {
            kMask = 0x3f,
            kOffset = 0,
         };
      }

      namespace nAI_START1_Edge {
         enum {
            kMask = 0x40,
            kOffset = 0x6,
         };
      }

      namespace nAI_START1_Polarity {
         enum {
            kMask = 0x80,
            kOffset = 0x7,
         };
      }

      namespace nAI_START2_Select {
         enum {
            kMask = 0x3f00,
            kOffset = 0x8,
         };
      }

      namespace nAI_START2_Edge {
         enum {
            kMask = 0x4000,
            kOffset = 0xe,
         };
      }

      namespace nAI_START2_Polarity {
         enum {
            kMask = 0x8000,
            kOffset = 0xf,
         };
      }

      namespace nAI_External_Gate_Select {
         enum {
            kMask = 0x3f0000,
            kOffset = 0x10,
         };
      }

      namespace nAI_External_Gate_Polarity {
         enum {
            kMask = 0x800000,
            kOffset = 0x17,
         };
      }

      namespace nAI_CONVERT_Source_Select {
         enum {
            kMask = 0x3f000000,
            kOffset = 0x18,
         };
      }

      namespace nAI_Convert_Source_Polarity {
         enum {
            kMask = 0x80000000,
            kOffset = 0x1f,
         };
      }

   }

   namespace nAI_Trigger_Select_Register2 {
      namespace nAI_START_Select {
         enum {
            kMask = 0x3f0000,
            kOffset = 0x10,
         };
      }

      namespace nAI_START_Edge {
         enum {
            kMask = 0x400000,
            kOffset = 0x16,
         };
      }

      namespace nAI_START_Polarity {
         enum {
            kMask = 0x800000,
            kOffset = 0x17,
         };
      }

   }

   using namespace nAI_Config_FIFO_Status_Register;
   using namespace nAI_Data_FIFO_Status_Register;
   using namespace nAI_FIFO_Data_Register;
   using namespace nAI_FIFO_Data_Register16;
   using namespace nAI_Config_FIFO_Data_Register;
   using namespace nAI_Data_Mode_Register;
   using namespace nAI_Trigger_Select_Register;
   using namespace nAI_Trigger_Select_Register2;
}

#endif


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// This file is autogenerated!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

