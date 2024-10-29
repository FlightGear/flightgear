

set(QMLDIR ${CMAKE_SOURCE_DIR}/src/GUI/qml)

if (WIN32)
    file(TO_NATIVE_PATH "${FG_QT_BIN_DIR}" _qt_bin_dir_native)
    set(CMAKE_MSVCIDE_RUN_PATH "${_qt_bin_dir_native}")

    add_custom_target(windeploy
        COMMENT "Running windeployqt on FGFS"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/windeployqt"
        #COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
        COMMAND windeployqt --dir "${CMAKE_BINARY_DIR}/windeployqt" --release --no-compiler-runtime --qmldir ${QMLDIR} "$<TARGET_FILE_DIR:fgfs>/$<TARGET_FILE_NAME:fgfs>"
    )

    # copy deployment directory during installation
    install(
        DIRECTORY "${CMAKE_BINARY_DIR}/windeployqt/"
        OPTIONAL
        TYPE BIN
    )
endif()

if (APPLE)
    add_custom_target(macdeploy
        COMMENT "Running macdeployqt on FGFS"
        #COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
        COMMAND ${FG_QT_BIN_DIR}/macdeployqt "$<TARGET_BUNDLE_DIR:fgfs>" -qmldir=${QMLDIR}
    )
endif()

