clebsCheck(buildplugin) {
    CLEBS_DEPENDENCIES *= buildplugin
}

clebsDependency(buildplugin) {
    unix|macx:CONFIG += hide_symbols
    CONFIG += plugin dll
    TEMPLATE = lib
    target.path = $$PLUGINDIR
    INSTALLS *= target
}
