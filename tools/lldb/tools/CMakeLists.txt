if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
# XXX Nyuzi: disable debugserver because this doesn't run on target
#  add_subdirectory(debugserver)
#  add_subdirectory(darwin-debug)
endif()
  add_subdirectory(argdumper)
  add_subdirectory(driver)
if (NOT __ANDROID_NDK__)
  add_subdirectory(lldb-mi)
endif()
if (LLDB_CAN_USE_LLDB_SERVER)
  add_subdirectory(lldb-server)
endif()
