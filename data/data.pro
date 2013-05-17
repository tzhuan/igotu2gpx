include(../clebs.pri)

TEMPLATE = subdirs

app22icons.files = $$files(icons/22x22/apps/*.png)
app22icons.path = $$ICONDIR/22x22/apps
INSTALLS *= app22icons
app32icons.files = $$files(icons/32x32/apps/*.png)
app32icons.path = $$ICONDIR/32x32/apps
INSTALLS *= app32icons
app48icons.files = $$files(icons/48x48/apps/*.png)
app48icons.path = $$ICONDIR/48x48/apps
INSTALLS *= app48icons
app64icons.files = $$files(icons/64x64/apps/*.png)
app64icons.path = $$ICONDIR/64x64/apps
INSTALLS *= app64icons
app128icons.files = $$files(icons/128x128/apps/*.png)
app128icons.path = $$ICONDIR/128x128/apps
INSTALLS *= app128icons
