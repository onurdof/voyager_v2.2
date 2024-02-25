idf_component_register(SRCS "stepper_motor_encoder.c" "stepper.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver freertos intercommunication)
