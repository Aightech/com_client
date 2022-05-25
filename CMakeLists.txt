cmake_minimum_required(VERSION 3.3)
###### Author: Alexis Devillard
###### Date:   27/04/2022
###### Brief: This CmakeLists.txt file enable to build generic project with the following structure:
######                                +---------+
######                                | project |
######                                +----+----+
######                                     |
######            +----------+------------++-------------+-----------+
######            |          |            |              |           |
######         +--+--+  +----+----+ +-----+------+ +-----+------+ +--+--+
######         | src |  | include | | CMakelists | | CMakefunct | | lib |
######         +--+--+  +----+----+ |    .txt    | |  ions.txt  | +--+--+
######            |          |      +------------+ +------------+    |
######      +-----+------+   |                                       |
######      |            |   +-------+            +--------------+---+
###### +----+-----+      |           |            |              |
###### | main.cpp | +----+----+ +----+----+ +-----+------+ +-----+------+
###### +----------+ | lib.cpp | | lib.hpp | | subproject | | subproject |
######              +---------+ +---------+ |     1      | |     2      |
######                                      +-----+------+ +------------+
######                                            |
######                        +-----------+-------+------+-----------+
######                        |           |              |           |
######                     +--+---+ +-----+------+ +-----+------+ +--+--+
######                     | .git | | CMakelists | | CMakeFunct | | ... |
######                     +------+ |    .txt    | |  ions.txt  | +-----+
######                              +------------+ +------------+
###### 
######

###### PROJECT NAME AND VERSION ######
set(EXEC_NAME com_client)
set(PROJECT_VERSION 1.19)
project(${EXEC_NAME} VERSION ${PROJECT_VERSION})

###### LIBRARY NAME ######
set(LIB_NAME ${EXEC_NAME}.${PROJECT_VERSION})

if(NOT DEFINED DIR_DEPTH)
  set(DIR_DEPTH 0)
else ()
  MATH(EXPR DIR_DEPTH "${DIR_DEPTH}+1")
endif()
string(REPEAT "\t" ${DIR_DEPTH} PREFIX_MSG)
MATH(EXPR color_code "${DIR_DEPTH}+33")
MATH(EXPR color_code2 "${DIR_DEPTH}+34")

###### include helper functions ######
include(CMakeFunctions.txt)

if (TARGET ${EXEC_NAME})
  # Do something when target found
  message("${PREFIX_MSG}-- ${ColourBold}${Esc}[${color_code}m${EXEC_NAME}${ColourReset} is already built")
elseif (TARGET ${LIB_NAME})
  message("${PREFIX_MSG}-- ${ColourBold}${Esc}[${color_code}m${LIB_NAME}${ColourReset} is already built")
else ()

  ###### C++ standard ######
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
  
  set(EXTRA_LIBS "")
  ###### Special libraries ######
  if(0)
    set(EXTRA_LIBS "pthread" "util")
  endif()

  ###### Boost libraries ######
  if(0)
    find_package(Boost 1.56 REQUIRED COMPONENTS system filesystem iostreams)
    if (Boost_FOUND)
      list(APPEND EXTRA_LIBS ${Boost_IOSTREAMS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})
    else ()
      message("Couldn't find Boost library")
    endif (Boost_FOUND)
  endif()

  ###### SFML libraries ######
  if(0)
    find_package(SFML REQUIRED COMPONENTS graphics window system)
    if (SFML_FOUND)
      list(APPEND EXTRA_LIBS "sfml-graphics" "sfml-window" "sfml-system")
    else ()
      message("Couldn't find SFML library")
    endif (SFML_FOUND)
  endif()

  ###### OpenCV libraries ######
  if(0)
    find_package(OpenCV REQUIRED)
    if (OpenCV_FOUND)
      list(APPEND EXTRA_LIBS ${OpenCV_LIBS})
      list(APPEND EXTRA_INCLUDES ${OpenCV_INCLUDE_DIRS})
      message("-- Opencv library Found")
    else ()
      message("Couldn't find Opencv library")
    endif (OpenCV_FOUND)
  endif()

  ###### Get the sources and headers ######
  ###### Exec main src file ######
  set(MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
  ###### Exec-only header file ######
  set(EXTRA_HEADERS "include/gnuplot-iostream.h")

  ###### Library src and headezr files (everything else) ######
  file(GLOB LIB_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "src/*.c")
  list(REMOVE_ITEM LIB_SRCS ${MAIN_FILE})
  file(GLOB LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "include/*.hpp")
  list(REMOVE_ITEM LIB_HEADERS ${EXTRA_HEADERS})


  ###### check for internet ######
  if(NOT DEFINED NO_CONNECTION)
    execute_process(COMMAND printf "-- Check internet connection: ${ColourSlowBlink}${Yellow}...${ColourReset}\\b\\b\\b")
    execute_process(
      COMMAND ping www.google.com -c 2 -w 1 
      ERROR_QUIET
      OUTPUT_QUIET
      RESULT_VARIABLE NO_CONNECTION
      )
    if(NO_CONNECTION EQUAL 1)
      message(" ${Green}Online${ColourReset}")
    else()
      message(" ${Red}Offline${ColourReset}")
    endif()
  endif()

  


  ###### Search for Subdirectories/Libraries ######
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib")
    execute_process(COMMAND git submodule status
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE SUBMODULES_LIST)
    subdirlist(LIBS "${CMAKE_CURRENT_SOURCE_DIR}/lib/")
    foreach(subdir ${LIBS})
      string(FIND "${SUBMODULES_LIST}" "${subdir}" FOUND)
      if(${FOUND} GREATER -1 AND NO_CONNECTION EQUAL 1 )
	#git submodules init
	execute_process(COMMAND git submodule update --remote --merge --init -- "lib/${subdir}"
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE OUTPUT)
      endif()
      subproject_version("lib/${subdir}" subdir_version)#get the version
      message("${PREFIX_MSG}-- ${ColourBold}${Esc}[${color_code}m${EXEC_NAME}${ColourReset}: Adding library ${ColourBold}${Esc}[${color_code2}m${subdir}${ColourReset} version: ${ColourBold}${subdir_version}${ColourReset}")
      add_subdirectory("lib/${subdir}")
      
      get_directory_property(libname
	DIRECTORY "lib/${subdir}"
	DEFINITION LIB_NAME)
      list(APPEND EXTRA_LIBS "${libname}")
      link_directories(${CMAKE_SOURCE_DIR}/lib/${subdir}/bin)    
    endforeach()
  endif()

  ###### Search for Subdirectories/Libraries ######
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tool_lib")
    execute_process(COMMAND git submodule status
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE SUBMODULES_LIST)
    subdirlist(LIBS "${CMAKE_CURRENT_SOURCE_DIR}/tool_lib/")
    foreach(subdir ${LIBS})
      string(FIND "${SUBMODULES_LIST}" "${subdir}" FOUND)
      if(${FOUND} GREATER -1 AND NO_CONNECTION EQUAL 1 )
	#git submodules init
	execute_process(COMMAND git submodule update --remote --merge --init -- "tool_lib/${subdir}"
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE OUTPUT)
      endif()
      subproject_version("tool_lib/${subdir}" subdir_version)#get the version
      message("${PREFIX_MSG}-- ${ColourBold}${Esc}[${color_code}m${EXEC_NAME}${ColourReset}: Adding ${ColourBold}tool${ColourReset} library ${ColourBold}${Esc}[${color_code2}m${subdir}${ColourReset} version: ${ColourBold}${subdir_version}${ColourReset}")
      add_subdirectory("tool_lib/${subdir}")
      
      get_directory_property(libname
	DIRECTORY "tool_lib/${subdir}"
	DEFINITION LIB_NAME)
      list(APPEND TOOL_LIBS "${libname}")
      link_directories(${CMAKE_SOURCE_DIR}/tool_lib/${subdir}/bin)    
    endforeach()
  endif()


  ###### Create the library ######
  add_library(${LIB_NAME} ${LIB_SRCS})
  target_link_libraries (${LIB_NAME}
    PUBLIC ${EXTRA_LIBS})
  target_include_directories(${LIB_NAME}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )



  ###### Create the executable ######
  add_executable(${EXEC_NAME} ${MAIN_FILE})
  target_link_libraries (${EXEC_NAME}
    PUBLIC ${LIB_NAME}
    PUBLIC ${TOOL_LIBS})
  target_include_directories(${EXEC_NAME}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")



  ###### Instalation rule ######
  install(TARGETS ${EXEC_NAME} DESTINATION bin)
  install(TARGETS ${LIB_NAME} DESTINATION bin)
  install(FILES ${HEADERS} DESTINATION include)



endif()
# enable_testing()
# # define a function to simplify adding tests
# function(do_test target arg result)
#   add_test(NAME Comp${arg} COMMAND ${target} ${arg})
#   set_tests_properties(Comp${arg}
#     PROPERTIES PASS_REGULAR_EXPRESSION ${result}
#     )
# endfunction(do_test)
# # do a bunch of result based tests
# do_test(canopen 1 1000 0 "")
# do_test(canopen 2 607A 0 4 d 1000 "")
