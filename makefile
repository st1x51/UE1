# PSP build Makefile (pspsdk build.mak based)
# Requires the PSPDEV toolchain and pspsdk build support.

# ---- target configuration ----
TARGET          ?= ue1_psp
PSP_EBOOT_TITLE ?= UE1 PSP
PSP_LARGE_MEMORY?= 1
GPROF           ?= 0

# ---- source discovery ----
CPP_SOURCES := $(shell find Source -name "*.cpp")
C_SOURCES   := $(shell find Source -name "*.c")

# Exclude desktop-only drivers and launchers from PSP builds.
CPP_SOURCES := $(filter-out Source/NOpenGLESDrv/%, $(CPP_SOURCES))
CPP_SOURCES := $(filter-out Source/NOpenGLDrv/%,  $(CPP_SOURCES))
CPP_SOURCES := $(filter-out Source/NOpenALDrv/%,  $(CPP_SOURCES))
CPP_SOURCES := $(filter-out Source/NSDLDrv/%,     $(CPP_SOURCES))
CPP_SOURCES := $(filter-out Source/Unreal/Src/SDLLaunch%, $(CPP_SOURCES))
CPP_SOURCES := $(filter-out Source/Window/%, $(CPP_SOURCES))
CPP_SOURCES := $(filter-out Source/WinDrv/%, $(CPP_SOURCES))
CPP_SOURCES := $(filter-out Source/Unreal/Src/WinLaunch%, $(CPP_SOURCES))
CPP_SOURCES := $(filter-out Source/Editor/%, $(CPP_SOURCES))
#CPP_SOURCES += Source/Unreal/Src/PSPLaunch.cpp
#CPP_SOURCES += Source/PSPDrv/Src/PSPClient.cpp
OBJS        := $(CPP_SOURCES:.cpp=.o) $(C_SOURCES:.c=.o)

# ---- include paths ----
INCDIRS := Source $(shell find Source -type d) Thirdparty $(shell find Thirdparty -type d 2>/dev/null)
INCLUDES := $(addprefix -I,$(INCDIRS))

# ---- toolchain ----
PSPSDK := $(shell psp-config --pspsdk-path)
ifeq ($(strip $(PSPSDK)),)
$(error PSP toolchain not found; please ensure PSPDEV is set and psp-config is available)
endif

CXX := psp-g++
CC  := psp-gcc

# Package identifiers to avoid duplicate THIS_PACKAGE symbols.
Source/Core/%.o     : CXXFLAGS += -DUPACKAGE_NAME=Core     -DCORE_EXPORTS
Source/Engine/%.o   : CXXFLAGS += -DUPACKAGE_NAME=Engine   -DENGINE_EXPORTS
Source/Render/%.o   : CXXFLAGS += -DUPACKAGE_NAME=Render   -DRENDER_EXPORTS
Source/Fire/%.o     : CXXFLAGS += -DUPACKAGE_NAME=Fire     -DFIRE_EXPORTS
Source/IpDrv/%.o    : CXXFLAGS += -DUPACKAGE_NAME=IpDrv    -DIPDRV_EXPORTS
Source/SoftDrv/%.o  : CXXFLAGS += -DUPACKAGE_NAME=SoftDrv  -DSOFTDRV_EXPORTS
Source/SoundDrv/%.o : CXXFLAGS += -DUPACKAGE_NAME=SoundDrv -DSOUNDDRV_EXPORTS
Source/Unreal/%.o   : CXXFLAGS += -DUPACKAGE_NAME=Unreal   -DUNREAL_EXPORTS
Source/PSPDrv/%.o   : CXXFLAGS += -DUPACKAGE_NAME=PSPDrv   -DPSPDRV_EXPORTS

Source/Core/%.o     : CFLAGS += -DUPACKAGE_NAME=Core     -DCORE_EXPORTS
Source/Engine/%.o   : CFLAGS += -DUPACKAGE_NAME=Engine   -DENGINE_EXPORTS
Source/Render/%.o   : CFLAGS += -DUPACKAGE_NAME=Render   -DRENDER_EXPORTS
Source/Fire/%.o     : CFLAGS += -DUPACKAGE_NAME=Fire     -DFIRE_EXPORTS
Source/IpDrv/%.o    : CFLAGS += -DUPACKAGE_NAME=IpDrv    -DIPDRV_EXPORTS
Source/SoftDrv/%.o  : CFLAGS += -DUPACKAGE_NAME=SoftDrv  -DSOFTDRV_EXPORTS
Source/SoundDrv/%.o : CFLAGS += -DUPACKAGE_NAME=SoundDrv -DSOUNDDRV_EXPORTS
Source/Unreal/%.o   : CFLAGS += -DUPACKAGE_NAME=Unreal   -DUNREAL_EXPORTS
Source/PSPDrv/%.o   : CFLAGS += -DUPACKAGE_NAME=PSPDrv   -DPSPDRV_EXPORTS

# ---- flags ----
COMMON_FLAGS = -ffast-math -O2 -G0 -Wall -Wextra -fno-strict-aliasing -fwrapv \
               -DPLATFORM_PSP -DPLATFORM_POSIX -DPLATFORM_LITTLE_ENDIAN \
               -DUNREAL_STATIC -DPSP -DNDEBUG $(INCLUDES)

COMMON_FLAGS += -mno-abicalls -fno-pic
COMMON_FLAGS += -DUSE_UNNAMED_SEM
CXXFLAGS = $(COMMON_FLAGS) -fexceptions -fno-rtti -fno-sized-deallocation
CFLAGS   = $(COMMON_FLAGS)
ASFLAGS  = $(CXXFLAGS)

# ---- libraries ----
# Only list application libs; pspsdk will pull in crt/psp kernel pieces automatically.
LIBS = -lpspgum -lpspgu -lpspaudiolib -lpspaudio -lpsppower -lpsprtc -lpsphprm -lpspctrl \
       -lpspvram -lpspdmac -lpspdebug -lpspdisplay -lpspge -lm -lstdc++

ifeq ($(GPROF),1)
COMMON_FLAGS += -pg -DGPROF_ENABLED
LIBS += -lpspprof
endif

# These variables drive PRX/EBOOT creation inside build.mak
BUILD_PRX       = 1
EXTRA_TARGETS   = EBOOT.PBP
USE_PSPSDK_LIBC = 1

include $(PSPSDK)/lib/build.mak

# ---- compilation rules ----
%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).prx EBOOT.PBP PARAM.SFO
	find . -name "*.o" -delete

after_build:
	@echo "=== Moving build artifacts to ./psp-out/ ==="
	@mkdir -p psp-out
	@mv -f EBOOT.PBP ./psp-out/EBOOT.PBP || true
	@mv -f $(TARGET).elf ./psp-out/$(TARGET).elf || true
	@mv -f $(TARGET).prx ./psp-out/$(TARGET).prx || true
	@echo "=== Done ==="

all: $(EXTRA_TARGETS) after_build
