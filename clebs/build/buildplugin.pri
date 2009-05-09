clebsCheck(buildplugin) {
    CLEBS_DEPENDENCIES *= buildplugin
}

clebsDependency(buildplugin) {
    unix:!novisibility:CONFIG += hide_symbols
    CONFIG += plugin dll
    TEMPLATE = lib
    target.path = $$PLUGINDIR
    INSTALLS *= target
}
