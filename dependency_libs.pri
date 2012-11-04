for(dep, DEPENDENCY_LIBS) {
    PRE_TARGETDEPS += $${OUT_PWD}/$${dep}
    LIBS           += $${OUT_PWD}/$${dep}
}
