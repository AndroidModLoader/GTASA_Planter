LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc
LOCAL_MODULE    := SAPlanter
LOCAL_SRC_FILES := main.cpp mod/logger.cpp
LOCAL_CFLAGS += -Ofast -mfloat-abi=softfp -DNDEBUG -std=c++17 #-fvisibility=hidden
LOCAL_C_INCLUDES += ./include
LOCAL_LDLIBS += -llog
include $(BUILD_SHARED_LIBRARY)