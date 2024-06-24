################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../service/src/lnx_msq.c \
../service/src/lnx_mutex.c \
../service/src/lnx_socket.c \
../service/src/lnx_task.c \
../service/src/srv_bitmap.c \
../service/src/srv_buffer.c \
../service/src/srv_task.c \
../service/src/srv_trace.c 

OBJS += \
./service/src/lnx_msq.o \
./service/src/lnx_mutex.o \
./service/src/lnx_socket.o \
./service/src/lnx_task.o \
./service/src/srv_bitmap.o \
./service/src/srv_buffer.o \
./service/src/srv_task.o \
./service/src/srv_trace.o 

C_DEPS += \
./service/src/lnx_msq.d \
./service/src/lnx_mutex.d \
./service/src/lnx_socket.d \
./service/src/lnx_task.d \
./service/src/srv_bitmap.d \
./service/src/srv_buffer.d \
./service/src/srv_task.d \
./service/src/srv_trace.d 


# Each subdirectory must supply rules for building sources it contributes
service/src/%.o: ../service/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/share/cspt_network_hub/service/inc" -I"/share/cspt_network_hub/adapter" -I"/share/cspt_network_hub/config" -I"/share/cspt_network_hub/cspt/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


