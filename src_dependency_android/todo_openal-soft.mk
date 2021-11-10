LOCAL_PATH := $(SRC_PATH)/src_dependency/openal-mob
include $(CLEAR_VARS)
LOCAL_MODULE := openal-soft

LOCAL_CFLAGS := -DAL_LIBTYPE_STATIC -DALSOFT_REQUIRE_OPENSL -std=c++17 -DRESTRICT=__restrict -DNOMINMAX -DAL_ALEXT_PROTOTYPES

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_C_INCLUDES := \
	$(LOCAL_EXPORT_C_INCLUDES)		\
	$(LOCAL_PATH)/include/AL		\
	$(LOCAL_PATH)/al				\
	$(LOCAL_PATH)/alc				\
	$(LOCAL_PATH)/common			\
	$(LOCAL_PATH)/core				\
	$(LOCAL_PATH)/build-android

LOCAL_EXPORT_LDLIBS := -lOpenSLES -llog

LOCAL_SRC_FILES := \
    common/alcomplex.cpp\
    common/alfstream.cpp\
    common/almalloc.cpp\
    common/alstring.cpp\
    common/dynload.cpp\
    common/polyphase_resampler.cpp\
    common/ringbuffer.cpp\
    common/strutils.cpp\
    common/threads.cpp\
	core/ambdec.cpp\
    core/bs2b.cpp\
    core/bsinc_tables.cpp\
    core/cpu_caps.cpp\
    core/devformat.cpp\
    core/except.cpp\
    core/filters/biquad.cpp\
    core/filters/nfc.cpp\
    core/filters/splitter.cpp\
    core/fmt_traits.cpp\
    core/fpu_ctrl.cpp\
    core/logging.cpp\
    core/mastering.cpp\
    core/uhjfilter.cpp\
    core/mixer/mixer_c.cpp\
	core/mixer/mixer_neon.cpp\
	al/auxeffectslot.cpp\
    al/buffer.cpp\
    al/effect.cpp\
    al/effects/autowah.cpp\
    al/effects/chorus.cpp\
    al/effects/compressor.cpp\
    al/effects/convolution.cpp\
    al/effects/dedicated.cpp\
    al/effects/distortion.cpp\
    al/effects/echo.cpp\
    al/effects/equalizer.cpp\
    al/effects/fshifter.cpp\
    al/effects/modulator.cpp\
    al/effects/null.cpp\
    al/effects/pshifter.cpp\
    al/effects/reverb.cpp\
    al/effects/vmorpher.cpp\
    al/error.cpp\
    al/event.cpp\
    al/extension.cpp\
    al/filter.cpp\
    al/listener.cpp\
    al/source.cpp\
    al/state.cpp\
	alc/backends/opensl.cpp\
	alc/backends/base.cpp\
    alc/backends/loopback.cpp\
    alc/backends/null.cpp\
    alc/alc.cpp\
    alc/alu.cpp\
    alc/alconfig.cpp\
    alc/bformatdec.cpp\
    alc/buffer_storage.cpp\
    alc/converter.cpp\
    alc/effectslot.cpp\
    alc/effects/autowah.cpp\
    alc/effects/chorus.cpp\
    alc/effects/compressor.cpp\
    alc/effects/convolution.cpp\
    alc/effects/dedicated.cpp\
    alc/effects/distortion.cpp\
    alc/effects/echo.cpp\
    alc/effects/equalizer.cpp\
    alc/effects/fshifter.cpp\
    alc/effects/modulator.cpp\
    alc/effects/null.cpp\
    alc/effects/pshifter.cpp\
    alc/effects/reverb.cpp\
    alc/effects/vmorpher.cpp\
    alc/front_stablizer.h\
    alc/helpers.cpp\
    alc/hrtf.cpp\
    alc/panning.cpp\
    alc/uiddefs.cpp\
    alc/voice.cpp\

include $(BUILD_STATIC_LIBRARY)
