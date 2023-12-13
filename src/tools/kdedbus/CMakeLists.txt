remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_executable(kdedbus6 main.cpp)
ecm_mark_nongui_executable(kdedbus6)
target_link_libraries(kdedbus6 Qt6::DBus Qt6::Xml)
install(TARGETS kdedbus6 ${KF_INSTALL_TARGETS_DEFAULT_ARGS})
