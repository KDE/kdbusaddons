# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

add_library(KF6DBusAddonsQML)
add_library(KF6::DBusAddonsQML ALIAS KF6DBusAddonsQML)

target_link_libraries(KF6DBusAddonsQML
    PRIVATE
        Qt::DBus
        Qt::DBusPrivate
        Qt::Quick
        PkgConfig::dbus-1
        $<COMPILE_ONLY:KF6DBusAddons>
)

set_target_properties(KF6DBusAddonsQML PROPERTIES
    VERSION ${KDBUSADDONS_VERSION}
    SOVERSION ${KDBUSADDONS_SOVERSION}
    EXPORT_NAME DBusAddonsQML
)

qt_add_dbus_interface(dbus_SRCS org.freedesktop.DBus.Properties.xml org.freedesktop.DBus.Properties)

target_sources(KF6DBusAddonsQML
    PRIVATE
        ${dbus_SRCS}
        dbusinterface.cpp
        dbusvariantwrapper.cpp
        kdbuserror.cpp
        kjsdbusinterface.cpp
        pack.cpp
        unpack.cpp
)

ecm_generate_headers(KDBusAddonsQML_HEADERS
  HEADER_NAMES
      KJSDBusInterface
  REQUIRED_HEADERS KDBusAddonsQML_HEADERS
)

target_include_directories(KF6DBusAddonsQML
    INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/KDBusAddons;${KDE_INSTALL_INCLUDEDIR_KF}/KDBusAddonsQML>")

install(TARGETS KF6DBusAddonsQML EXPORT KF6DBusAddonsTargets ${KF_INSTALL_TARGETS_DEFAULT_ARGS})
install(
    FILES ${KDBusAddonsQML_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KDBusAddonsQML
    COMPONENT Devel
)

############################################

ecm_add_qml_module(KF6DBusAddonsQML URI org.kde.dbusaddons VERSION 1.0 GENERATE_PLUGIN_SOURCE INSTALLED_PLUGIN_TARGET KF6::kdbusaddonsplugin)
ecm_finalize_qml_module(KF6DBusAddonsQML DESTINATION ${KDE_INSTALL_QMLDIR})
