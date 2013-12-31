
set(libkdbusaddons_SRCS
    kdbusconnectionpool.cpp
    kdbusinterprocesslock.cpp
    kdbusservice.cpp
    kdedmodule.cpp
    kdeinitinterface.cpp
)

qt5_add_dbus_adaptor(libkdbusaddons_SRCS
   org.freedesktop.Application.xml
   kdbusservice.h
   KDBusService
   kdbusservice_adaptor
   KDBusServiceAdaptor)
qt5_add_dbus_adaptor(libkdbusaddons_SRCS
   org.kde.KDBusService.xml
   kdbusservice.h
   KDBusService
   kdbusserviceextensions_adaptor
   KDBusServiceExtensionsAdaptor)

add_library(KF5DBusAddons ${libkdbusaddons_SRCS})
generate_export_header(KF5DBusAddons BASE_NAME KDBusAddons)
add_library(KF5::DBusAddons ALIAS KF5DBusAddons)



target_link_libraries(KF5DBusAddons PUBLIC Qt5::DBus)

target_include_directories(KF5DBusAddons INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/kdbusaddons>" "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/KDBusAddons>")

set_target_properties(KF5DBusAddons PROPERTIES VERSION   ${KDBUSADDONS_VERSION_STRING}
                                               SOVERSION ${KDBUSADDONS_SOVERSION}
                                               EXPORT_NAME DBusAddons
)

ecm_generate_headers(
  KDBusConnectionPool
  KDBusInterProcessLock
  KDBusService
  KDEDModule
  KDEInitInterface
  REQUIRED_HEADERS KDBusAddons_HEADERS
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/KDBusAddons DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)

install(TARGETS KF5DBusAddons EXPORT KF5DBusAddonsTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    ${KDBusAddons_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/kdbusaddons_export.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/kdbusaddons COMPONENT Devel
)

