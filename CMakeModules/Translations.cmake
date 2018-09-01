
GET_FILENAME_COMPONENT(SRC_PARENT_DIR ${PROJECT_SOURCE_DIR} PATH)
SET(FGDATA_SRC_DIR "${SRC_PARENT_DIR}/fgdata")

if(EXISTS ${TRANSLATIONS_SRC_DIR})
    message(STATUS "Using explicitly defined translations from: ${TRANSLATIONS_SRC_DIR}")
    set(do_translate TRUE)
elseif(EXISTS ${FGDATA_SRC_DIR})
    SET(TRANSLATIONS_SRC_DIR "${FGDATA_SRC_DIR}/Translations")
    message(STATUS "Found translations dir implicitly: ${TRANSLATIONS_SRC_DIR}")
    set(do_translate TRUE)
else()
    message(STATUS "Couldn't find translations data, will not include translated string in the executable")
endif()

find_package(Qt5 5.4 COMPONENTS LinguistTools)
if (${do_translate} AND NOT TARGET Qt5::lrelease)
    set(do_translate FALSE)
    message(STATUS "Built-in translations disabled becuase Qt5 lrelease tool was not found."
        "\n(on Linux You may need to install an additional package containing the Qt5 translation tools)")
endif()

if (${do_translate})
    # FIXME - determine this based on subdirs of TRANSLATIONS_SRC_DIR
    set(LANGUAGES en_US de es nl fr it pl pt zh_CN)
    set(translation_res "${PROJECT_BINARY_DIR}/translations.qrc")

    add_custom_target(fgfs_qm_files ALL)

    file(WRITE ${translation_res} "<RCC>\n<qresource prefix=\"/\">\n")

    # qm generation and installation
    foreach(LANG ${LANGUAGES})
        # trim down to a two-letter code for output. This will need to get
        # smarter if we ever have different translations for a dialect,
        # eg en_GB, fr_CA or pt_BR. 
        string(SUBSTRING ${LANG} 0 2 simple_lang)

        set(out_file "${PROJECT_BINARY_DIR}/FlightGear_${simple_lang}.qm")
        add_custom_command(
            OUTPUT ${out_file}
            COMMAND Qt5::lrelease ${TRANSLATIONS_SRC_DIR}/${LANG}/FlightGear-Qt.xlf
                -qm ${out_file}
            DEPENDS ${TRANSLATIONS_SRC_DIR}/${LANG}/FlightGear-Qt.xlf
        )
        add_custom_target(fgfs_${simple_lang}_qm ALL DEPENDS ${out_file})

        add_dependencies(fgfs_qm_files fgfs_${simple_lang}_qm)

        # local path needed here, not absolute
        file(APPEND ${translation_res} "<file>FlightGear_${simple_lang}.qm</file>\n")
    endforeach()

    file(APPEND ${translation_res} "</qresource>\n</RCC>")

    # set this so config.h can detect it
    set(HAVE_QRC_TRANSLATIONS TRUE)
endif() # of do translate