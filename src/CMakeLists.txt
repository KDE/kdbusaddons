add_subdirectory(tools/kquitapp)

ecm_create_qm_loader(libkdbusaddons_QM_LOADER kdbusaddons5_qt)

set(libkdbusaddons_SRCS
    kdbusconnectionpool.cpp
    kdbusinterprocesslock.cpp
    kdbusservice.cpp
    kdedmodule.cpp
    kdeinitinterface.cpp
    ${libkdbusaddons_QM_LOADER}
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


target_link_libraries(KF5DBusAddons PUBLIC Qt5::DBus PRIVATE ${qtx11extras})

target_include_directories(KF5DBusAddons INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/KDBusAddons>")

set_target_properties(KF5DBusAddons PROPERTIES VERSION   ${KDBUSADDONS_VERSION_STRING}
                                               SOVERSION ${KDBUSADDONS_SOVERSION}
                                               EXPORT_NAME DBusAddons
)

configure_file(config-kdbusaddons.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kdbusaddons.h )

ecm_generate_headers(KDBusAddons_HEADERS
  HEADER_NAMES
  KDBusConnectionPool
  KDBusInterProcessLock
  KDBusService
  KDEDModule
  KDEInitInterface
  REQUIRED_HEADERS KDBusAddons_HEADERS
)

install(TARGETS KF5DBusAddons EXPORT KF5DBusAddonsTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    ${KDBusAddons_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/kdbusaddons_export.h
    DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KDBusAddons COMPONENT Devel
)


include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KDBusAddons LIB_NAME KF5DBusAddons DEPS "dbus" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KF5_INCLUDE_INSTALL_DIR}/KDBusAddons)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
