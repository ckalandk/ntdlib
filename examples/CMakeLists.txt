add_executable(namedthread_demo namedthread-demo.cpp)
add_executable(sync_queue_demo sync_queue_demo.cpp)
target_link_libraries(namedthread_demo PRIVATE ntd::ntd)
target_link_libraries(sync_queue_demo PRIVATE ntd::ntd)
