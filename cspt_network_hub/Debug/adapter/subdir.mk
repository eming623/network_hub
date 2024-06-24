################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../adapter/cspt_adapter.c 

OBJS += \
./adapter/cspt_adapter.o 

C_DEPS += \
./adapter/cspt_adapter.d 


# Each subdirectory must supply rules for building sources it contributes
adapter/%.o: ../adapter/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/share/cspt_network_hub/service/inc" -I"/share/cspt_network_hub/adapter" -I"/share/cspt_network_hub/config" -I"/share/cspt_network_hub/cspt/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


