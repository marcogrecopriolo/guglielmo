# Find libsamplerate

FIND_PATH(LIBSAMPLERATE_INCLUDE_DIR samplerate.h
    PATHS ${MSVC_INCLUDE_PATH}
)

SET(LIBSAMPLERATE_NAMES ${LIBSAMPLERATE_NAMES} samplerate libsamplerate)
FIND_LIBRARY(LIBSAMPLERATE_LIBRARY NAMES ${LIBSAMPLERATE_NAMES}
    PATHS ${MSVC_LIBRARIES_PATH}
)

IF (LIBSAMPLERATE_INCLUDE_DIR AND LIBSAMPLERATE_LIBRARY)
    SET(LIBSAMPLERATE_FOUND TRUE)
ENDIF (LIBSAMPLERATE_INCLUDE_DIR AND LIBSAMPLERATE_LIBRARY)

IF (LIBSAMPLERATE_FOUND)
    IF (NOT LibSampleRate_FIND_QUIETLY)
        MESSAGE (STATUS "Found LibSampleRate: ${LIBSNDFILE_LIBRARY}")
    ENDIF (NOT LibSampleRate_FIND_QUIETLY)
ELSE (LIBSAMPLERATE_FOUND)
    IF (LibSampleRate_FIND_REQUIRED)
        MESSAGE (FATAL_ERROR "Could not find samplerate")
    ENDIF (LibSampleRate_FIND_REQUIRED)
ENDIF (LIBSAMPLERATE_FOUND)
