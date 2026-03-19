cmake_minimum_required(VERSION 3.20)
project(BreathalyzerVST3 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{VST3_SDK_ROOT} AND NOT VST3_SDK_ROOT)
  message(FATAL_ERROR "Set VST3_SDK_ROOT to the Steinberg VST3 SDK path.")
endif()

if(NOT VST3_SDK_ROOT)
  set(VST3_SDK_ROOT $ENV{VST3_SDK_ROOT})
endif()

file(TO_CMAKE_PATH "${VST3_SDK_ROOT}" VST3_SDK_ROOT)

list(APPEND CMAKE_MODULE_PATH "${VST3_SDK_ROOT}/cmake/modules")
include("${VST3_SDK_ROOT}/cmake/modules/SMTG_VST3_SDK.cmake")
include("${VST3_SDK_ROOT}/cmake/modules/SMTG_AddVST3Library.cmake")

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ADD_VST3_UTILITIES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_MODULE_INFO OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)

add_subdirectory(${VST3_SDK_ROOT} ${CMAKE_BINARY_DIR}/vst3sdk)

set(Breathalyzer_SOURCES
    src/BreathalyzerProcessor.cpp
    src/BreathalyzerController.cpp
    src/BreathalyzerPluginFactory.cpp
    src/dasiydsp/adsr.cpp
    src/dasiydsp/formantosc.cpp
    src/dasiydsp/FormantVoice.cpp
    ${VST3_SDK_ROOT}/public.sdk/source/main/dllmain.cpp
)

set(Breathalyzer_HEADERS
    src/BreathalyzerProcessor.h
    src/BreathalyzerController.h
    src/BreathalyzerIDs.h
    src/BreathalyzerDefaults.h
    src/dsp/BreathalyzerSynth.h
    src/dsp/growlfx.h
    src/WeCore/SongbirdAEFilter.h
    src/WeCore/General/CoreMath.h
    src/WeCore/General/ParameterDefinition.h
    src/WeCore/WEFilters/TPTSVFilterParameters.h
    src/WeCore/WEFilters/TPTSVFilter.h
    src/WeCore/SongbirdFilters/Formant.h
    src/WeCore/SongbirdFilters/SongbirdFormantFilter.h
    src/dasiydsp/adsr.h
    src/dasiydsp/dsp.h
    src/dasiydsp/formantosc.h
    src/dasiydsp/FormantVoice.h
)

smtg_add_vst3plugin(Breathalyzer
    PACKAGE_NAME "Breathalyzer"
    SOURCES_LIST ${Breathalyzer_SOURCES} ${Breathalyzer_HEADERS}
)

target_include_directories(Breathalyzer PRIVATE
    ${VST3_SDK_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WeCore
)

target_link_libraries(Breathalyzer PRIVATE
    sdk
    sdk_common
    base
    pluginterfaces
    vstgui_support
)

target_compile_definitions(Breathalyzer PRIVATE
    VSTGUI_LIVE_EDITING=1
    $<$<CONFIG:Debug>:BREATHALYZER_DEBUG_NAME>
    $<$<CONFIG:Debug>:BREATHALYZER_DEBUG_UIDS>
)

set_target_properties(Breathalyzer PROPERTIES
    OUTPUT_NAME "Breathalyzer"
    OUTPUT_NAME_DEBUG "DebugBreathalyzer"
    OUTPUT_NAME_RELEASE "Breathalyzer"
    OUTPUT_NAME_RELWITHDEBINFO "Breathalyzer"
    OUTPUT_NAME_MINSIZEREL "Breathalyzer"
)

smtg_target_add_plugin_resources(Breathalyzer
    RESOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/breathalyzer.uidesc
        ${CMAKE_CURRENT_SOURCE_DIR}/logo_strip.png
)
