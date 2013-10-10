# project info
PROJECT( sbml2matlab )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

# Set build type default.
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
    "Choose the build type. The options are: None (CMAKE_CXX_FLAGS or
     CMAKE_C_FLAGS are used), Debug, Release, RelWithDebInfo, MinSizeRel.")
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
  "Choose the build type. The options are: None (CMAKE_CXX_FLAGS or
  CMAKE_C_FLAGS are used), Debug, Release, RelWithDebInfo, MinSizeRel." FORCE)
endif()

set(EXTRA_LIBS "" CACHE STRING "Semicolon separatred list of additional libraries to link against." )

OPTION(WITH_LIBSBML_EXPAT  "Set if libsbml was compiled with a separate expat library." OFF)
OPTION(WITH_LIBSBML_LIBXML "Set if libsbml was compiled with a separate libxml library." OFF)
OPTION(WITH_LIBSBML_XERCES "Set if libsbml was compiled with a separate xerces library." OFF)
OPTION(WITH_LIBSBML_COMPRESSION "Set if libsbml was compiled with separate zdll and bzip libraries." OFF)
OPTION(LIBSBML_STATIC "Whether to use the static libsbml library" ON)


if(WITH_LIBSBML_EXPAT)
    find_library(EXPAT_LIBRARY 
        NAMES libexpat.lib libexpat.so libexpat.dylib libexpat
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

    find_library(ICONV_LIBRARY 
        NAMES iconv.lib iconv.so iconv.dylib iconv
	PATHS /usr/lib /usr/local/lib 
              ${CMAKE_SOURCE_DIR} 
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${LIBSBML_INCLUDE_DIR}/../lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${ICONV_LIBRARY} )
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
    find_library(ZDLL_LIBRARY 
        NAMES zdll.lib zdll.so zdll.dylib zdll
	PATHS /usr/lib /usr/local/lib 
              ${CMAKE_SOURCE_DIR} 
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${LIBSBML_INCLUDE_DIR}/../lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${ZDLL_LIBRARY} )

    find_library(BZIP_LIBRARY 
        NAMES bzip2.lib bzip2.so bzip2.dylib bzip2
	PATHS /usr/lib /usr/local/lib 
              ${CMAKE_SOURCE_DIR} 
              ${CMAKE_SOURCE_DIR}/dependencies/lib
              ${LIBSBML_INCLUDE_DIR}/../lib
        )
    set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${BZIP_LIBRARY} )
endif()

set(SBML2MATLAB_LIBS ${SBML2MATLAB_LIBS} ${EXTRA_LIBS} )

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

# The following line can make the modified version show up in CMake, but weird things happen with multiple compiles, if so.
#    set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING "Flags used by the compiler during all build types (Forced version)." FORCE )
endif()

# dependencies
# it will expect a CMakeLists.txt in each folder
ADD_SUBDIRECTORY(NOM)

# dependency headers
INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}/NOM
    ${LIBSBML_INCLUDE_DIR}
)

link_directories(${LIBSBML_LIB})

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

install(FILES README.markdown DESTINATION .)
install(FILES LICENSE.txt DESTINATION .)
install(FILES help.zip DESTINATION ./help)

# link to dependencies
TARGET_LINK_LIBRARIES(sbml2matlab NOM)
TARGET_LINK_LIBRARIES(libsbml2matlab NOM)
# TARGET_LINK_LIBRARIES(sbml2matlab NOM-static)
# TARGET_LINK_LIBRARIES(libsbml2matlab-static NOM-static)

#---------------------CPACK---------------------
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Translate SBML files to MATLAB functions")
SET(CPACK_PACKAGE_VENDOR "Stanley Gu")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.markdown")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "2")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "sbml2matlab-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backslashes.
  # SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "sbml2matlab.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\code.google.com/p/snowburst")
  SET(CPACK_NSIS_CONTACT "stanleygu@gmail.com")
  SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "bin/sbml2matlab")
  SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
SET(CPACK_PACKAGE_EXECUTABLES "sbml2matlab" "sbml2matlab")
SET(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\README.lnk'  '\$INSTDIR\\\\README.markdown'")
INCLUDE(CPack)
