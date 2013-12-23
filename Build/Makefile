##    
## Makefile
## Firmware for SeeedStudio RFBeeV2(Zigbee) module 
##   
## Copyright (c) NXP B.V. 2012.   
## Spread by SeeedStudio
## Author     : Jack Shao
## Create Time: 2013/10 
## Change Log :   
##   
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.  
##

# Application target name
TARGET ?= COO

###############################################################################
#User definable make parameters that may be overwritten from the command line

# Default target device is the JN5148

JENNIC_CHIP ?= JN5168
CERTIFICATES ?= DEVELOPMENT_CERTS

###############################################################################
#Other parameters passed from command line

PDM_BUILD_TYPE ?= _EEPROM

###############################################################################
# Default DK2 development kit target hardware

JENNIC_PCB ?= DEVKIT4

###############################################################################
# Select the network stack (e.g. MAC, ZBPro, SE)

JENNIC_STACK ?= ZBPro

###############################################################################
# ZBPro Stack specific options (ZCR or ZED)
ZBPRO_DEVICE_TYPE ?= ZCR
ifeq ($(TARGET),END)
ZBPRO_DEVICE_TYPE := ZED
endif

###############################################################################
# Debug options - define DEBUG to enable
# DEBUG ?=HW
#
# Define which UART to use for HW debug
# DEBUG_PORT ?= UART1

###############################################################################
# Define TRACE to use with DBG module
# By default TRACE and OVERLAYS have been turned off
# Set them to 1 to enable TRACE and overlays.
TRACE ?=1

### Don't use JENNIC_CHIP_FAMILY here because it may not be set from command line
## It is not set until after the config.mk files are included below
#ifeq ($(JENNIC_CHIP),JN5168)
OVERLAY_BUILD ?=0
#else
#OVERLAY_BUILD ?=1
#endif

FACT_TEST?=0

# Application Flags
CFLAGS += -DTARGET_$(TARGET)
CFLAGS += -D$(CERTIFICATES)
CFLAGS += -D$(JENNIC_PCB)

#OTA support option
OTA_SUPPORT = 1
ifeq ($(OTA_SUPPORT),1)
OTA_DEFAULT_VERSION = 1
CFLAGS += -DOTA_SUPPORT_OPTIONS
endif

ifeq ($(FACT_TEST),1)
CFLAGS += -DFACT_TEST
BIN_SUFFIX=_TEST
endif



ifeq ($(TRACE), 1)
CFLAGS  += -DDBG_ENABLE
$(info Building trace version ...)

# Enable any debug output here:
CFLAGS  += -DTRACE_EP=1
CFLAGS  += -DTRACE_NODE=1
CFLAGS  += -DTRACE_NODE_HIGH=1
CFLAGS  += -DTRACE_ATAPI=1
CFLAGS  += -DTRACE_OTA=1
CFLAGS  += -DTRACE_JOIN=1
CFLAGS  += -DTRACE_UART=1
CFLAGS  += -DTRACE_START=1
CFLAGS  += -DTRACE_TIMERS=1
CFLAGS  += -DTRACE_ZBP_UTILS=1
CFLAGS  += -DTRACE_EXCEPTION=1 


else  #TRACE != 1

CFLAGS  += -DTRACE_EP=0
CFLAGS  += -DTRACE_OTA=0
CFLAGS  += -DTRACE_JOIN=0
CFLAGS  += -DTRACE_UART=0
CFLAGS  += -DTRACE_NODE=0
CFLAGS  += -DTRACE_NODE_HIGH=1
CFLAGS  += -DTRACE_START=0
CFLAGS  += -DTRACE_TIMERS=0
CFLAGS  += -DTRACE_ZBP_UTILS=0
CFLAGS  += -DTRACE_EXCEPTION=0 
CFLAGS  += -DTRACE_ATAPI=0


endif

###############################################################################
# Path definitions

# Use if application directory contains multiple targets
SDK_BASE_DIR		= $(abspath ../../../)
APP_BASE		= $(abspath ../)
APP_BLD_DIR		= $(APP_BASE)/Build
APP_SRC_DIR		= $(APP_BASE)/Source
UTIL_SRC_DIR		= $(COMPONENTS_BASE_DIR)/Utilities/Source

###############################################################################
# Application Source files

# Note: Path to source file is found using vpath below, so only .c filename is required
APPSRC = os_gen.c
APPSRC += os_irq.S
APPSRC += os_irq_alignment.S
APPSRC += os_irq_buserror.S
APPSRC += os_irq_illegalinstruction.S
APPSRC += os_irq_stackoverflowexception.S
APPSRC += os_irq_unimplementedmodule.S
APPSRC += pdum_gen.c
APPSRC += zps_gen.c
##########
APPSRC += app_timer_driver.c
APPSRC += app_main.c
APPSRC += node.c
APPSRC += endpoint.c
APPSRC += app_zbp_utilities.c

APPSRC += ringbuffer.c
APPSRC += at_api.c
APPSRC += uart.c
APPSRC += app_ota.c
APPSRC += zigbee_join.c


####
APP_ZPSCFG = app.zpscfg

###############################################################################
# Standard Application header search paths

INCFLAGS += -I$(APP_SRC_DIR)
INCFLAGS += -I$(APP_SRC_DIR)/gen
INCFLAGS += -I$(APP_SRC_DIR)/..

INCFLAGS += -I$(COMPONENTS_BASE_DIR)/Utilities/Include
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/Recal/Include

###############################################################################
# Application libraries
# Specify additional Component libraries

APPLIBS += Recal

###############################################################################

# You should not need to edit below this line

###############################################################################
###############################################################################
# Configure for the selected chip or chip family

include $(SDK_BASE_DIR)/Chip/Common/Build/config.mk
include $(SDK_BASE_DIR)/Platform/Common/Build/Config.mk
include $(SDK_BASE_DIR)/Stack/Common/Build/config.mk

###############################################################################

TEMP = $(APPSRC:.c=.o)
APPOBJS = $(TEMP:.S=.o)

###############################################################################
# Application dynamic dependencies

APPDEPS = $(APPOBJS:.o=.d)

###############################################################################
# Linker

# Add application libraries before chip specific libraries to linker so
# symbols are resolved correctly (i.e. ordering is significant for GCC)

APPLDLIBS := $(foreach lib,$(APPLIBS),$(if $(wildcard $(addprefix $(COMPONENTS_BASE_DIR)/Library/lib,$(addsuffix _$(TEMPCHIP).a,$(lib)))),$(addsuffix _$(TEMPCHIP),$(lib)),$(addsuffix _$(JENNIC_CHIP_FAMILY),$(lib))))
LDLIBS := $(APPLDLIBS) $(LDLIBS)

LDFLAGS += -L$(ZCL_BASE)/Build
LDFLAGS += -L$(STACK_BASE_DIR)/SE/Build
LDFLAGS += -L$(STACK_BASE_DIR)/ZBPro/Build

###############################################################################
# Dependency rules

.PHONY: all clean
# Path to directories containing application source 
vpath % $(APP_SRC_DIR):$(APP_SRC_DIR)/gen:$(SE_SRC_DIRS):$(UTIL_SRC_DIR)


all: $(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).bin


#-include $(APPDEPS)
#%.d:
#	rm -f $*.o

$(APP_SRC_DIR)/gen/os_gen.c $(APP_SRC_DIR)/gen/os_gen.h $(APP_SRC_DIR)/gen/os_irq.S $(APP_SRC_DIR)/gen/os_irq_alignment.S $(APP_SRC_DIR)/gen/os_irq_buserror.S $(APP_SRC_DIR)/gen/os_irq_illegalinstruction.S $(APP_SRC_DIR)/gen/os_irq_stackoverflowexception.S $(APP_SRC_DIR)/gen/os_irq_unimplementedmodule.S: $(APP_SRC_DIR)/App_AllInOne_$(JENNIC_CHIP_FAMILY).oscfgdiag $(OSCONFIG) 
	$(info Configuring the OS ...)
	$(OSCONFIG) -f "$(shell cygpath -w "$<")" -o "$(shell cygpath -w "$(APP_SRC_DIR)/gen")" -v $(JENNIC_CHIP)
	@echo

$(APP_SRC_DIR)/gen/pdum_gen.c $(APP_SRC_DIR)/gen/pdum_gen.h: $(APP_SRC_DIR)/$(APP_ZPSCFG) $(PDUMCONFIG)
	$(info Configuring the PDUM ...)
	$(PDUMCONFIG) -z $(TARGET) -f "$(shell cygpath -w "$<")" -o "$(shell cygpath -w "$(APP_SRC_DIR)/gen")"
	@echo

$(APP_SRC_DIR)/gen/zps_gen.c $(APP_SRC_DIR)/gen/zps_gen.h: $(APP_SRC_DIR)/$(APP_ZPSCFG) $(ZPSCONFIG)
	$(info Configuring the Zigbee Protocol Stack ...)
	$(ZPSCONFIG) -n $(TARGET) -t $(JENNIC_CHIP) -l $(ZPS_NWK_LIB) -a $(ZPS_APL_LIB) -f "$(shell cygpath -w "$<")" -o "$(shell cygpath -w "$(APP_SRC_DIR)/gen")"
	@echo

%.o: %.S
	$(info Assembling $< ...)
	$(CC) -c -o $(subst Source,Build,$@) $(CFLAGS) $(INCFLAGS) $< -MD -MF $*.d -MP
	@echo

%.o: %.c 
	$(info Compiling $< ...)
	$(CC) -c -o $(subst Source,Build,$@) $(CFLAGS) $(INCFLAGS) $< -MD -MF $*.d -MP
	@echo

$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).elf: $(APPOBJS) $(addsuffix.a,$(addprefix $(COMPONENTS_BASE_DIR)/Library/lib,$(APPLDLIBS))) 
	$(info Linking $@ ...)
	$(CC) -Wl,--gc-sections -Wl,-u_AppColdStart -Wl,-u_AppWarmStart $(LDFLAGS) -TAppBuild_JN5168.ld -o $@ -Wl,--start-group $(APPOBJS) $(addprefix -l,$(LDLIBS)) -Wl,--end-group -Wl,-Map,$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).map 	 
	$(SIZE) $@ 

$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).bin: $(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).elf 
	$(info Generating binary ...)
	$(OBJCOPY) -j .version -j .bir -j .flashheader -j .vsr_table -j .vsr_handlers  -j .ro_mac_address -j .ro_ota_header -j .ro_se_lnkKey -j .ro_se_cert -j.ro_se_pvKey -j .ro_se_customData -j .rodata -j .text -j .data -j .bss -j .heap -j .stack -S -O binary $< $@
	@echo

	
###############################################################################

clean:
	rm -f $(APPOBJS) $(APPDEPS) $(TARGET)_$(JENNIC_CHIP)*.elf $(TARGET)_$(JENNIC_CHIP)*.map
	rm -f $(APP_SRC_DIR)/gen/os_gen.c $(APP_SRC_DIR)/gen/os_gen.h $(APP_SRC_DIR)/gen/os_irq*.S $(APP_SRC_DIR)/gen/pdum_gen.* $(APP_SRC_DIR)/gen/zps_gen*.*

###############################################################################
