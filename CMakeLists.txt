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

INCLUDE(InstallRequiredSystemLibraries)

#---------------------CPACK---------------------
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Translate SBML files to MATLAB functions")
SET(CPACK_PACKAGE_NAME "sbml2matlab")
SET(CPACK_PACKAGE_VENDOR "Stanley Gu")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "2")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "sbml2matlab-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

INCLUDE(CPack)

option(BUILD_SHARED_LIBS  "Build shared libraries by default (static libraries still built)" ON)
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS are used), Debug, Release, RelWithDebInfo, MinSizeRel" )


####################################################################
#
# Here we have the main configuration options for sbml2matlab
#

# which language bindings should be build
option(WITH_SWIG   "Regenerate SWIG-based language bindings." ON )
option(WITH_PYTHON       "Generate Python language bindings." OFF)
OPTION(WITH_LIBSBML_EXPAT  "Set if libsbml was compiled with a separate expat library."  OFF)
OPTION(WITH_LIBSBML_LIBXML "Set if libsbml was compiled with a separate libxml library." ON)
OPTION(WITH_LIBSBML_XERCES "Set if libsbml was compiled with a separate xerces library." OFF)
OPTION(WITH_LIBSBML_COMPRESSION "Set if libsbml was compiled with separate zdll and bzip libraries." OFF)

set(EXTRA_LIBS "" CACHE STRING "Libraries the other libraries depend on that are in non-standard locations" )
set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${EXTRA_LIBS} )


SET(SBML2MATLAB_VERSION_STRING "v${SBML2MATLAB_VERSION_MAJOR}.${SBML2MATLAB_VERSION_MINOR}${SBML2MATLAB_VERSION_PATCH}${SBML2MATLAB_VERSION_RELEASE}")
add_definitions( -DSBML2MATLAB_VERSION_STRING="${SBML2MATLAB_VERSION_STRING}" )


########################################################
#
# Locate libsbml
#

find_path(LIBSBML_INCLUDE_DIR
        NAMES sbml/SBMLTypes.h
        PATHS /usr/include /usr/local/include 
              ${CMAKE_SOURCE_DIR}/include 
              ${CMAKE_SOURCE_DIR}/dependencies/include
              ${CMAKE_SOURCE_DIR}/../libsbml-5/release/include/
        )
INCLUDE_DIRECTORIES(${INCLUDE_DIRECTORIES} ${LIBSBML_INCLUDE_DIR})

find_library(LIBSBML_LIBRARY 
        NAMES libsbml.lib libsbml.so libsbml
        PATHS /usr/lib /usr/local/lib 
              ${CMAKE_SOURCE_DIR} 
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${CMAKE_SOURCE_DIR}/../libsbml-5/release/lib/
        )

set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${LIBSBML_LIBRARY} )


###############################################################
#
# Locate the libraries libsbml depends on, for static linking.
#

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
        NAMES libxml2.lib libxml2.so libxml2.dylib libxml2
	PATHS /usr/lib /usr/local/lib 
              ${CMAKE_SOURCE_DIR} 
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${LIBSBML_INCLUDE_DIR}/../lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${LIBXML_LIBRARY} )

    if(WIN32)
      find_library(ICONV_LIBRARY 
          NAMES iconv.lib iconv.so iconv.dylib iconv libiconv.lib libiconv.so libiconv.dylib libiconv
          PATHS /usr/lib /usr/local/lib 
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
    	    NAMES zdll.lib zdll.so zdll.dylib zdll
    	    PATHS /usr/lib /usr/local/lib 
    	          ${CMAKE_SOURCE_DIR} 
    	          ${CMAKE_SOURCE_DIR}/dependencies/lib
    	          ${LIBSBML_INCLUDE_DIR}/../lib
    	    )
    	set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${ZDLL_LIBRARY} )
    endif()

    find_library(BZIP_LIBRARY 
        NAMES bzip2.lib bzip2.so libbz2.so bzip2.dylib bzip2
	PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu  
              ${CMAKE_SOURCE_DIR} 
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${LIBSBML_INCLUDE_DIR}/../lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${BZIP_LIBRARY} )
endif()

###############################################################################
#
# If WITH_SWIG is selected, we need to find swig
#
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
    message(STATUS "  Using SWIG                    = ${SWIG_EXECUTABLE}")
    
    if (SWIG_FOUND AND SWIG_VERSION VERSION_LESS "2.0.4")
        message(WARNING "   You are using swig version ${SWIG_VERSION} while we recommend at least version 2.0.4.")
    endif()

endif()

# 
# Determine the python installation directory
#
if (WITH_PYTHON)
  set(PYTHON_PACKAGE_INSTALL_DIR)
  if (UNIX OR CYGWIN) 
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c "import sys;import platform; sys.stdout.write(platform.python_version()[:3])"
      OUTPUT_VARIABLE PYTHON_VERSION)
    set(PYTHON_PACKAGE_INSTALL_DIR lib/python${PYTHON_VERSION}/site-packages)
  else()
    set(PYTHON_PACKAGE_INSTALL_DIR ${MISC_PREFIX}bindings/python)
  endif()
endif()

if(WITH_PYTHON)
    message(STATUS "  Using Python                  = ${PYTHON_EXECUTABLE}")
endif()


# Language bindings
ADD_SUBDIRECTORY(bindings)

##New NOM stuff
set (NOM_SRC_DIR NOM/)

file(GLOB LIBNOM_SOURCES 
    ${NOM_SRC_DIR}NOM.cpp
    )
file(GLOB LIBNOM_HEADERS 
        ${NOM_SRC_DIR}NOM.h
        )

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

# Set up the include directories
INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}/NOM
    ${LIBSBML_INCLUDE_DIR}
)

##### Build the NOM library #####
set(LIBNOM_LIBRARY NOM)
add_library(${LIBNOM_LIBRARY} ${LIBNOM_HEADERS} ${LIBNOM_SOURCES})
#message(STATUS "  SBML2MATLAB_LIBS:  ${SBML2MATLAB_LIBS}")
target_link_libraries(${LIBNOM_LIBRARY} ${SBML2MATLAB_LIBS})
add_definitions(-DLIB_EXPORTS)
install(TARGETS ${LIBNOM_LIBRARY} DESTINATION lib)
install(FILES   ${LIBNOM_HEADERS} DESTINATION include)

if (NOT UNIX)
  add_definitions(-DWIN32 -DLIBSBML_EXPORTS -DLIBLAX_EXPORTS)
  if (WITH_PYTHON)
     INSTALL(TARGETS ${LIBNOM_LIBRARY} DESTINATION bindings/python/nom )
  endif()
endif(NOT UNIX)


##### Build the static library #####
add_library (${LIBNOM_LIBRARY}-static STATIC ${LIBNOM_SOURCES} )

if (WIN32 AND NOT CYGWIN)
    # don't decorate static library 
    set_target_properties(${LIBNOM_LIBRARY}-static PROPERTIES COMPILE_DEFINITIONS "LIBLAX_STATIC=1;LIBSBML_STATIC=1;LIBNOM_STATIC=1")
endif(WIN32 AND NOT CYGWIN)

target_link_libraries(${LIBNOM_LIBRARY}-static ${SBML2MATLAB_LIBS})

INSTALL(TARGETS ${LIBNOM_LIBRARY}-static
        DESTINATION lib
  )

##### Build the various sbml2matlab things #####

SET(SBML2MATLAB_SOURCE sbml2matlab.h uScanner.h sbml2matlab.cpp)

ADD_EXECUTABLE( sbml2matlab
	${PROJECT_SOURCE_DIR}/${SBML2MATLAB_SOURCE}
)
install(TARGETS sbml2matlab LIBRARY DESTINATION . RUNTIME DESTINATION . ARCHIVE DESTINATION .)	

ADD_LIBRARY( libsbml2matlab SHARED
	${PROJECT_SOURCE_DIR}/${SBML2MATLAB_SOURCE}
)

ADD_LIBRARY( libsbml2matlab-static STATIC
	${PROJECT_SOURCE_DIR}/${SBML2MATLAB_SOURCE}
)

install(TARGETS libsbml2matlab LIBRARY DESTINATION . RUNTIME DESTINATION . ARCHIVE DESTINATION .)	

install(FILES README.txt DESTINATION .)
install(FILES LICENSE.txt DESTINATION .)
install(FILES NOTICE.txt DESTINATION .)
install(FILES help.zip DESTINATION ./help)

if (NOT UNIX)
  if (WITH_PYTHON)
     message(STATUS "  Installing sbml2matlab in bindings/python/sbml2matlab")
     INSTALL(TARGETS libsbml2matlab DESTINATION ${PYTHON_PACKAGE_INSTALL_DIR})
  endif()
endif(NOT UNIX)
INSTALL(FILES NOTICE.txt DESTINATION ${PYTHON_PACKAGE_INSTALL_DIR})
INSTALL(FILES LICENSE.txt DESTINATION ${PYTHON_PACKAGE_INSTALL_DIR})
INSTALL(FILES README.txt DESTINATION ${PYTHON_PACKAGE_INSTALL_DIR})
INSTALL(FILES bindings/python/__init__.py DESTINATION ${PYTHON_PACKAGE_INSTALL_DIR})
INSTALL(FILES bindings/python/setup.py DESTINATION ${PYTHON_PACKAGE_INSTALL_DIR}/..)

# link to dependencies
# TARGET_LINK_LIBRARIES(sbml2matlab NOM)
TARGET_LINK_LIBRARIES(libsbml2matlab        NOM-static ${SBML2MATLAB_LIBS})
TARGET_LINK_LIBRARIES(libsbml2matlab-static NOM-static ${SBML2MATLAB_LIBS})
TARGET_LINK_LIBRARIES(sbml2matlab           NOM-static ${SBML2MATLAB_LIBS})
#message(STATUS "  SBML2MATLAB_LIBS:  ${SBML2MATLAB_LIBS}")

IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backslashes.
  # SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "sbml2matlab.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\code.google.com/p/snowburst")
  SET(CPACK_NSIS_CONTACT "stanleygu@gmail.com")
  SET(CPACK_NSIS_MODIFY_PATH ON)
  # don't decorate static library 
  set_target_properties(libsbml2matlab-static PROPERTIES COMPILE_DEFINITIONS "LIBLAX_STATIC=1;LIBSBML_STATIC=1;SBML2MATLAB_STATIC=1;NOM_STATIC=1")
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "bin/sbml2matlab")
  SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
SET(CPACK_PACKAGE_EXECUTABLES "sbml2matlab" "sbml2matlab")
SET(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\README.lnk'  '\$INSTDIR\\\\README.markdown'")
