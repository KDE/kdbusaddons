add_executable(kquitapp kquitapp.cpp)
ecm_mark_nongui_executable(kquitapp)
target_link_libraries(kquitapp Qt5::DBus)
install(TARGETS kquitapp_executable ${INSTALL_TARGETS_DEFAULT_ARGS})
