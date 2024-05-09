LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
	LOCAL_MODULE := SAPlanter
else
	LOCAL_MODULE := SAPlanter64
endif
LOCAL_SRC_FILES := main.cpp mod/logger.cpp
LOCAL_CFLAGS += -Ofast -mfloat-abi=softfp -DNDEBUG -std=c++17 #-fvisibility=hidden
LOCAL_LDLIBS += -llog
include $(BUILD_SHARED_LIBRARY)