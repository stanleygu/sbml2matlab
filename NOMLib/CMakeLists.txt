# project info
PROJECT( NOM )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

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

# build library
# i think this would build libnom.dll, not nomLIB.dll
ADD_LIBRARY( NOM
    SHARED
    ${NOM_SOURCE_FILES}
)

# link to dependencies
TARGET_LINK_LIBRARIES( NOM
    ${LIBSBML_LIB}/libsbml.lib
)

# install NOM
install(TARGETS NOM 
	RUNTIME DESTINATION .
)

# Installing libsbml/NOM dependencies
file(GLOB NOM_DEPENDENCIES "${LIBSBML_BIN}/libsbml.dll")
install(FILES ${NOM_DEPENDENCIES} DESTINATION .)

