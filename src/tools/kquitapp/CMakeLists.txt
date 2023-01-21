add_executable(kquitapp5 kquitapp.cpp)
ecm_mark_nongui_executable(kquitapp5)
target_link_libraries(kquitapp5 Qt6::DBus)
install(TARGETS kquitapp5 ${KF_INSTALL_TARGETS_DEFAULT_ARGS})
