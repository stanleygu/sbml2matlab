####################################################################
#
# CMake Build Script for sbml2matlab
#

cmake_minimum_required(VERSION 2.8)
project(sbml2matlab)

####################################################################
####################################################################
#
# These lines configure the parameters for packaging the binaries
# they can be invoked with: make package / nmake package or by using
# cpack -G zip|deb|rpm|dmg|nsis
#

include(InstallRequiredSystemLibraries)

#---------------------CPACK---------------------
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Translate SBML files to MATLAB functions")
set(CPACK_PACKAGE_NAME "sbml2matlab")
set(CPACK_PACKAGE_VENDOR "Stanley Gu")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_install_DIRECTORY "sbml2matlab-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

set(LIBSBML_PREFIX "" CACHE PATH "Path to where libSBML is installed")

include(CPack)

option(BUILD_SHARED_LIBS  "Build shared libraries by default (static libraries still built)" ON)
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS are used), Debug, Release, RelWithDebInfo, MinSizeRel" )


####################################################################
#
# Here we have the main configuration options for sbml2matlab
#

# which language bindings should be build
option(WITH_SWIG   "Regenerate SWIG-based language bindings." ON )
option(WITH_PYTHON       "Generate Python language bindings." OFF)
option(WITH_LIBSBML_EXPAT  "Set if libsbml was compiled with a separate expat library."  OFF)
option(WITH_LIBSBML_LIBXML "Set if libsbml was compiled with a separate libxml library." ON)
option(WITH_LIBSBML_XERCES "Set if libsbml was compiled with a separate xerces library." OFF)
option(WITH_LIBSBML_COMPRESSION "Set if libsbml was compiled with separate zdll and bzip libraries." OFF)

set(EXTRA_LIBS "" CACHE STRING "Libraries the other libraries depend on that are in non-standard locations" )
set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${EXTRA_LIBS} )


set(SBML2MATLAB_VERSION_STRING "v${SBML2MATLAB_VERSION_MAJOR}.${SBML2MATLAB_VERSION_MINOR}${SBML2MATLAB_VERSION_PATCH}${SBML2MATLAB_VERSION_RELEASE}")
add_definitions( -DSBML2MATLAB_VERSION_STRING="${SBML2MATLAB_VERSION_STRING}" )


########################################################

# Locate libsbml
find_path(LIBSBML_INCLUDE_DIR
        NAMES sbml/SBMLTypes.h
        PATHS ${LIBSBML_PREFIX}/include
              /usr/include /usr/local/include
              ${CMAKE_SOURCE_DIR}/include
              ${CMAKE_SOURCE_DIR}/dependencies/include
              ${CMAKE_SOURCE_DIR}/../libsbml-5/release/include/
        )
include_directories(${INCLUDE_DIRECTORIES} ${LIBSBML_INCLUDE_DIR})

find_library(LIBSBML_LIBRARY
        NAMES sbml libsbml.lib libsbml.so libsbml
        PATHS ${LIBSBML_PREFIX}/lib
              /usr/lib /usr/local/lib
              ${CMAKE_SOURCE_DIR}
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${CMAKE_SOURCE_DIR}/../libsbml-5/release/lib/
        )

find_library(LIBSBML_STATIC_LIBRARY
        NAMES sbml-static
        PATHS ${LIBSBML_PREFIX}/lib
              /usr/lib /usr/local/lib
              ${CMAKE_SOURCE_DIR}
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${CMAKE_SOURCE_DIR}/../libsbml-5/release/lib/
        )

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${LIBSBML_STATIC_LIBRARY})
else()
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${LIBSBML_STATIC_LIBRARY} z bz2 )
endif()

###############################################################

# Locate the libraries libsbml depends on, for static linking.
if(WITH_LIBSBML_EXPAT)
    find_library(EXPAT_LIBRARY
        NAMES libexpat.lib libexpat.so libexpat.dylib libexpat expat.lib expat.so expat.dylib expat
	PATHS /usr/lib /usr/local/lib
              ${CMAKE_SOURCE_DIR}
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${LIBSBML_INCLUDE_DIR}/../lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${EXPAT_LIBRARY} )
endif()

if(WITH_LIBSBML_LIBXML)
    find_library(LIBXML_LIBRARY
        NAMES xml2 libxml2.lib libxml2.so libxml2.dylib libxml2
              PATHS
              /usr/lib /usr/local/lib
              ${CMAKE_SOURCE_DIR}
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${LIBSBML_INCLUDE_DIR}/../lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${LIBXML_LIBRARY} )

    if(WIN32)
      find_library(ICONV_LIBRARY
          NAMES iconv iconv.lib iconv.so iconv.dylib iconv libiconv.lib libiconv.so libiconv.dylib libiconv
          PATHS
                /usr/lib /usr/local/lib
                ${CMAKE_SOURCE_DIR}
                ${CMAKE_SOURCE_DIR}/dependencies/lib
                ${LIBSBML_INCLUDE_DIR}/../lib
          )
      set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${ICONV_LIBRARY} )
    endif()
endif()

if(WITH_LIBSBML_XERCES)
    find_library(XERCES_LIBRARY
        NAMES xerces-c_static_3.lib xerces-c_static_3.so xerces-c_static_3.dylib xerces-c_static_3
	PATHS /usr/lib /usr/local/lib
              ${CMAKE_SOURCE_DIR}
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${LIBSBML_INCLUDE_DIR}/../lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${XERCES_LIBRARY} )
endif()

if(WITH_LIBSBML_COMPRESSION)
    if(WIN32)
    	find_library(ZDLL_LIBRARY
    	    NAMES z zdll.lib zdll.so zdll.dylib zdll
    	    PATHS /usr/lib /usr/local/lib
    	          ${CMAKE_SOURCE_DIR}
    	          ${CMAKE_SOURCE_DIR}/dependencies/lib
    	          ${LIBSBML_INCLUDE_DIR}/../lib
    	    )
    	set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${ZDLL_LIBRARY} )
    endif()

    find_library(BZIP_LIBRARY
        NAMES bzip2 bzip2.lib bzip2.so libbz2.so bzip2.dylib bzip2
	PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
              ${CMAKE_SOURCE_DIR}
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${LIBSBML_INCLUDE_DIR}/../lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${BZIP_LIBRARY} )
endif()

###############################################################################

# If WITH_SWIG is selected, we need to find swig
if(WITH_SWIG)
    find_program(SWIG_EXECUTABLE
        NAMES swig
        PATHS
              "c:/Program Files (x86)/Swig"
              c:/Program Files/Swig
              c:/swigwin-2.0.11
              c:/swigwin-2.0.10
              c:/swigwin-2.0.9
              c:/swigwin-2.0.8
              c:/swigwin-2.0.7
              c:/swigwin-2.0.6
              c:/swigwin-2.0.5
              c:/swigwin-2.0.4
              c:/swigwin-2.0.3
              c:/swigwin-2.0.2
              c:/swigwin-2.0.1
              c:/swigwin-2.0.0
              /usr/local/bin
              /opt/local/bin
              /usr/bin
        DOC "The file name of the swig executable."
        )
    find_package(SWIG)
    set(SWIG_EXTRA_ARGS "" CACHE STRING "Extra arguments to SWIG" )
endif(WITH_SWIG)

if(WITH_SWIG)
    if (SWIG_FOUND AND SWIG_VERSION VERSION_LESS "2.0.4")
        message(WARNING "   You are using swig version ${SWIG_VERSION} while we recommend at least version 2.0.4.")
    endif()

endif()

# Determine the python installation directory
if (WITH_PYTHON)
  set(PYTHON_PACKAGE_install_DIR)
  if (UNIX OR CYGWIN)
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c "import sys;import platform; sys.stdout.write(platform.python_version()[:3])"
      OUTPUT_VARIABLE PYTHON_VERSION)
    set(PYTHON_PACKAGE_install_DIR lib/python${PYTHON_VERSION}/site-packages)
  else()
    set(PYTHON_PACKAGE_install_DIR ${MISC_PREFIX}bindings/python)
  endif()
endif()

# Language bindings
add_subdirectory(bindings)

if(WIN32 AND NOT UNIX)
    #Add this flag to suppress 'sprintf' vs. 'sprintf_s' and 'strcpy' vs. 'strcpy_s' warnings in VS.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
    #We throw C++ exceptions from our 'C' functions; change the Exception Handler (EH) flag
    string(REGEX
       REPLACE
       "/EHsc"
       "/EHs"
       CMAKE_CXX_FLAGS
       "${CMAKE_CXX_FLAGS}"
    )

    string(REGEX
       REPLACE
       "/EHc"
       "/EHs"
       CMAKE_CXX_FLAGS
       "${CMAKE_CXX_FLAGS}"
    )
endif()

message(STATUS "
-------------------------------------------------------------------------------
sbml2matlab Configuration Summary
-------------------------------------------------------------------------------

    https://github.com/sys-bio/sbml2matlab

    Configured on host $ENV{COMPUTERNAME} ${HOSTNAME}
      host OS                   ${CMAKE_SYSTEM_NAME}
      host architecture         ${CMAKE_SYSTEM_PROCESSOR}

    General flags:
      CC                        ${CMAKE_C_COMPILER}
      CXX                       ${CMAKE_CXX_COMPILER}
      CPPFLAGS                  ${BUILD_DEFINITIONS}
      CFLAGS                    ${CMAKE_C_FLAGS}
      CXXFLAGS                  ${CMAKE_CXX_FLAGS}
      LDFLAGS                   ${CMAKE_EXE_LINKER_FLAGS}

    Installation prefix:        ${CMAKE_INSTALL_PREFIX}

    Options:
      Full RPATH                ${ENABLE_FULL_RPATH}
      Enable unit tests         ${WITH_TESTS}

    Packages marked with *** are missing required dependencies:
    "
)

if(LIBSBML_LIBRARY)
    message( "    * libsbml libs                ${LIBSBML_LIBRARY}")
    message( "    * libsbml includes            ${LIBSBML_INCLUDE_DIR}")
else()
    message( "  *** libsbml                     *N/A")
endif()

if(WITH_SWIG)
    message( "    * swig                        ${SWIG_EXECUTABLE}")
else()
    message( "    * swig                        *N/A")
endif()

if(WITH_PYTHON)
    message( "    * Python                      ${PYTHON_EXECUTABLE}")
else()
    message( "    * Python                      *N/A")
endif()

# Set up the include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/NOM
    ${LIBSBML_INCLUDE_DIR}
)

# NOM sources
set (NOM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NOM)

file(GLOB LIBNOM_SOURCES
    ${NOM_SRC_DIR}/NOM.cpp
    )
file(GLOB LIBNOM_HEADERS
    ${NOM_SRC_DIR}/NOM.h
    )

# Build the NOM library
add_library(NOM ${LIBNOM_HEADERS} ${LIBNOM_SOURCES})
target_link_libraries(NOM ${SBML2MATLAB_LIBS})
add_definitions(-DLIB_EXPORTS)
install(TARGETS NOM DESTINATION lib)
install(FILES   ${LIBNOM_HEADERS} DESTINATION include)

if (NOT UNIX)
  add_definitions(-DWIN32 -DLIBSBML_EXPORTS -DLIBLAX_EXPORTS)
  if (WITH_PYTHON)
     install(TARGETS NOM DESTINATION bindings/python/nom )
  endif()
endif(NOT UNIX)


# Build the NOM static library
add_library (NOM-static STATIC ${LIBNOM_SOURCES} )

if (WIN32 AND NOT CYGWIN)
    # don't decorate static library
    set_target_properties(NOM-static PROPERTIES COMPILE_DEFINITIONS "LIBLAX_STATIC=1;LIBSBML_STATIC=1;LIBNOM_STATIC=1")
endif()
if(UNIX)
  set_target_properties(NOM-static PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

target_link_libraries(NOM-static ${SBML2MATLAB_LIBS})

install(TARGETS NOM-static
        DESTINATION lib
  )

# Build sbml2matlab

set(SBML2MATLAB_SOURCE sbml2matlab.h uScanner.h sbml2matlab.cpp)

add_executable( sbml2matlab
  ${SBML2MATLAB_SOURCE}
)
install(TARGETS sbml2matlab DESTINATION bin )


add_library( libsbml2matlab SHARED
  ${SBML2MATLAB_SOURCE}
)
set_target_properties( libsbml2matlab PROPERTIES PREFIX "" )

add_library( libsbml2matlab-static STATIC
  ${SBML2MATLAB_SOURCE}
)
set_target_properties( libsbml2matlab-static PROPERTIES PREFIX "" )

if(UNIX)
  set_target_properties(libsbml2matlab-static PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

install( TARGETS libsbml2matlab LIBRARY DESTINATION lib RUNTIME DESTINATION bin ARCHIVE DESTINATION lib )

install(FILES README.txt DESTINATION .)
install(FILES LICENSE.txt DESTINATION .)
install(FILES NOTICE.txt DESTINATION .)
install(FILES help.zip DESTINATION ./help)

if (NOT UNIX)
  if (WITH_PYTHON)
    message(STATUS "  Installing sbml2matlab in bindings/python/sbml2matlab")
    install(TARGETS libsbml2matlab DESTINATION ${PYTHON_PACKAGE_install_DIR})
    install(FILES NOTICE.txt DESTINATION ${PYTHON_PACKAGE_install_DIR})
    install(FILES LICENSE.txt DESTINATION ${PYTHON_PACKAGE_install_DIR})
    install(FILES README.txt DESTINATION ${PYTHON_PACKAGE_install_DIR})
    install(FILES bindings/python/__init__.py DESTINATION ${PYTHON_PACKAGE_install_DIR})
    install(FILES bindings/python/setup.py DESTINATION ${PYTHON_PACKAGE_install_DIR}/..)
  endif()
endif(NOT UNIX)

# link to dependencies
# target_link_libraries(sbml2matlab NOM)
target_link_libraries(libsbml2matlab        NOM-static ${SBML2MATLAB_LIBS})
target_link_libraries(libsbml2matlab-static NOM-static ${SBML2MATLAB_LIBS})
target_link_libraries(sbml2matlab           NOM-static ${SBML2MATLAB_LIBS})
#message(STATUS "  SBML2MATLAB_LIBS:  ${SBML2MATLAB_LIBS}")

IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backslashes.
  # set(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
  set(CPACK_NSIS_installED_ICON_NAME "sbml2matlab.exe")
  set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_install_DIRECTORY}")
  set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\code.google.com/p/snowburst")
  set(CPACK_NSIS_CONTACT "stanleygu@gmail.com")
  set(CPACK_NSIS_MODIFY_PATH ON)
  # don't decorate static library
  set_target_properties(libsbml2matlab-static PROPERTIES COMPILE_DEFINITIONS "LIBLAX_STATIC=1;LIBSBML_STATIC=1;SBML2MATLAB_STATIC=1;NOM_STATIC=1")
ELSE(WIN32 AND NOT UNIX)
  set(CPACK_STRIP_FILES "bin/sbml2matlab")
  set(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
set(CPACK_PACKAGE_EXECUTABLES "sbml2matlab" "sbml2matlab")
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\README.lnk'  '\$INSTDIR\\\\README.markdown'")
