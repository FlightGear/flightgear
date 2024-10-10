

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

# disabled for now, because we want to run code-signing
# and other steps with macdeployqt

# if (APPLE)
#     add_custom_target(TARGET fgfs
#         POST_BUILD
#         COMMENT "Running macdeployqt on FGFS"
#         COMMAND Qt::macdeployqt  "$<TARGET_FILE_DIR:fgfs>/$<TARGET_FILE_NAME:fgfs>" --qmldir ${QMLDIR} 
#     )
# endif()

