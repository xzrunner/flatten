INNER_SAVED_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := flatten

LOCAL_C_INCLUDES  := \
	${CLIB_PATH} \
	${FLATTEN_SRC_PATH}/include \
	${SPRITE2_SRC_PATH}/include \
	${COOKING_SRC_PATH}/include \
	${LOGGER_SRC_PATH} \
	${CU_SRC_PATH} \
	${SM_SRC_PATH} \
	${DS_SRC_PATH} \
	${MEMMGR_SRC_PATH}/include \
	${UNIRENDER_SRC_PATH}/include \
	${SHADERLAB_SRC_PATH}/include \

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,,$(shell find $(LOCAL_PATH) -name "*.cpp" -print)) \

include $(BUILD_STATIC_LIBRARY)	

LOCAL_PATH := $(INNER_SAVED_LOCAL_PATH)