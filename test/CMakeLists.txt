set(MODULE_NAME test-input-event)

add_executable(${MODULE_NAME} TestInputEvent.cpp)
target_link_libraries(${MODULE_NAME} boost_unit_test_framework catch turtle input-event pthread)
add_test(${MODULE_NAME} ${MODULE_NAME})
