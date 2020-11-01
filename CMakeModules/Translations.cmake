set(xlf_file "en_US/FlightGear-Qt.xlf")

if(EXISTS "${TRANSLATIONS_SRC_DIR}/${xlf_file}")
    message(STATUS "Using explicitly defined translations from: ${TRANSLATIONS_SRC_DIR}")
    set(do_translate TRUE)
elseif(EXISTS "${FG_DATA_DIR}/Translations/${xlf_file}")
    set(TRANSLATIONS_SRC_DIR "${FG_DATA_DIR}/Translations")
    set(do_translate TRUE)
    message(STATUS "Found translations dir implicitly: ${TRANSLATIONS_SRC_DIR}")
else()
    message(STATUS "Couldn't find translations data, will not include translated string in the executable")
    set(do_translate FALSE)
endif()

find_package(Qt5 5.4 COMPONENTS LinguistTools)
if (${do_translate} AND NOT TARGET Qt5::lrelease)
    set(do_translate FALSE)
    message(STATUS "Built-in translations disabled becuase Qt5 lrelease tool was not found."
        "\n(on Linux You may need to install an additional package containing the Qt5 translation tools)")
endif()


function(translation_dir_list result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if (${child} STREQUAL "default")
        continue()
    endif()

    if (IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist} PARENT_SCOPE)
endfunction()

if (${do_translate})
    translation_dir_list(LANGUAGES ${TRANSLATIONS_SRC_DIR})
    message(STATUS "Detected language files: ${LANGUAGES}")

    set(translation_res "${PROJECT_BINARY_DIR}/translations.qrc")

    add_custom_target(fgfs_qm_files ALL)

    file(WRITE ${translation_res} "<RCC>\n<qresource prefix=\"/\">\n")

    # qm generation and installation
    foreach(LANG ${LANGUAGES})
        # avoid issue if the FGData we found is a different version, and a translation is
        # missing: just skip it
        if (NOT EXISTS ${TRANSLATIONS_SRC_DIR}/${LANG}/FlightGear-Qt.xlf)
            continue()
        endif()

        set(out_file "${PROJECT_BINARY_DIR}/FlightGear_${LANG}.qm")
        add_custom_command(
            OUTPUT ${out_file}
            COMMAND Qt5::lrelease ${TRANSLATIONS_SRC_DIR}/${LANG}/FlightGear-Qt.xlf
                -qm ${out_file}
            DEPENDS ${TRANSLATIONS_SRC_DIR}/${LANG}/FlightGear-Qt.xlf
        )
        add_custom_target(fgfs_${LANG}_qm ALL DEPENDS ${out_file})

        add_dependencies(fgfs_qm_files fgfs_${LANG}_qm)

        # fix for bug https://sourceforge.net/p/flightgear/codetickets/2406/
        # ensure we expose the English translations in a 'short' form, so that
        # the QTranslator search order finds it
        if (${LANG} STREQUAL "en_US")
            message(STATUS "Wibble")
            file(APPEND ${translation_res} "<file alias=\"FlightGear_en.qm\">FlightGear_${LANG}.qm</file>\n")
        else()
            # local path needed here, not absolute
            file(APPEND ${translation_res} "<file>FlightGear_${LANG}.qm</file>\n")
        endif()
    endforeach()

    file(APPEND ${translation_res} "</qresource>\n</RCC>")

    # set this so config.h can detect it
    set(HAVE_QRC_TRANSLATIONS TRUE)

    add_custom_target(ts)
    
    foreach(lang ${LANGUAGES})
        add_custom_target(
            ts_${lang}
            COMMAND Qt5::lupdate ${CMAKE_SOURCE_DIR}/src/GUI
                -locations relative  -no-ui-lines -ts ${TRANSLATIONS_SRC_DIR}/${lang}/FlightGear-Qt.xlf
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
        add_dependencies(ts ts_${lang})
    endforeach()
endif() # of do translate

