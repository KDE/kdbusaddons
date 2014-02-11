add_executable(kquitapp5 kquitapp.cpp)
ecm_mark_nongui_executable(kquitapp5)
target_link_libraries(kquitapp5 Qt5::DBus)
install(TARGETS kquitapp5 ${INSTALL_TARGETS_DEFAULT_ARGS})
