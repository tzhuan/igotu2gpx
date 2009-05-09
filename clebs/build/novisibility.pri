clebsCheck(novisibility) {
    CLEBS_DEPENDENCIES *= novisibility
}

clebsDependency(novisibility) {
    CONFIG -= hide_symbols
    CONFIG *= novisibility
}

