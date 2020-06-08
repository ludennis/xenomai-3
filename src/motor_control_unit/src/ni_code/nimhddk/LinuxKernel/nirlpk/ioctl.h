/*
 * ioctl.h
 *   IO control codes for nirlpk
 *
 * Copyright (c) 2012 National Instruments. All rights reserved.
 * License: NATIONAL INSTRUMENTS SOFTWARE LICENSE AGREEMENT
 *   Refer to "MHDDK License Agreement.pdf" in the root of this distribution.
 *
 */

#ifndef ___nirlpk_ioctl_h___
#define ___nirlpk_ioctl_h___

#include <linux/ioctl.h>

#define NIRLP_IOCTL_MAGIC   0xbb

#define NIRLP_IOCTL_GET_PHYSICAL_ADDRESS _IOWR(NIRLP_IOCTL_MAGIC, 1, unsigned long)
#define NIRLP_IOCTL_ALLOCATE_DMA_BUFFER  _IOWR(NIRLP_IOCTL_MAGIC, 2, unsigned long)
#define NIRLP_IOCTL_FREE_DMA_BUFFER      _IOW(NIRLP_IOCTL_MAGIC,  3, unsigned long)
#define NIRLP_IOCTL_GET_BAR_SIZE         _IOWR(NIRLP_IOCTL_MAGIC, 4, unsigned long)

#define NIRLP_IOCTL_MAX     4

#endif // ___nirlpk_ioctl_h___
