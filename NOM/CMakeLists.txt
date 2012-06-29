# project info
PROJECT( NOM )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

# define target
SET (TARGET NOM)


# define sources

SET (LIBSBML_INCLUDE CACHE PATH "Full Path to LIBSBML Include Directory")
SET (LIBSBML_LIB CACHE PATH "Full Path to LIBSBML Import Libraries")
SET (LIBSBML_BIN CACHE PATH "Full Path to LIBSBML Binaries")



# dependencies
# it will expect a CMakeLists.txt in each folder
# ADD_SUBDIRECTORY( sbml )

# dependency headers
INCLUDE_DIRECTORIES(
    ${LIBSBML_INCLUDE}
)

# have linker point to libsbml
#link_directories(${LIBSBML_LIBRARY})
link_directories(${LIBSBML_LIB})

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



# set_property(TARGET ${TARGET}-static 
                # PROPERTY    COMPILE_DEFINITIONS 
                            # LIBSBML_STATIC
# ) 

if (WIN32)
	# link to dependencies
	
	TARGET_LINK_LIBRARIES( ${TARGET}
		${LIBSBML_LIB}/libcheck.lib
		${LIBSBML_LIB}/libexpat.lib
		${LIBSBML_LIB}/iconv.lib
		${LIBSBML_LIB}/libxml2.lib
		${LIBSBML_LIB}/libsbml-static.lib
		${LIBSBML_LIB}/xerces-c_static_3.lib
		${LIBSBML_LIB}/xerces-c_static_3D.lib
		${LIBSBML_LIB}/zdll.lib
		${LIBSBML_LIB}/bzip2.lib

	)
	set_property(TARGET ${TARGET} 
                PROPERTY    COMPILE_DEFINITIONS 
                            LIBSBML_STATIC
							LIBLAX_STATIC
                            ) 
	# gathering libsbml/NOM dependencies
	file(GLOB NOM_DEPENDENCIES 
	# "${LIBSBML_BIN}/*.dll
		${LIBSBML_BIN}/bzip2.dll
		${LIBSBML_BIN}/iconv.dll
		${LIBSBML_BIN}/libcheck.dll
		${LIBSBML_BIN}/libexpat.dll
		${LIBSBML_BIN}/libxml2.dll
		${LIBSBML_BIN}/msvcp100.dll
		${LIBSBML_BIN}/msvcr100.dll
		${LIBSBML_BIN}/MSVCRT.dll
		${LIBSBML_BIN}/xerces-c_3_1.dll
		${LIBSBML_BIN}/zlib1.dll
	)
	# build static library
	ADD_LIBRARY( ${TARGET}-static
		STATIC
		${NOM_SOURCE_FILES}
	)
	target_link_libraries(${TARGET}-static
		${LIBSBML_LIB}/libsbml-static.lib
	)
elseif (APPLE)	
	TARGET_LINK_LIBRARIES( NOM
		${LIBSBML_LIB}/libsbml.dylib
	)
else ()
	# link to dependencies
	TARGET_LINK_LIBRARIES( NOM
		${LIBSBML_LIB}/libsbml.so
	)
	# Installing libsbml/NOM dependencies
	# removed because it is generating broken links
	#file(GLOB NOM_DEPENDENCIES "${LIBSBML_BIN}/libsbml.so" "${LIBSBML_BIN}/libxml2.so")
endif ()



# link to dependencies
# TARGET_LINK_LIBRARIES( NOM
    # ${LIBSBML_LIB}/libsbml.lib
    # ${LIBSBML_LIB}/bzip2.lib
    # ${LIBSBML_LIB}/iconv.lib
    # ${LIBSBML_LIB}/libcheck.lib
    # ${LIBSBML_LIB}/libexpat.lib
    # ${LIBSBML_LIB}/libxml2.lib
    # ${LIBSBML_LIB}/xerces-c_3.lib
    # ${LIBSBML_LIB}/xerces-c_3D.lib
    # ${LIBSBML_LIB}/zdll.lib
# )

# install NOM
install(TARGETS NOM LIBRARY DESTINATION . RUNTIME DESTINATION . ARCHIVE DESTINATION .)
# Installing libsbml/NOM dependencies
install(FILES ${NOM_DEPENDENCIES} DESTINATION .)

