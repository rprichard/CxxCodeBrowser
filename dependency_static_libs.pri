# It makes sense to use this include file from static library pro files, not
# just from executables.  In a static library, it will add the dependency's
# include files to INCLUDEPATH and DEPENDPATH, and (assuming the library and
# executable are configured with create_prl and link_prl), the library
# archive's path will be propagated to the executable's link command-line.

for(dep, DEPENDENCY_STATIC_LIBS) {
    # Currently each dependency has this structure:
    #   <output-tree>/PATH_TO_LIB/BASENAME/BASENAME.a
    #   <source-tree>/PATH_TO_LIB/BASENAME [all sources and headers]
    # In the future, it's conceivable that some third-party library would have
    # a separate include directory:
    #   <output-tree>/PATH_TO_LIB/BASENAME/BASENAME.a
    #   <source-tree>/PATH_TO_LIB/BASENAME/src     [sources]
    #   <source-tree>/PATH_TO_LIB/BASENAME/include [headers]
    # This case could be handled in this pri by using the exists function,
    # e.g.:
    #     exists($${_PRO_FILE_PWD_}/$${dep}/include) {
    #         use $${dep}/include as the header path...
    #     } else {
    #         use $${dep} as normal...
    #     }
    LIBS += $${OUT_PWD}/$${dep}/$$basename(dep).a
    INCLUDEPATH += $${dep}
    DEPENDPATH += $${dep}
}
