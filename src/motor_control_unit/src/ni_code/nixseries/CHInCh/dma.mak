#
# dma.mak
#
#   CHInCh DMA support for MHDDK
#

VPATH += \
   $(DMA_LIBRARY_DIR)

CXX_INCLUDES += \
   $(INCLUDE_FLAG)$(DMA_LIBRARY_DIR)/..

OBJECTS += \
   $(BUILD_DIR)/tCHInChDMAChannel$(OBJ_SUFFIX) \
   $(BUILD_DIR)/tCHInChDMAChannelController$(OBJ_SUFFIX) \
   $(BUILD_DIR)/tCHInChSGL$(OBJ_SUFFIX) \
   $(BUILD_DIR)/tCHInChSGLChunkyLink$(OBJ_SUFFIX) \
   $(BUILD_DIR)/tDMABuffer$(OBJ_SUFFIX) \
   $(BUILD_DIR)/tLinearDMABuffer$(OBJ_SUFFIX) \
   $(BUILD_DIR)/tScatterGatherDMABuffer$(OBJ_SUFFIX) \
