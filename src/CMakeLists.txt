add_subdirectory(tools/kquitapp)

add_library(KF5DBusAddons)
add_library(KF5::DBusAddons ALIAS KF5DBusAddons)

ecm_create_qm_loader(KF5DBusAddons kdbusaddons5_qt)

target_sources(KF5DBusAddons PRIVATE
    kdbusinterprocesslock.cpp
    kdbusinterprocesslock.h
    kdbusservice.cpp
    kdbusservice.h
    kdedmodule.cpp
    kdedmodule.h
    kdeinitinterface.cpp
    kdeinitinterface.h
    updatelaunchenvironmentjob.cpp
    updatelaunchenvironmentjob.h
)
if (QT_MAJOR_VERSION EQUAL "5")
    target_sources(KF5DBusAddons PRIVATE
        kdbusconnectionpool.cpp
        kdbusconnectionpool.h
    )
endif()

ecm_qt_declare_logging_category(KF5DBusAddons
    HEADER kdbusaddons_debug.h
    IDENTIFIER KDBUSADDONS_LOG
    CATEGORY_NAME kf.dbusaddons
    OLD_CATEGORY_NAMES kf5.kdbusaddons
    DESCRIPTION "KDBusAddons"
    EXPORT KDBUSADDONS
)

set(libkdbusaddons_dbus_SRCS)
qt_add_dbus_interface(libkdbusaddons_dbus_SRCS org.freedesktop.Application.xml FreeDesktopApplpicationIface)
qt_add_dbus_interface(libkdbusaddons_dbus_SRCS org.kde.KDBusService.xml KDBusServiceIface)

qt_add_dbus_adaptor(libkdbusaddons_dbus_SRCS
   org.freedesktop.Application.xml
   kdbusservice.h
   KDBusService
   kdbusservice_adaptor
   KDBusServiceAdaptor)
qt_add_dbus_adaptor(libkdbusaddons_dbus_SRCS
   org.kde.KDBusService.xml
   kdbusservice.h
   KDBusService
   kdbusserviceextensions_adaptor
   KDBusServiceExtensionsAdaptor)

target_sources(KF5DBusAddons PRIVATE
    ${libkdbusaddons_dbus_SRCS}
)

ecm_generate_export_header(KF5DBusAddons
    BASE_NAME KDBusAddons
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 5.68
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)

if(TARGET Qt5::X11Extras)
    set(_qt_x11_extras_libs Qt5::X11Extras)
elseif(TARGET Qt6::Gui)
    set(_qt_x11_extras_libs Qt6::GuiPrivate) # qtx11extras_p.h
endif()

target_link_libraries(KF5DBusAddons PUBLIC Qt${QT_MAJOR_VERSION}::DBus PRIVATE ${_qt_x11_extras_libs})

target_include_directories(KF5DBusAddons INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/KDBusAddons>")

set_target_properties(KF5DBusAddons PROPERTIES VERSION   ${KDBUSADDONS_VERSION}
                                               SOVERSION ${KDBUSADDONS_SOVERSION}
                                               EXPORT_NAME DBusAddons
)

configure_file(config-kdbusaddons.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kdbusaddons.h )

ecm_generate_headers(KDBusAddons_HEADERS
  HEADER_NAMES
  KDBusInterProcessLock
  KDBusService
  KDEDModule
  KDEInitInterface
  UpdateLaunchEnvironmentJob
  REQUIRED_HEADERS KDBusAddons_HEADERS
)
if (QT_MAJOR_VERSION EQUAL "5")
    ecm_generate_headers(KDBusAddons_Legacy_HEADERS
        HEADER_NAMES
        KDBusConnectionPool
        REQUIRED_HEADERS KDBusAddons_Legacy_HEADERS
    )
endif()

install(TARGETS KF5DBusAddons EXPORT KF5DBusAddonsTargets ${KF_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    ${KDBusAddons_HEADERS}
    ${KDBusAddons_Legacy_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/kdbusaddons_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KDBusAddons COMPONENT Devel
)

ecm_qt_install_logging_categories(
    EXPORT KDBUSADDONS
    FILE kdbusaddons.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

if(BUILD_QCH)
    ecm_add_qch(
        KF5DBusAddons_QCH
        NAME KDBusAddons
        BASE_NAME KF5DBusAddons
        VERSION ${KF_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${KDBusAddons_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt5DBus_QCH
            Qt5Core_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            KDBUSADDONS_EXPORT
            KDBUSADDONS_DEPRECATED
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KDBusAddons LIB_NAME KF5DBusAddons DEPS "dbus" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF}/KDBusAddons)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
