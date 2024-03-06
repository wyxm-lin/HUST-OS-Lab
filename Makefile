# we assume that the utilities from RISC-V cross-compiler (i.e., riscv64-unknown-elf-gcc and etc.)
# are in your system PATH. To check if your environment satisfies this requirement, simple use 
# `which` command as follows:
# $ which riscv64-unknown-elf-gcc
# if you have an output path, your environment satisfy our requirement.

# ---------------------	macros --------------------------
CROSS_PREFIX 	:= riscv64-unknown-elf-
CC 				:= $(CROSS_PREFIX)gcc
AR 				:= $(CROSS_PREFIX)ar
RANLIB        	:= $(CROSS_PREFIX)ranlib

SRC_DIR        	:= .
OBJ_DIR 		:= obj
SPROJS_INCLUDE 	:= -I.  

HOSTFS_ROOT := hostfs_root
ifneq (,)
  march := -march=
  is_32bit := $(findstring 32,$(march))
  mabi := -mabi=$(if $(is_32bit),ilp32,lp64)
endif

CFLAGS        := -Wall -Werror  -fno-builtin -nostdlib -D__NO_INLINE__ -mcmodel=medany -g -Og -std=gnu99 -Wno-unused -Wno-attributes -fno-delete-null-pointer-checks -fno-PIE $(march)
COMPILE       	:= $(CC) -MMD -MP $(CFLAGS) $(SPROJS_INCLUDE)

#---------------------	utils -----------------------
UTIL_CPPS 	:= util/*.c

UTIL_CPPS  := $(wildcard $(UTIL_CPPS))
UTIL_OBJS  :=  $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(UTIL_CPPS)))


UTIL_LIB   := $(OBJ_DIR)/util.a

#---------------------	kernel -----------------------
KERNEL_LDS  	:= kernel/kernel.lds
KERNEL_CPPS 	:= \
	kernel/*.c \
	kernel/machine/*.c \
	kernel/util/*.c

KERNEL_ASMS 	:= \
	kernel/*.S \
	kernel/machine/*.S \
	kernel/util/*.S

KERNEL_CPPS  	:= $(wildcard $(KERNEL_CPPS))
KERNEL_ASMS  	:= $(wildcard $(KERNEL_ASMS))
KERNEL_OBJS  	:=  $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(KERNEL_CPPS)))
KERNEL_OBJS  	+=  $(addprefix $(OBJ_DIR)/, $(patsubst %.S,%.o,$(KERNEL_ASMS)))

KERNEL_TARGET = $(OBJ_DIR)/riscv-pke


#---------------------	spike interface library -----------------------
SPIKE_INF_CPPS 	:= spike_interface/*.c

SPIKE_INF_CPPS  := $(wildcard $(SPIKE_INF_CPPS))
SPIKE_INF_OBJS 	:=  $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(SPIKE_INF_CPPS)))


SPIKE_INF_LIB   := $(OBJ_DIR)/spike_interface.a


#---------------------	user   -----------------------
USER_CPPS 		:= user/app_shell.c user/user_lib.c

USER_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_CPPS)))

USER_TARGET 	:= $(HOSTFS_ROOT)/bin/app_shell

USER_CPPS0 		:= user/app0.c user/user_lib.c

USER_OBJS0  	:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_CPPS0)))

USER_TARGET0 	:= $(HOSTFS_ROOT)/bin/app0

USER_CPPS1 		:= user/app1.c user/user_lib.c

USER_OBJS1  	:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_CPPS1)))

USER_TARGET1 	:= $(HOSTFS_ROOT)/bin/app1

USER_CPPS2 		:= user/lab3_1.c user/user_lib.c

USER_OBJS2  	:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_CPPS2)))

USER_TARGET2	:= $(HOSTFS_ROOT)/bin/lab3_1

USER_CPPS3 		:= user/lab3_2.c user/user_lib.c

USER_OBJS3  	:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_CPPS3)))

USER_TARGET3	:= $(HOSTFS_ROOT)/bin/lab3_2

USER_CPPS4 		:= user/lab3_3.c user/user_lib.c

USER_OBJS4  	:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_CPPS4)))

USER_TARGET4	:= $(HOSTFS_ROOT)/bin/lab3_3
#------------------------targets------------------------
$(OBJ_DIR):
	@-mkdir -p $(OBJ_DIR)	
	@-mkdir -p $(dir $(UTIL_OBJS))
	@-mkdir -p $(dir $(SPIKE_INF_OBJS))
	@-mkdir -p $(dir $(KERNEL_OBJS))
	@-mkdir -p $(dir $(USER_OBJS))
	@-mkdir -p $(dir $(USER_OBJS0))
	@-mkdir -p $(dir $(USER_OBJS1))
	@-mkdir -p $(dir $(USER_OBJS2))
	@-mkdir -p $(dir $(USER_OBJS3))
	@-mkdir -p $(dir $(USER_OBJS4))
	
$(OBJ_DIR)/%.o : %.c
	@echo "compiling" $<
	@$(COMPILE) -c $< -o $@

$(OBJ_DIR)/%.o : %.S
	@echo "compiling" $<
	@$(COMPILE) -c $< -o $@

$(UTIL_LIB): $(OBJ_DIR) $(UTIL_OBJS)
	@echo "linking " $@	...	
	@$(AR) -rcs $@ $(UTIL_OBJS) 
	@echo "Util lib has been build into" \"$@\"
	
$(SPIKE_INF_LIB): $(OBJ_DIR) $(UTIL_OBJS) $(SPIKE_INF_OBJS)
	@echo "linking " $@	...	
	@$(AR) -rcs $@ $(SPIKE_INF_OBJS) $(UTIL_OBJS)
	@echo "Spike lib has been build into" \"$@\"

$(KERNEL_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(SPIKE_INF_LIB) $(KERNEL_OBJS) $(KERNEL_LDS)
	@echo "linking" $@ ...
	@$(COMPILE) $(KERNEL_OBJS) $(UTIL_LIB) $(SPIKE_INF_LIB) -o $@ -T $(KERNEL_LDS)
	@echo "PKE core has been built into" \"$@\"

$(USER_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"
	@cp $@ $(OBJ_DIR)

$(USER_TARGET0): $(OBJ_DIR) $(UTIL_LIB) $(USER_OBJS0)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_OBJS0) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"
	@cp $@ $(OBJ_DIR)

$(USER_TARGET1): $(OBJ_DIR) $(UTIL_LIB) $(USER_OBJS1)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_OBJS1) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"
	@cp $@ $(OBJ_DIR)

$(USER_TARGET2): $(OBJ_DIR) $(UTIL_LIB) $(USER_OBJS2)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_OBJS2) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"
	@cp $@ $(OBJ_DIR)

$(USER_TARGET3): $(OBJ_DIR) $(UTIL_LIB) $(USER_OBJS3)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_OBJS3) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"
	@cp $@ $(OBJ_DIR)

$(USER_TARGET4): $(OBJ_DIR) $(UTIL_LIB) $(USER_OBJS4)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_OBJS4) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"
	@cp $@ $(OBJ_DIR)

-include $(wildcard $(OBJ_DIR)/*/*.d)
-include $(wildcard $(OBJ_DIR)/*/*/*.d)

.DEFAULT_GOAL := $(all)

all: $(KERNEL_TARGET) $(USER_TARGET) $(USER_TARGET0) $(USER_TARGET1) $(USER_TARGET2) $(USER_TARGET3) $(USER_TARGET4)
	@echo "********************HUST PKE********************"
	@echo "All targets have been built successfully!"
.PHONY:all

run: $(KERNEL_TARGET) $(USER_TARGET)
	@echo "********************HUST PKE********************"
	spike -p2 $(KERNEL_TARGET) /bin/lab3_3

# need openocd!
gdb:$(KERNEL_TARGET) $(USER_TARGET)
	spike --rbb-port=9824 -H $(KERNEL_TARGET) $(USER_TARGET) &
	@sleep 1
	openocd -f ./.spike.cfg &
	@sleep 1
	riscv64-unknown-elf-gdb -command=./.gdbinit

# clean gdb. need openocd!
gdb_clean:
	@-kill -9 $$(lsof -i:9824 -t)
	@-kill -9 $$(lsof -i:3333 -t)
	@sleep 1

objdump:
	riscv64-unknown-elf-objdump -d $(KERNEL_TARGET) > $(OBJ_DIR)/kernel_dump
	riscv64-unknown-elf-objdump -d $(USER_TARGET) > $(OBJ_DIR)/user_dump

cscope:
	find ./ -name "*.c" > cscope.files
	find ./ -name "*.h" >> cscope.files
	find ./ -name "*.S" >> cscope.files
	find ./ -name "*.lds" >> cscope.files
	cscope -bqk

format:
	@python ./format.py ./

clean:
	rm -fr ${OBJ_DIR} ${HOSTFS_ROOT}/bin
