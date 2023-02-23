idf_component_register()

include(ExternalProject)

set(ACCELSTEPPER_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/accelstepper_install)
externalproject_add(accelstepper_proj
    URL http://www.airspayce.com/mikem/arduino/AccelStepper/AccelStepper-1.64.zip

    USES_TERMINAL_DOWNLOAD TRUE
    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD TRUE

    INSTALL_DIR ${ACCELSTEPPER_INSTALL_DIR}
    BUILD_BYPRODUCTS "${ACCELSTEPPER_INSTALL_DIR}/lib/libaccelstepper.a"
)

add_prebuilt_library(accelstepper_lib "${ACCELSTEPPER_INSTALL_DIR}/lib/libaccelstepper.a"
                     PRIV_REQUIRES cxx)
target_include_directories(accelstepper_lib INTERFACE "${ACCELSTEPPER_INSTALL_DIR}/include")
add_dependencies(accelstepper_lib accelstepper_proj)

target_link_libraries(${COMPONENT_LIB} INTERFACE accelstepper_lib)