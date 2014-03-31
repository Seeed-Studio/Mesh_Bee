##    
## Makefile
## Firmware for SeeedStudio Mesh_Bee(Zigbee) module 
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

JENNIC_PCB ?= MESH_BEE

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
TRACE ?=1
TRACE_ALL?=1

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

ifeq ($(TRACE), 1)

CFLAGS  += -DDBG_ENABLE
$(info Building trace version ...)

ifeq ($(TRACE_ALL), 1) 
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

else

CFLAGS  += -DTRACE_EP=0
CFLAGS  += -DTRACE_OTA=0
CFLAGS  += -DTRACE_JOIN=1
CFLAGS  += -DTRACE_UART=0
CFLAGS  += -DTRACE_NODE=1
CFLAGS  += -DTRACE_NODE_HIGH=1
CFLAGS  += -DTRACE_START=1
CFLAGS  += -DTRACE_TIMERS=0
CFLAGS  += -DTRACE_ZBP_UTILS=0
CFLAGS  += -DTRACE_EXCEPTION=1
CFLAGS  += -DTRACE_ATAPI=0

endif

else

CFLAGS  += -DTRACE_EP=0
CFLAGS  += -DTRACE_OTA=0
CFLAGS  += -DTRACE_JOIN=0
CFLAGS  += -DTRACE_UART=0
CFLAGS  += -DTRACE_NODE=0
CFLAGS  += -DTRACE_NODE_HIGH=0
CFLAGS  += -DTRACE_START=0
CFLAGS  += -DTRACE_TIMERS=0
CFLAGS  += -DTRACE_ZBP_UTILS=0
CFLAGS  += -DTRACE_EXCEPTION=0
CFLAGS  += -DTRACE_ATAPI=0

endif

###############################################################################
# Path definitions

# Use if application directory contains multiple targets
SDK_BASE_DIR    = $(abspath ../../../)
APP_BASE        = $(abspath ../)
APP_INC_DIR     = $(APP_BASE)/include
APP_BLD_DIR     = $(APP_BASE)/build
APP_SRC_DIR     = $(APP_BASE)/src
UTIL_SRC_DIR    = $(COMPONENTS_BASE_DIR)/Utilities/Source

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
APPSRC += zigbee_main.c
APPSRC += zigbee_node.c
APPSRC += zigbee_endpoint.c
APPSRC += zigbee_join.c
APPSRC += zigbee_zbp_utilities.c

APPSRC += firmware_ringbuffer.c
APPSRC += firmware_at_api.c
APPSRC += firmware_uart.c
APPSRC += firmware_ota.c
APPSRC += firmware_hal.c
####
APP_ZPSCFG = MeshBee.zpscfg

###############################################################################
# Standard Application header search paths

INCFLAGS += -I$(APP_INC_DIR)
INCFLAGS += -I$(APP_BLD_DIR)/gen

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
vpath % $(APP_SRC_DIR):$(APP_BLD_DIR)/gen:$(SE_SRC_DIRS):$(UTIL_SRC_DIR)


all: $(APP_BLD_DIR)/output/$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).bin


$(APP_BLD_DIR)/gen/os_gen.c $(APP_BLD_DIR)/gen/os_gen.h $(APP_BLD_DIR)/gen/os_irq.S $(APP_BLD_DIR)/gen/os_irq_alignment.S $(APP_BLD_DIR)/gen/os_irq_buserror.S $(APP_BLD_DIR)/gen/os_irq_illegalinstruction.S $(APP_BLD_DIR)/gen/os_irq_stackoverflowexception.S $(APP_BLD_DIR)/gen/os_irq_unimplementedmodule.S: $(APP_SRC_DIR)/MeshBee.oscfgdiag $(OSCONFIG) 
	$(info Configuring the OS ...)
	$(OSCONFIG) -f "$(shell cygpath -w "$<")" -o "$(shell cygpath -w "$(APP_BLD_DIR)/gen")" -v $(JENNIC_CHIP)
	@echo

$(APP_BLD_DIR)/gen/pdum_gen.c $(APP_BLD_DIR)/gen/pdum_gen.h: $(APP_SRC_DIR)/$(APP_ZPSCFG) $(PDUMCONFIG)
	$(info Configuring the PDUM ...)
	$(PDUMCONFIG) -z $(TARGET) -f "$(shell cygpath -w "$<")" -o "$(shell cygpath -w "$(APP_BLD_DIR)/gen")"
	@echo

$(APP_BLD_DIR)/gen/zps_gen.c $(APP_BLD_DIR)/gen/zps_gen.h: $(APP_SRC_DIR)/$(APP_ZPSCFG) $(ZPSCONFIG)
	$(info Configuring the Zigbee Protocol Stack ...)
	$(ZPSCONFIG) -n $(TARGET) -t $(JENNIC_CHIP) -l $(ZPS_NWK_LIB) -a $(ZPS_APL_LIB) -f "$(shell cygpath -w "$<")" -o "$(shell cygpath -w "$(APP_BLD_DIR)/gen")"
	@echo

%.o: %.S
	$(info Assembling $< ...)
	$(CC) -c -o $(APP_BLD_DIR)/obj/$@ $(CFLAGS) $(INCFLAGS) $< -MD -MF $(APP_BLD_DIR)/obj/$*.d -MP
	@echo

%.o: %.c 
	$(info Compiling $< ...)
	$(CC) -c -o $(APP_BLD_DIR)/obj/$@ $(CFLAGS) $(INCFLAGS) $< -MD -MF $(APP_BLD_DIR)/obj/$*.d -MP
	@echo

$(APP_BLD_DIR)/output/$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).elf: $(APPOBJS) $(addsuffix.a,$(addprefix $(COMPONENTS_BASE_DIR)/Library/lib,$(APPLDLIBS))) 
	$(info Linking $@ ...)
	$(CC) -Wl,--gc-sections -Wl,-u_AppColdStart -Wl,-u_AppWarmStart $(LDFLAGS) -TAppBuild_JN5168.ld -o $@ -Wl,--start-group $(addprefix $(APP_BLD_DIR)/obj/,$(APPOBJS)) $(addprefix -l,$(LDLIBS)) -Wl,--end-group -Wl,-Map,$(APP_BLD_DIR)/output/$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).map 	 
	$(SIZE) $@ 

$(APP_BLD_DIR)/output/$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).bin: $(APP_BLD_DIR)/output/$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).elf 
	$(info Generating binary ...)
	$(OBJCOPY) -j .version -j .bir -j .flashheader -j .vsr_table -j .vsr_handlers  -j .ro_mac_address -j .ro_ota_header -j .ro_se_lnkKey -j .ro_se_cert -j.ro_se_pvKey -j .ro_se_customData -j .rodata -j .text -j .data -j .bss -j .heap -j .stack -S -O binary $< $@
	@echo

	
###############################################################################

clean:
	rm -f $(APP_BLD_DIR)/obj/* $(APP_BLD_DIR)/output/$(TARGET)_$(JENNIC_CHIP)*.elf $(APP_BLD_DIR)/output/$(TARGET)_$(JENNIC_CHIP)*.map
	rm -f $(APP_BLD_DIR)/gen/os_gen.c $(APP_BLD_DIR)/gen/os_gen.h $(APP_BLD_DIR)/gen/os_irq*.S $(APP_BLD_DIR)/gen/pdum_gen.* $(APP_BLD_DIR)/gen/zps_gen*.*

###############################################################################
