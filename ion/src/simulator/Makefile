ion_src += $(addprefix ion/src/shared/dummy/, \
  authentication.cpp \
  backlight.cpp \
  battery.cpp \
  display.cpp \
  external_apps.cpp \
  fcc_id.cpp \
  led.cpp \
  platform_info.cpp \
  post_and_hardware_tests.cpp \
  power.cpp \
  reset.h \
  stack.cpp \
  usb.cpp \
)

ion_src += $(addprefix ion/src/simulator/shared/, \
  clipboard.cpp \
  clipboard_helper.cpp \
  console.cpp \
  compilation_flags.cpp \
  crc32.cpp \
  device_name.cpp \
  display.cpp \
  events.cpp \
  events_platform.cpp \
  exam_bytes.cpp \
  framebuffer.cpp \
  init.cpp \
  keyboard.cpp \
  main.cpp \
  random.cpp \
  timing.cpp \
  window.cpp \
)

$(call object_for,ion/src/simulator/shared/compilation_flags.cpp): SFLAGS += -DEXTERNAL_APPS_API_LEVEL=$(EXTERNAL_APPS_API_LEVEL)

ion_simulator_assets_paths = $(addprefix ion/src/simulator/assets/,background-with-shadow.webp horizontal_arrow.png vertical_arrow.png round.png small_squircle.png large_squircle.png)
$(addprefix $(BUILD_DIR)/,$(addprefix ion/src/simulator/assets/,background.jpg background-no-shadow.webp)): ion/src/simulator/assets/background-with-shadow.webp
	$(call rule_label,CONVERT)
	$(Q) mkdir -p $(dir $@)
	$(Q) convert -crop 1005x1975+93+13 -resize 1160x2220 $< $@
ion_simulator_assets_paths += $(addprefix $(BUILD_DIR)/,$(addprefix ion/src/simulator/assets/,background.jpg background-no-shadow.webp))
simulator_app_deps += $(ion_simulator_assets_paths)

# layout.cpp generation

$(eval $(call rule_for, \
  LAYOUT, \
  ion/src/simulator/shared/layout.cpp, \
  $(addprefix ion/src/simulator/shared/,layout.json layout.py), \
  $$(PYTHON) ion/src/simulator/shared/layout.py -i $$< -o $$@, \
  global \
))

ion_src += $(BUILD_DIR)/ion/src/simulator/shared/layout.cpp

include ion/src/simulator/$(TARGET)/Makefile
include ion/src/simulator/external/Makefile

SFLAGS += -DION_EVENTS_JOURNAL

# This flags the ability to store files on the local system
# These include state-files and cached SDL window postion
ifeq ($(ION_SIMULATOR_FILES),1)
ion_src += $(addprefix ion/src/simulator/shared/, \
  actions.cpp \
  state_file.cpp \
  screenshot.cpp \
  platform_files.cpp \
)
ion_src += eadk/src/simulator.cpp
SFLAGS += -DION_SIMULATOR_FILES=1
# Export symbols so that dlopen-ing NWB files can use eadk_external_data and eadk_external_data_size
LDFLAGS += $(LD_EXPORT_SYMBOLS_FLAG)
ION_SIMULATOR_WINDOW_SETUP ?= ion/src/simulator/shared/window_position_cached.cpp
else
ION_SIMULATOR_WINDOW_SETUP ?= ion/src/simulator/shared/dummy/window_position.cpp
endif
ion_src += $(ION_SIMULATOR_WINDOW_SETUP)
