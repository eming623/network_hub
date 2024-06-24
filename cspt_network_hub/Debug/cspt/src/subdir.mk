################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../cspt/src/cspt_entry.c \
../cspt/src/cspt_kernel.c \
../cspt/src/cspt_tcpsender.c \
../cspt/src/cspt_tcpserver.c \
../cspt/src/cspt_udp.c 

OBJS += \
./cspt/src/cspt_entry.o \
./cspt/src/cspt_kernel.o \
./cspt/src/cspt_tcpsender.o \
./cspt/src/cspt_tcpserver.o \
./cspt/src/cspt_udp.o 

C_DEPS += \
./cspt/src/cspt_entry.d \
./cspt/src/cspt_kernel.d \
./cspt/src/cspt_tcpsender.d \
./cspt/src/cspt_tcpserver.d \
./cspt/src/cspt_udp.d 


# Each subdirectory must supply rules for building sources it contributes
cspt/src/%.o: ../cspt/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/share/cspt_network_hub/service/inc" -I"/share/cspt_network_hub/adapter" -I"/share/cspt_network_hub/config" -I"/share/cspt_network_hub/cspt/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


