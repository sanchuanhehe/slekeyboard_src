#===============================================================================
# @brief    cmake file
# Copyright (c) CompanyNameMagicTag 2022-2023. All rights reserved.
#===============================================================================
set(COMPONENT_NAME "sle_keyboard")

set(SOURCES
    # ${CMAKE_CURRENT_SOURCE_DIR}/blinky_cmsis.c
)

set(PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(PRIVATE_HEADER
)

set(PRIVATE_DEFINES
)

set(PUBLIC_DEFINES
)

# use this when you want to add ccflags like -include xxx
set(COMPONENT_PUBLIC_CCFLAGS
)

set(COMPONENT_CCFLAGS
)

if(DEFINED CONFIG_SLE_KEYBOARD_ENABLE)
    if(DEFINED CONFIG_SELECT_KEYBOARD)
        add_subdirectory_if_exist(keyboard)
    elseif(DEFINED CONFIG_SELECT_DONGLE)
        add_subdirectory_if_exist(dongle)
    endif()
endif()


set(WHOLE_LINK
    true
)

set(MAIN_COMPONENT
    false
)

build_component()
