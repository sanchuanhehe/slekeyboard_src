set(SOURCES_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/led/led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keyboard_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keyscan/keyscan_task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keyscan/sle_keyboard_server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keyscan/sle_keyboard_server_adv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keyscan/usb_init_keyboard_app.c
)
set(SOURCES "${SOURCES}" ${SOURCES_LIST} PARENT_SCOPE)

set(PRIVATE_HEADER_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/led
    ${CMAKE_CURRENT_SOURCE_DIR}/keyscan
)
set(PRIVATE_HEADER "${PRIVATE_HEADER}" ${PRIVATE_HEADER_LIST} PARENT_SCOPE)
