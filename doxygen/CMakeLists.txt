find_package(Doxygen REQUIRED)

set(DOXYGEN_PROJECT_LOGO ${PROJECT_SOURCE_DIR}/doc/mboxid_logo.svg)
set(DOXYGEN_FULL_PATH_NAMES NO)
set(DOXYGEN_HIDE_UNDOC_MEMBERS YES)
set(DOXYGEN_HIDE_UNDOC_CLASSES YES)
set(DOXYGEN_QT_AUTOBRIEF YES)

doxygen_add_docs(doxygen 
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    COMMENT "generate documentation"
    )
