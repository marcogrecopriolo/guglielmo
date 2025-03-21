cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0048 NEW)
project(guglielmo VERSION 0.7)
set (BINARY_TARGET ${PROJECT_NAME})
set (orgName SQSL)
set (orgDomain sqsl.org)

option(LTO "Attempt to enable Link Time Optimization if available" ON)
option(AIRSPY "Enable Airspy support" ON)
option(SDRPLAY "Enable SDRPlay support" ON)
option(SDRPLAY_V3 "Enable SDRPlay-v3 support" ON)
option(RTLSDR "Enable rtl-sdr support" ON)
option(HACKRF "Enable HackRF support" ON)
option(LIMESDR "Enable LimeSDR support" ON)
option(PLUTO "Enable Pluto support" OFF)
option(FDK_AAC "Use FDK AAC instead of FAAD2, for AAC decoding" OFF)
option(RPI "Enable Raspberry PI support" OFF)
option(VITERBI_NEON "Use NEON instructions for the Viterbi decoder (only on ARM)" OFF)
option(VITERBI_SSE "Use SSE instructions for the Viterbi decoder" ON)
option(USE_SPI "Enable SPI / EPG  support" ON)

# sigh! add_definitions ignore anything past the variable, so the single and double quotes have to be added to the date format
execute_process(COMMAND date "+\"%Y-%m-%d %H:%M:%S %z\"" OUTPUT_VARIABLE currentDate OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions (-DCURRENT_DATE=${currentDate})
add_definitions (-DTARGET=\"${PROJECT_NAME}\")
add_definitions (-DCURRENT_VERSION=\"${PROJECT_VERSION}\")
add_definitions (-DORGNAME=\"${orgName}\")
add_definitions (-DORGDOMAIN=\"${orgDomain}\")
set(MPRIS_DEFAULT OFF)
if (UNIX)
    add_definitions(-DCHOOSE_CONFIG=Unix)
    execute_process(COMMAND uname OUTPUT_VARIABLE os OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(${os} STREQUAL "Linux")
	set(MPRIS_DEFAULT ON)
    endif()
elseif (NOT MSYS AND NOT MINGW)
    execute_process(COMMAND powershell Get-Date -Uformat \"%Y-%m-%d %H:%M:%S %Z\" OUTPUT_VARIABLE currentDate OUTPUT_STRIP_TRAILING_WHITESPACE)
        add_definitions (-DCURRENT_DATE=\"${currentDate}\")
	add_definitions(-DCHOOSE_CONFIG=Windows)
endif()
option(MPRIS "Enable MPRIS support" ${MPRIS_DEFAULT})

set(CMAKE_CXX_STANDARD 11)
if(LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT guglielmo_ipo_supported)
    if(guglielmo_ipo_supported AND NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endif()

if (NOT (UNIX OR MSYS OR MINGW))
    set (MSVC_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/../includes")
    set (MSVC_LIBRARIES_PATH "${CMAKE_SOURCE_DIR}/../libraries")
    set (incBasePath "c:/\"Program\ Files/Microsoft\ Visual\ Studio\"/2022/Community/SDK/ScopeCppSDK/vc15")
    set (libBasePath "c:/\"Program\ Files/Windows\ Kits\"/10/Lib/10.0.19041.0")
    include_directories (
            ${MSVC_INCLUDE_PATH}
            include/share
            ${incBasePath}/VC/include
            ${incBasePath}/SDK/include
            ${incBasePath}/SDK/include/shared
            ${incBasePath}/SDK/include/ucrt
            ${incBasePath}/SDK/include/um
    )
    link_directories (
        ${MSVC_LIBRARIES_PATH}
        ${libBasePath}/ucrt/x86
        ${libBasePath}/um/x86
        c:/"Program\ Files/Microsoft\ Visual\ Studio"/2022/Community/VC/Tools/MSVC/14.31.31103/lib/x86
    )
endif()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "/usr/local/bin" CACHE PATH "default install path" FORCE)
endif()

if(MINGW)
    add_definitions ( -municode)
endif()

# msys2 cmake mishandles prefixes and postfixes
if(MSYS)
    set(CMAKE_FIND_LIBRARY_PREFIXES "")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
endif()

########################################################################
# select the release build type by default to get optimization flags
########################################################################
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

### make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

add_definitions (-DPRESET_NAME)
if(RPI)
	add_definitions (-D__THREADED_BACKEND)
	add_definitions (-D__MSC_THREAD__)
endif()

########################################################################

	find_package (PkgConfig)
	find_package (LSBId)

	find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
	find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
	if (QT_VERSION VERSION_LESS 5.15)
	    macro(qt_wrap_cpp)
	        qt5_wrap_cpp(${ARGV})
	    endmacro()
	    macro(qt_wrap_ui)
	        qt5_wrap_ui(${ARGV})
	    endmacro()
	endif ()
	find_package (Qt${QT_VERSION_MAJOR}Widgets REQUIRED)
	find_package (Qwt REQUIRED)

	include_directories (
	  ${QWT_INCLUDE_DIRS}
	)

        find_package(FFTW3f)
        if (NOT FFTW3F_FOUND)
            message(FATAL_ERROR "please install FFTW3")
        endif ()

	# for MinGW we rely on the system zlib
	if (NOT MINGW)
	    find_package(zlib)
	    if (NOT ZLIB_FOUND)
		message(FATAL_ERROR "please install libz")
	    endif ()
	    list(APPEND extraLibs ${ZLIB_LIBRARY})
	endif ()

        find_package(LibSndFile)
        if (NOT LIBSNDFILE_FOUND)
            message(FATAL_ERROR "please install libsndfile")
        endif ()
        list(APPEND extraLibs ${LIBSNDFILE_LIBRARY})

        find_package(LibSampleRate)
        if (NOT LIBSAMPLERATE_FOUND)
            message(FATAL_ERROR "please install libsamplerate")
        endif ()
        list(APPEND extraLibs ${LIBSAMPLERATE_LIBRARY})

	if (MPRIS)
		# TODO check
		pkg_check_modules(MPRIS mpris-qt5)
		if (MPRIS_FOUND)
			add_definitions(-DHAVE_MPRIS)
			include_directories(${MPRIS_INCLUDE_DIRS})
			list(APPEND extraLibs ${MPRIS_LIBRARIES})
		endif ()
	endif ()

#######################################################################
#
#	Here we really start

	include_directories (
	           ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	           ${QT${QT_VERSION_MAJOR}Widgets_INCLUDES}
	           ${QT_QTCORE_INCLUDE_DIR}
	           ${QT_QTGUI_INCLUDE_DIR}
	           .
	           ./include
	           ./include/dab
	           ./include/fm
	           ./include/rds
	           ./include/ofdm
	           ./include/protection
	           ./include/backend
	           ./include/backend/audio
	           ./include/backend/data
	           ./include/backend/data/journaline
	           ./include/backend/data/mot
	           ./include/support
	           ./include/support/viterbi-spiral
	           ./include/output
		   ./include/share
	           ./devices
	           /usr/include/
	)

	set (${PROJECT_NAME}_HDRS
	     ./include/constants.h
	     ./include/logger.h
	     ./include/logging.h
	     ./include/settings.h
	     ./include/combobox.h
	     ./include/listwidget.h
	     ./include/dab/dab-processor.h
	     ./include/dab/dab-tables.h
             ./include/fm/fm-demodulator.h
             ./include/fm/fm-processor.h
             ./include/support/fir-filters.h
             ./include/support/fft.h
             ./include/support/fft-filters.h
             ./include/support/iir-filters.h
             ./include/support/pll.h
             ./include/support/trigtabs.h
             ./include/support/squelchClass.h
             ./include/rds/rds-blocksynchronizer.h
             ./include/rds/rds-decoder.h
             ./include/rds/rds-groupdecoder.h
             ./include/rds/rds-group.h
             ./include/rds/codetables.h
             ./include/ofdm/sample-reader.h
	     ./include/ofdm/phasereference.h
	     ./include/ofdm/ofdm-decoder.h
	     ./include/ofdm/phasetable.h
	     ./include/ofdm/freq-interleaver.h
	     ./include/ofdm/fib-decoder.h
	     ./include/ofdm/dab-config.h
	     ./include/ofdm/fib-table.h
	     ./include/ofdm/fic-handler.h
	     ./include/ofdm/tii_detector.h
	     ./include/ofdm/timesyncer.h
	     ./include/protection/protTables.h
	     ./include/protection/protection.h
	     ./include/protection/uep-protection.h
	     ./include/protection/eep-protection.h
	     ./include/backend/firecode-checker.h
	     ./include/backend/frame-processor.h
	     ./include/backend/charsets.h
	     ./include/backend/galois.h
	     ./include/backend/reed-solomon.h
	     ./include/backend/msc-handler.h
	     ./include/backend/backend.h
	     ./include/backend/backend-deconvolver.h
	     ./include/backend/backend-driver.h
	     ./include/backend/services.h
	     ./include/backend/audio/mp4processor.h
	     ./include/backend/audio/bitWriter.h
	     ./include/backend/audio/mp2processor.h
	     ./include/backend/data/ip-datahandler.h
	     ./include/backend/data/tdc-datahandler.h
	     ./include/backend/data/journaline-datahandler.h
	     ./include/backend/data/journaline/dabdatagroupdecoder.h
	     ./include/backend/data/journaline/crc_8_16.h
	     ./include/backend/data/journaline/log.h
	     ./include/backend/data/journaline/newssvcdec_impl.h
	     ./include/backend/data/journaline/Splitter.h
	     ./include/backend/data/journaline/dabdgdec_impl.h
	     ./include/backend/data/journaline/newsobject.h
	     ./include/backend/data/journaline/NML.h
	     ./include/backend/data/virtual-datahandler.h
	     ./include/backend/data/pad-handler.h
	     ./include/backend/data/mot/mot-handler.h
	     ./include/backend/data/mot/mot-object.h
	     ./include/backend/data/mot/mot-dir.h
	     ./include/backend/data/data-processor.h
	     ./include/output/audio-base.h
	     ./include/output/newconverter.h
	     ./include/output/audiosink.h
	     ./include/output/Qt-audio.h
	     ./include/output/Qt-audiodevice.h
	     ./include/support/process-params.h
	     ./include/support/fft-handler.h
	     ./include/support/ringbuffer.h
	     ./include/support/dab-params.h
	     ./include/support/band-handler.h
	     ./include/support/bits-helper.h
	     ./include/support/math-helper.h
	     ./include/support/viterbi-spiral/viterbi-spiral.h
	     ./include/radio.h
	     ./devices/device-handler.h
	)

	set (${PROJECT_NAME}_SRCS
	     ${${PROJECT_NAME}_SRCS}
	     ./src/main.cpp
	     ./src/dab/dab-processor.cpp
	     ./src/dab/dab-tables.cpp
             ./src/fm/fm-demodulator.cpp
             ./src/fm/fm-processor.cpp
	     ./src/support/fir-filters.cpp
             ./src/support/fft.cpp
             ./src/support/fft-filters.cpp
             ./src/support/iir-filters.cpp
             ./src/support/pll.cpp
             ./src/support/trigtabs.cpp
             ./src/rds/rds-blocksynchronizer.cpp
             ./src/rds/rds-decoder.cpp
             ./src/rds/rds-group.cpp
             ./src/rds/rds-groupdecoder.cpp
             ./src/ofdm/sample-reader.cpp
	     ./src/ofdm/ofdm-decoder.cpp
	     ./src/ofdm/phasereference.cpp
	     ./src/ofdm/phasetable.cpp
	     ./src/ofdm/freq-interleaver.cpp
	     ./src/ofdm/fib-decoder.cpp
	     ./src/ofdm/fic-handler.cpp
	     ./src/ofdm/tii_detector.cpp
	     ./src/ofdm/timesyncer.cpp
	     ./src/protection/protTables.cpp
	     ./src/protection/protection.cpp
	     ./src/protection/eep-protection.cpp
	     ./src/protection/uep-protection.cpp
	     ./src/backend/firecode-checker.cpp
	     ./src/backend/charsets.cpp
	     ./src/backend/galois.cpp
	     ./src/backend/reed-solomon.cpp
	     ./src/backend/msc-handler.cpp
	     ./src/backend/backend.cpp
	     ./src/backend/backend-deconvolver.cpp
	     ./src/backend/backend-driver.cpp
	     ./src/backend/audio/mp4processor.cpp
	     ./src/backend/audio/bitWriter.cpp
	     ./src/backend/audio/mp2processor.cpp
	     ./src/backend/data/ip-datahandler.cpp
	     ./src/backend/data/journaline-datahandler.cpp
	     ./src/backend/data/journaline/crc_8_16.c
	     ./src/backend/data/journaline/log.c
	     ./src/backend/data/journaline/newssvcdec_impl.cpp
	     ./src/backend/data/journaline/Splitter.cpp
	     ./src/backend/data/journaline/dabdgdec_impl.c
	     ./src/backend/data/journaline/newsobject.cpp
	     ./src/backend/data/journaline/NML.cpp
	     ./src/backend/data/tdc-datahandler.cpp
	     ./src/backend/data/pad-handler.cpp
	     ./src/backend/data/mot/mot-handler.cpp
	     ./src/backend/data/mot/mot-object.cpp
	     ./src/backend/data/mot/mot-dir.cpp
	     ./src/backend/data/data-processor.cpp
	     ./src/output/audio-base.cpp
	     ./src/output/newconverter.cpp
	     ./src/output/audiosink.cpp
	     ./src/output/Qt-audio.cpp
	     ./src/output/Qt-audiodevice.cpp
	     ./src/support/fft-handler.cpp
	     ./src/support/dab-params.cpp
	     ./src/support/band-handler.cpp
	     ./src/support/viterbi-spiral/viterbi-spiral.cpp
	     ./src/radio.cpp
	     ./src/dialogs.cpp
	     ./devices/device-handler.cpp
	)

	set (${PROJECT_NAME}_UIS
	        ${${PROJECT_NAME}_UIS}
	     ./guglielmo.ui
	     ./about.ui
	     ./settings.ui
	   )


#########################################################################

	if (FDK_AAC)
	   find_package(Fdk-AAC)
	   if (NOT LIBFDK_AAC_FOUND)
	      message (FATAL_ERROR, "please install libfdk-aac")
	   endif ()
	   include_directories (./specials/fdk-aac)
	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./include/backend/audio/fdk-aac.h
	   )
	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./src/backend/audio/fdk-aac.cpp
	   )
           list(APPEND extraLibs ${FDK_AAC_LIBRARIES})
	   add_definitions (-D__WITH_FDK_AAC__)
	else (FDK_AAC)
           find_package(Faad)
           if (NOT FAAD_FOUND )
              message(FATAL_ERROR "please install libfaad")
           endif ()
	   set (${PROJECT_NAME}_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./include/backend/audio/faad-decoder.h
	   )
	   set (${PROJECT_NAME}_SRCS
                ${${PROJECT_NAME}_SRCS}
	        ./src/backend/audio/faad-decoder.cpp
	   )
	   add_definitions (-D__WITH_FAAD__)
	endif (FDK_AAC)

#	the viterbi decoder
#
	if (VITERBI_NEON)
	   set(VITERBI_SSE false)
	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./src/support/viterbi-spiral/spiral-neon.h
	   )
	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./src/support/viterbi-spiral/spiral-neon.c
	   )
	   
	   add_definitions (-DNEON_AVAILABLE)
	elseif(VITERBI_SSE)
	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./src/support/viterbi-spiral/spiral-sse.h
	   )
	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./src/support/viterbi-spiral/spiral-sse.c
	   )
	   add_definitions (-DSSE_AVAILABLE)
	else (VITERBI_SSE)
	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./src/support/viterbi-spiral/spiral-no-sse.h
	   )
	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./src/support/viterbi-spiral/spiral-no-sse.c
	   )
	endif (VITERBI_NEON)

##########################################################################
#	The devices
#
	if (SDRPLAY)
	   include_directories (
	     ./devices/sdrplay-handler
	   )

	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./devices/sdrplay-handler/sdrplay-handler.h
	        ./devices/sdrplay-handler/mirsdrapi-rsp.h
           )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./devices/sdrplay-handler/sdrplay-handler.cpp
	   )

	 add_definitions (-DHAVE_SDRPLAY)
	endif (SDRPLAY)

	if (SDRPLAY_V3)
	   include_directories (
	     ./devices/sdrplay-handler-v3
	     ./devices/sdrplay-handler-v3/include
	   )

	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./devices/sdrplay-handler-v3/sdrplay-handler-v3.h
           )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./devices/sdrplay-handler-v3/sdrplay-handler-v3.cpp
	   )

	 add_definitions (-DHAVE_SDRPLAY_V3)
	endif (SDRPLAY_V3)

	if (AIRSPY)
#           find_package(LibAIRSPY)
#           if (NOT LIBAIRSPY_FOUND)
#               message(FATAL_ERROR "please install airspy library")
#           endif ()
#	   ## include_directories (${AIRSPYLIB_INCLUDE_DIR})

	   include_directories (
	     ./devices/airspy-handler
	     ./devices/airspy-handler/libairspy
	   )

	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./devices/airspy-handler/airspy-handler.h
	        ./devices/airspy-handler/libairspy/airspy.h
	   )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./devices/airspy-handler/airspy-handler.cpp
	   )

	   add_definitions (-DHAVE_AIRSPY)
	endif (AIRSPY)

	if (HACKRF)
	   include_directories (
	     ./devices/hackrf-handler
	     ./devices/hackrf-handler/libhackrf
	   )

	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./devices/hackrf-handler/hackrf-handler.h
	        ./devices/hackrf-handler/libhackrf/hackrf.h
	   )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./devices/hackrf-handler/hackrf-handler.cpp
	   )

	   add_definitions (-DHAVE_HACKRF)
	endif (HACKRF)
	if (LIMESDR)
	   include_directories (
	     ./devices/lime-handler
	   )

	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./devices/lime-handler/lime-handler.h
	        ./devices/lime-handler/LimeSuite.h
	        ./devices/lime-handler/LMS7002M_parameters.h
	   )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./devices/lime-handler/lime-handler.cpp
	   )

	   add_definitions (-DHAVE_LIME)
	endif (LIMESDR)

	if (RTLSDR)
	   include_directories (
	        ./devices/rtlsdr-handler/
	   )

	   set (${PROJECT_NAME}_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./devices/rtlsdr-handler/rtlsdr-handler.h
	        ./devices/rtlsdr-handler/rtl-sdr.h
	        ./devices/rtlsdr-handler/rtl-sdr_export.h
	   )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./devices/rtlsdr-handler/rtlsdr-handler.cpp
	   )

	   add_definitions (-DHAVE_RTLSDR)
	endif()

	if (PLUTO)
	   include_directories (
	     ./devices/pluto-handler
	   )

	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./devices/pluto-handler/dabFilter.h
	        ./devices/pluto-handler/pluto-handler.h
	   )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./devices/pluto-handler/pluto-handler.cpp
	   )

	   add_definitions (-DHAVE_PLUTO)
	   list(APPEND extraLibs -liio  -lad9361)
	endif (PLUTO)

	if (USE_SPI)
	   find_package (Qt${QT_VERSION_MAJOR}Xml REQUIRED)
	   include_directories (
	     ./include/backend/data/epg
	     ${Qt${QT_VERSION_MAJOR}Xml_INCLUDE_DIRS}
	   )

	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./include/backend/data/epg/epgdec.h
	   )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./src/backend/data/epg/epgdec.cpp
	   )
	   add_definitions (-DUSE_SPI)
	   list(APPEND extraLibs ${Qt${QT_VERSION_MAJOR}Xml_LIBRARIES})
	endif (USE_SPI)

        find_package(Portaudio)
        if (NOT PORTAUDIO_FOUND)
            message(FATAL_ERROR "please install portaudio V19")
        endif ()
        include_directories (
	     ${PORTAUDIO_INCLUDE_DIRS}
	)
        list(APPEND extraLibs ${PORTAUDIO_LIBRARIES})

	find_package (Qt${QT_VERSION_MAJOR}Multimedia REQUIRED)
	include_directories (
	  ${Qt${QT_VERSION_MAJOR}Multimedia_INCLUDE_DIRS}
	)
        list(APPEND extraLibs ${Qt${QT_VERSION_MAJOR}Multimedia_LIBRARIES})

	if(WIN32 AND NOT MSYS AND NOT MINGW)
		set (${PROJECT_NAME}_HDRS
	 	    ${${PROJECT_NAME}_HDRS}
		    ./include/share/getopt.h
		)
		set (${PROJECT_NAME}_SRCS
	 	    ${${PROJECT_NAME}_SRCS}
		    ./src/share/getopt.c
		)
	endif()

	qt_wrap_ui (UIS ${${PROJECT_NAME}_UIS})

	include_directories (
	          ${SDRPLAY_INCLUDES}
	          ${QT${QT_VERSION_MAJOR}Widgets_INCLUDES}
	          ${QT_QTCORE_INCLUDE_DIR}
	          ${QT_QTGUI_INCLUDE_DIR}
	          ${FFTW_INCLUDE_DIRS}
	          ${FAAD_INCLUDE_DIRS}
	)

	list(REMOVE_ITEM ${PROJECT_NAME}_HDRS "./devices/device-handler.h")
	qt_wrap_cpp (MOCS ${${PROJECT_NAME}_HDRS})

	add_executable (${BINARY_TARGET}
	                ${${PROJECT_NAME}_SRCS}
	                ${UIS}
	                ${RSCS}
	                ${TRS}
	                ${MOCS}
	                ./guglielmo.qrc
	)

	target_link_libraries (${BINARY_TARGET}
	                       Qt${QT_VERSION_MAJOR}::Widgets
	                       ${RTLTCP_lib}
	                       ${FFTW3F_LIBRARIES}
	                       ${extraLibs}
	                       ${FAAD_LIBRARIES}
	                       ${CMAKE_DL_LIBS}
			       ${QWT_LIBRARIES}
	)
	INSTALL (TARGETS ${BINARY_TARGET} DESTINATION ./linux-bin)

########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
