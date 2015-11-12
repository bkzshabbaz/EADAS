################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
src/lcd.obj: ../src/lcd.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/home/nisarg/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/bin/cl430" -vmspx --abi=eabi --data_model=restricted -Ooff --opt_for_speed=3 --use_hw_mpy=F5 --include_path="/home/nisarg/ti/ccsv6/ccs_base/msp430/include" --include_path="/home/nisarg/ti/ccsv6/tools/compiler/ti-cgt-msp430_4.4.5/include" --advice:power=all --advice:hw_config=all -g --define=__MSP430FR6989__ --define=_MPU_ENABLE --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="src/lcd.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: $<'
	@echo ' '


