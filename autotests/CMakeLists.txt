
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Qt5Test ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)

include(ECMAddTests)

ecm_add_tests(
    kdbusinterprocesslocktest.cpp
    kdbusservicetest.cpp
    LINK_LIBRARIES Qt5::Test KF5::DBusAddons
)

if (PythonModuleGeneration_FOUND AND EXISTS ${CMAKE_BINARY_DIR}/prefix/lib/python2.7/dist-packages)
  add_test(NAME Py2${name} COMMAND python2 ${pythontest} ${CMAKE_BINARY_DIR}/prefix/lib/python2.7/dist-packages)
endif()

if (PythonModuleGeneration_FOUND AND EXISTS ${CMAKE_BINARY_DIR}/prefix/lib/python3.5/dist-packages)
  add_test(NAME Py3${name} COMMAND python3 ${pythontest} ${CMAKE_BINARY_DIR}/prefix/lib/python3.5/dist-packages)
endif()
