cmake_minimum_required(VERSION 3.16)

set(KF_VERSION "5.240.0") # handled by release scripts
project(KDBusAddons VERSION ${KF_VERSION})

include(FeatureSummary)
find_package(ECM 5.240.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)


set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(KDEGitCommitHooks)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)

set(REQUIRED_QT_VERSION 6.4.0)
find_package(Qt${QT_MAJOR_VERSION}DBus ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)

if (QT_MAJOR_VERSION STREQUAL "5")
    find_package(Qt5X11Extras ${REQUIRED_QT_VERSION} CONFIG)
elseif (QT_MAJOR_VERSION STREQUAL "6")
    find_package(Qt6Gui ${REQUIRED_QT_VERSION} CONFIG) # qtx11extras_p.h
endif()

include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMGenerateExportHeader)
include(ECMMarkNonGuiExecutable)
include(ECMAddQch)

include(ECMPoQmTools)

ecm_setup_version(PROJECT VARIABLE_PREFIX KDBUSADDONS
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kdbusaddons_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF6DBusAddonsConfigVersion.cmake"
                        SOVERSION 6)

# qtx11extras_p.h is in Qt6Gui
if(TARGET Qt5::X11Extras OR (TARGET Qt6::Gui AND NOT WIN32))
    set(HAVE_X11 TRUE)
else()
    set(HAVE_X11 FALSE)
endif()

set(EXCLUDE_DEPRECATED_BEFORE_AND_AT 0 CACHE STRING "Control the range of deprecated API excluded from the build [default=0].")

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

ecm_set_disabled_deprecation_versions(
    QT 6.4
)

ecm_install_po_files_as_qm(poqm)

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF6DBusAddons")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KF6DBusAddons_QCH
        FILE KF6DBusAddonsQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KF6DBusAddonsQchTargets.cmake\")")
endif()

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/KF6DBusAddonsConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/KF6DBusAddonsConfig.cmake"
                              INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
                              )

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/KF6DBusAddonsConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/KF6DBusAddonsConfigVersion.cmake"
               "KF6DBusAddonsMacros.cmake"
               "KF6dbus.service.in"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel )

install(EXPORT KF6DBusAddonsTargets  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"  FILE KF6DBusAddonsTargets.cmake  NAMESPACE KF6:: )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdbusaddons_version.h  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KDBusAddons COMPONENT Devel)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
