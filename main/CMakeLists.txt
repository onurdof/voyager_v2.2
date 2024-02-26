idf_component_register(
    SRCS "voyager_main.c"
    INCLUDE_DIRS "."
    REQUIRES driver uart_interface stepper buttons intercommunication feedback gidon_encoder pedal_encoder 
    )
