# temporarily disable our own finders for module lookup
list(REMOVE_ITEM CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

if(${CMAKE_VERSION} VERSION_GREATER "3.10.0")
  set(HDF5_PREFER_PARALLEL true)
endif()

# call the built-in finder with the specifications set outside
find_package(HDF5)

# post-process found targets
if(HDF5_FOUND)
  if(NOT TARGET HDF5::HDF5)
    add_library(_hdf5_import INTERFACE)
    target_include_directories(_hdf5_import INTERFACE ${HDF5_INCLUDE_DIRS})
    target_link_libraries(_hdf5_import INTERFACE ${HDF5_LIBRARIES})

    add_library(HDF5::HDF5 ALIAS _hdf5_import)
  endif()

  if(NOT TARGET HDF5::HDF5_HL)
    add_library(_hdf5_import_hl INTERFACE)
    target_include_directories(_hdf5_import_hl INTERFACE ${HDF5_INCLUDE_DIRS})
    target_link_libraries(_hdf5_import_hl INTERFACE ${HDF5_HL_LIBRARIES})

    add_library(HDF5::HDF5_HL ALIAS _hdf5_import_hl)
  endif()

  message(STATUS "HDF5_IS_PARALLEL: ${HDF5_IS_PARALLEL}")
  message(STATUS "HDF5 include directory: ${HDF5_INCLUDE_DIRS}")
  message(STATUS "HDF5 libraries: ${HDF5_LIBRARIES}")
  message(STATUS "HDF5 HL libraries: ${HDF5_HL_LIBRARIES}")
  list(APPEND BACI_ALL_ENABLED_EXTERNAL_LIBS HDF5::HDF5 HDF5::HDF5_HL)
endif()

# re-enable our own finders
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
