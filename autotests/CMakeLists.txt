
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Qt5Test ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)

include(ECMAddTests)

if(UNIX)
    add_executable(kdbussimpleservice kdbussimpleservice.cpp)
    target_link_libraries(kdbussimpleservice Qt5::Core KF5::DBusAddons)

    ecm_add_tests(
        deadservicetest.cpp
        LINK_LIBRARIES Qt5::Test KF5::DBusAddons
    )

    add_dependencies(deadservicetest kdbussimpleservice)
endif()

ecm_add_tests(
    kdbusinterprocesslocktest.cpp
    kdbusservicetest.cpp
    LINK_LIBRARIES Qt5::Test KF5::DBusAddons
)
