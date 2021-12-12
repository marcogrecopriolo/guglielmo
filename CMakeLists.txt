cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0048 NEW)
project(guglielmo VERSION 0.3)
set (BINARY_TARGET ${PROJECT_NAME})
set (orgName SQSL)
set (orgDomain sqsl.org)

# sigh! add_definitions ignore anything past the variable, so the single and double quotes have to be added to the date format
execute_process(COMMAND date "+\"%Y-%m-%d %H:%M:%S %z\"" OUTPUT_VARIABLE currentDate OUTPUT_STRIP_TRAILING_WHITESPACE)
add_definitions (-DCURRENT_DATE=${currentDate})
add_definitions (-DTARGET=\"${PROJECT_NAME}\")
add_definitions (-DCURRENT_VERSION=\"${PROJECT_VERSION}\")
add_definitions (-DORGNAME=\"${orgName}\")
add_definitions (-DORGDOMAIN=\"${orgDomain}\")
execute_process(COMMAND uname OUTPUT_VARIABLE os OUTPUT_STRIP_TRAILING_WHITESPACE)
if(${os} STREQUAL "Linux")
   add_definitions(-DCHOOSE_CONFIG=${os})
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_XCC_FLAGS} -Wall -std=c++11 -flto")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

set(CMAKE_AUTORCC ON)
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "/usr/local/bin" CACHE PATH "default install path" FORCE)
endif()
#set (CMAKE_INSTALL_PREFIX /usr/local/bin)

#	modify if you want

if(MINGW)
    add_definitions ( -municode)
#else (MINGW)
#	add_definitions (-DSSE_AVAILABLE)
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

if(DEFINED AIRSPY)
	set(AIRSPY true)
	set(deviceChosen true)
endif ()

if(DEFINED SDRPLAY)
	set(SDRPLAY true)
	set(deviceChosen true)
endif ()

if(DEFINED RTLSDR)
	set(RTLSDR true)
	set(deviceChosen true)
endif ()

if(DEFINED HACKRF)
	set(HACKRF true)
	set(deviceChosen true)
endif ()

if(DEFINED LIMESDR)
	set(LIMESDR true)
	set(deviceChosen true)
endif ()

# choose all devices if none specified
if (NOT deviceChosen)
	set(AIRSPY true)
	set(SDRPLAY true)
	set(RTLSDR true)
	set(HACKRF true)
	set(LIMESDR true)
endif()

if (DEFINED FDK_AAC)
        set (FDK_AAC true)
endif ()

add_definitions (-DPRESET_NAME)
add_definitions (-D__THREADED_BACKEND)	# uncomment for use for an RPI
add_definitions (-D__MSC_THREAD__)
#
########################################################################

	find_package (PkgConfig)
	find_package (LSBId)

	find_package (Qt5Core REQUIRED)
	find_package (Qt5Widgets REQUIRED)
#	find_package (Qt5Xml REQUIRED)
	find_package (Qwt REQUIRED)

	include_directories (
#	  ${Qt5Xml_INCLUDE_DIRS}
	  ${QWT_INCLUDE_DIRS}
	)

        find_package(FFTW3f)
        if (NOT FFTW3F_FOUND)
            message(FATAL_ERROR "please install FFTW3")
        endif ()

        find_package(zlib)
	if (NOT ZLIB_FOUND)
            message(FATAL_ERROR "please install libz")
        endif ()
	list(APPEND extraLibs ${ZLIB_LIBRARY})

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

	find_library (PTHREADS pthread)
	if (NOT(PTHREADS))
	   message (FATAL_ERROR "please install libpthread")
	else (NOT(PTHREADS))
	   set (extraLibs ${extraLibs} ${PTHREADS})
	endif (NOT(PTHREADS))

#######################################################################
#
#	Here we really start

	include_directories (
	           ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	           ${QT5Widgets_INCLUDES}
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
	           ./include/support/viterbi-jan
	           ./include/support/viterbi-spiral
	           ./include/output
	           ./include/scopes-qwt6
	           ./devices
	           /usr/include/
	)

	set (${PROJECT_NAME}_HDRS
             ./include/country-codes.h
	     ./include/constants.h
	     ./include/settings.h
	     ./include/dab/dab-processor.h
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
	     ./incluces/backend/reed-solomon.h
	     ./include/backend/msc-handler.h
	     ./include/backend/backend.h
	     ./include/backend/backend-deconvolver.h
	     ./include/backend/backend-driver.h
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
	     ./include/support/process-params.h
	     ./include/support/fft-handler.h
	     ./include/support/ringbuffer.h
	     ./include/support/dab-params.h
	     ./include/support/band-handler.h
	     ./include/support/text-mapper.h
	     ./include/support/dab-tables.h
	     ./include/support/viterbi-jan/viterbi-handler.h
	     ./include/support/viterbi-spiral/viterbi-spiral.h
	     ./devices/device-handler.h
	)

	set (${PROJECT_NAME}_SRCS
	     ${${PROJECT_NAME}_SRCS}
	     ./src/main.cpp
	     ./src/dab/dab-processor.cpp
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
	     ./src/support/fft-handler.cpp
	     ./src/support/dab-params.cpp
	     ./src/support/band-handler.cpp
	     ./src/support/text-mapper.cpp
	     ./src/support/dab-tables.cpp
	     ./src/support/viterbi-jan/viterbi-handler.cpp
	     ./src/support/viterbi-spiral/viterbi-spiral.cpp
	     ./devices/device-handler.cpp
	)

	set (${PROJECT_NAME}_MOCS
	     ./include/dab/dab-processor.h
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
	     ./include/output/audio-base.h
	     ./include/ofdm/sample-reader.h
	     ./include/ofdm/ofdm-decoder.h
	     ./include/ofdm/phasereference.h
	     ./include/ofdm/fib-decoder.h
	     ./include/ofdm/fic-handler.h
	     ./include/ofdm/tii_detector.h
	     ./include/backend/msc-handler.h
	     ./include/backend/backend.h
	     ./include/backend/audio/mp2processor.h
	     ./include/backend/audio/mp4processor.h
	     ./include/backend/data/virtual-datahandler.h
	     ./include/backend/data/pad-handler.h
	     ./include/backend/data/mot/mot-handler.h
	     ./include/backend/data/mot/mot-object.h
	     ./include/backend/data/mot/mot-dir.h
	     ./include/backend/data/ip-datahandler.h
	     ./include/backend/data/tdc-datahandler.h
	     ./include/backend/data/journaline-datahandler.h
	     ./include/backend/data/data-processor.h
	     ./include/support/band-handler.h
	)

	set (${PROJECT_NAME}_UIS
	        ${${PROJECT_NAME}_UIS}
	     ./guglielmo.ui
	     ./about.ui
	     ./settings.ui
	   )

	set (${PROJECT_NAME}_MOCS
	        ${${PROJECT_NAME}_MOCS} ./include/radio.h)

	set ($(PROJECT_NAME)_HDRS
	           ${${PROJECT_NAME}_HDRS}
		./include/radio.h
	   )

	set (${PROJECT_NAME}_SRCS
	           ${${PROJECT_NAME}_SRCS}
		./src/radio.cpp
		./src/dialogs.cpp
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
	   set (${PROJECT_NAME}_MOCS
	        ${${PROJECT_NAME}_MOCS}
	        ./include/backend/audio/fdk-aac.h
	   )
           list(APPEND extraLibs ${FDK_AAC_LIBRARIES})
	   add_definitions (-D__WITH_FDK_AAC__)
	elseif (NOT FDK_AAC)
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
	   set (${PROJECT_NAME}_MOCS
	        ${${PROJECT_NAME}_MOCS}
	        ./include/backend/audio/faad-decoder.h
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
	   set (${PROJECT_NAME}_MOCS
	        ${${PROJECT_NAME}_MOCS}
	        ./devices/sdrplay-handler/sdrplay-handler.h
	        ./devices/sdrplay-handler/sdrplayselect.h
	   )

	   include_directories (
	     ./devices/sdrplay-handler
	   )

	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./devices/sdrplay-handler/sdrplay-handler.h
	        ./devices/sdrplay-handler/sdrplayselect.h
	        ./devices/sdrplay-handler/mirsdrapi-rsp.h
           )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./devices/sdrplay-handler/sdrplay-handler.cpp
	        ./devices/sdrplay-handler/sdrplayselect.cpp
	   )

	 add_definitions (-DHAVE_SDRPLAY)
	endif (SDRPLAY)

	if (AIRSPY)
#           find_package(LibAIRSPY)
#           if (NOT LIBAIRSPY_FOUND)
#               message(FATAL_ERROR "please install airspy library")
#           endif ()
#	   ## include_directories (${AIRSPYLIB_INCLUDE_DIR})

	   set (${PROJECT_NAME}_MOCS
	        ${${PROJECT_NAME}_MOCS}
	        ./devices/airspy-handler/airspy-handler.h
	   )

	   include_directories (
	     ./devices/airspy-handler
	     ./devices/airspy-handler/libairspy
	   )

	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	        ./devices/airspy-handler/airspy-handler.h
	        ./devices/airspy-handler/airspyfilter.h
	        ./devices/airspy-handler/libairspy/airspy.h
	   )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	        ./devices/airspy-handler/airspy-handler.cpp
	        ./devices/airspy-handler/airspyfilter.cpp
	   )

	   add_definitions (-DHAVE_AIRSPY)
	endif (AIRSPY)

	if (HACKRF)

	   set (${PROJECT_NAME}_MOCS
	        ${${PROJECT_NAME}_MOCS}
	        ./devices/hackrf-handler/hackrf-handler.h
	   )

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
#
	if (LIMESDR)
	   set (${PROJECT_NAME}_MOCS
	        ${${PROJECT_NAME}_MOCS}
	        ./devices/lime-handler/lime-handler.h
	   )

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
	   set (${PROJECT_NAME}_MOCS
	        ${${PROJECT_NAME}_MOCS}
	        ./devices/rtlsdr-handler/rtlsdr-handler.h
	   )

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

#
	if (TRY_EPG)
	   set ($(PROJECT_NAME)_HDRS
	        ${${PROJECT_NAME}_HDRS}
	             ./include/backend/data/epg/epgdec.h
	   )

	   set (${PROJECT_NAME}_SRCS
	        ${${PROJECT_NAME}_SRCS}
	             ./src/backend/data/epg/epgdec.cpp
	   )
	   add_definitions (-DTRY_EPG)
	endif (TRY_EPG)

        find_package(Portaudio)
        if (NOT PORTAUDIO_FOUND)
            message(FATAL_ERROR "please install portaudio V19")
        endif ()
        include_directories (
	     ${PORTAUDIO_INCLUDE_DIRS}
	)
        list(APPEND extraLibs ${PORTAUDIO_LIBRARIES})
	set (${PROJECT_NAME}_HDRS
	     ${${PROJECT_NAME}_HDRS}
	    ./include/output/audiosink.h
	)

	set (${PROJECT_NAME}_MOCS
	    ${${PROJECT_NAME}_MOCS}
	   ./include/output/audiosink.h
	)

	set (${PROJECT_NAME}_SRCS
	   ${${PROJECT_NAME}_SRCS}
	   ./src/output/audiosink.cpp
	)
	find_package (Qt5Multimedia REQUIRED)
	include_directories (
	  ${Qt5Multimedia_INCLUDE_DIRS}
	)
        list(APPEND extraLibs ${Qt5Multimedia_LIBRARIES})
	set (${PROJECT_NAME}_HDRS
	     ${${PROJECT_NAME}_HDRS}
	    ./include/output/Qt-audio.h
	    ./include/output/Qt-audiodevice.h
	)

	set (${PROJECT_NAME}_MOCS
	    ${${PROJECT_NAME}_MOCS}
	   ./include/output/Qt-audio.h
	   ./include/output/Qt-audiodevice.h
	)

	set (${PROJECT_NAME}_SRCS
	   ${${PROJECT_NAME}_SRCS}
	   ./src/output/Qt-audio.cpp
	   ./src/output/Qt-audiodevice.cpp
	)

	QT5_WRAP_UI (UIS ${${PROJECT_NAME}_UIS})

	include_directories (
	          ${SDRPLAY_INCLUDES}
	          ${QT5Widgets_INCLUDES}
	          ${QT_QTCORE_INCLUDE_DIR}
	          ${QT_QTGUI_INCLUDE_DIR}
	          ${FFTW_INCLUDE_DIRS}
	          ${FAAD_INCLUDE_DIRS}
	)

	QT5_WRAP_CPP (MOCS ${${PROJECT_NAME}_MOCS})

	add_executable (${BINARY_TARGET}
	                ${${PROJECT_NAME}_SRCS}
	                ${UIS}
	                ${RSCS}
	                ${TRS}
	                ${MOCS}
	                ./guglielmo.qrc
	)

	target_link_libraries (${BINARY_TARGET}
	                       Qt5::Widgets
#	                       Qt5::Xml
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
