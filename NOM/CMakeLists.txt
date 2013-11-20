# project info
PROJECT( NOM )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

# define target
SET (TARGET NOM)

####################################################################
#
# Locate libsbml
#

find_path(LIBSBML_INCLUDE_DIR "The 'include' directory for libsbml"
        NAMES sbml/SBMLTypes.h
        PATHS /usr/include /usr/local/include 
              ${CMAKE_SOURCE_DIR}/include 
              ${CMAKE_SOURCE_DIR}/dependencies/include
        )

if(LIBSBML_STATIC)
    find_library(LIBSBML_STATIC_LIBRARY 
        NAMES libsbml-static.lib libsbml-static.so libsbml-static.dylib libsbml-static
        PATHS /usr/lib /usr/local/lib 
              ${CMAKE_SOURCE_DIR} 
              ${CMAKE_SOURCE_DIR}/dependencies/lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${LIBSBML_STATIC_LIBRARY} )
else()
    find_library(LIBSBML_LIBRARY 
        NAMES libsbml.lib libsbml.so libsbml.dylib libsbml
        PATHS /usr/lib /usr/local/lib 
              ${CMAKE_SOURCE_DIR} 
              ${CMAKE_SOURCE_DIR}/dependencies/lib
        )
   set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${LIBSBML_LIBRARY} )
endif()

if(WIN32)
  #Find the libsbml dll so we can install it with our program.
  find_path(LIBSBML_BIN_DIR
        NAMES libsbml.dll
        PATHS /usr/bin /usr/local/bin
              ${LIBSBML_INCLUDE_DIR}/../bin
              ${CMAKE_SOURCE_DIR}/ 
              ${CMAKE_SOURCE_DIR}/dependencies/libsbml/bin
              ${CMAKE_SOURCE_DIR}/../libsbml-5/release/bin/
        )
        set(LIBSBML_DLL ${LIBSBML_BIN_DIR}/libsbml.dll)
        install(FILES   ${LIBSBML_DLL} DESTINATION .)
endif()


# dependency headers
INCLUDE_DIRECTORIES(
    ${INCLUDE_DIRECTORIES}
    ${LIBSBML_INCLUDE_DIR}
)

# have linker point to libsbml
link_directories(${LIBSBML_LIBRARY})
#link_directories(${LIBSBML_LIB})

# source files
FILE( GLOB NOM_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/NOM.h
    ${PROJECT_SOURCE_DIR}/NOM.cpp
    # you can list each file in here instead if you want
)

# build shared library
ADD_LIBRARY( ${TARGET}
    SHARED
    ${NOM_SOURCE_FILES}
)



TARGET_LINK_LIBRARIES(${TARGET} ${SBML2MATLAB_LIBS} )

#We can afford to look for everything here, since anything not found will simply be ignored.
if (WIN32)
    # gathering libsbml/NOM dependencies
    file(GLOB NOM_DEPENDENCIES 
        # "${LIBSBML_BIN_DIR}/*.dll
        ${LIBSBML_BIN_DIR}/libsbml.dll
        ${LIBSBML_BIN_DIR}/bzip2.dll
        ${LIBSBML_BIN_DIR}/iconv.dll
        ${LIBSBML_BIN_DIR}/libcheck.dll
        ${LIBSBML_BIN_DIR}/libexpat.dll
        ${LIBSBML_BIN_DIR}/libxml2.dll
        ${LIBSBML_BIN_DIR}/msvcp100.dll
        ${LIBSBML_BIN_DIR}/msvcr100.dll
        ${LIBSBML_BIN_DIR}/MSVCRT.dll
        ${LIBSBML_BIN_DIR}/xerces-c_3_1.dll
        ${LIBSBML_BIN_DIR}/zlib1.dll
    )
endif ()

# build static library
ADD_LIBRARY( ${TARGET}-static
    STATIC
    ${NOM_SOURCE_FILES}
)

# install NOM
install(TARGETS NOM LIBRARY DESTINATION . RUNTIME DESTINATION . ARCHIVE DESTINATION .)
# Installing libsbml/NOM dependencies
#message(STATUS "  NOM_DEPENDENCIES found:  ${NOM_DEPENDENCIES}")
#message(STATUS "  LIBSBML_BIN_DIR:  ${LIBSBML_BIN_DIR}")
install(FILES ${NOM_DEPENDENCIES} DESTINATION .)

if (WITH_PYTHON)
  install(TARGETS NOM DESTINATION bindings/python/sbml2matlab )
  install(FILES ${NOM_DEPENDENCIES} DESTINATION bindings/python/sbml2matlab )
endif()

