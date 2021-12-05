
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Qt${QT_MAJOR_VERSION}Test ${REQUIRED_QT_VERSION} REQUIRED CONFIG)

include(ECMAddTests)

if(UNIX)
    add_executable(kdbussimpleservice kdbussimpleservice.cpp)
    target_link_libraries(kdbussimpleservice Qt${QT_MAJOR_VERSION}::Core KF5::DBusAddons)

    ecm_add_tests(
        deadservicetest.cpp
        LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF5::DBusAddons
    )

    add_dependencies(deadservicetest kdbussimpleservice)
endif()

ecm_add_tests(
    kdbusinterprocesslocktest.cpp
    kdbusservicetest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF5::DBusAddons
)
