/******************************************************************************
 * Copyright (C) 2009  Michael Hofmann <mh21@piware.de>                       *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with this program; if not, write to the Free Software Foundation, Inc.,    *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.                *
 ******************************************************************************/

#include "igotu/igotucontrol.h"
#include "igotu/utils.h"

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QSettings>

using namespace igotu;

#define UPDATE_PREF QLatin1String("Preferences/updateNotification")
#define DEVICE_PREF QLatin1String("Preferences/device")
#define OFFSET_PREF QLatin1String("Preferences/utcOffset")

class PreferencesDialogPrivate : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_device_textEdited(const QString &text);
    void on_utcOffset_currentIndexChanged(int index);
    void on_update_currentIndexChanged(int index);

public:
    static QString currentDevice();
    static int currentUtcOffset();
    static UpdateNotification::Type currentUpdateNotification();
    void syncDialogToPreferences();

private:
    void setCurrentDevice(const QString &device);
    void setCurrentUtcOffset(int offset);
    void setCurrentUpdateNotification(UpdateNotification::Type type);

public:
    PreferencesDialog *p;

    boost::scoped_ptr<Ui::PreferencesDialog> ui;
};

// PreferencesDialogPrivate ====================================================

void PreferencesDialogPrivate::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole) {
        QSettings().remove(QLatin1String("Preferences"));
        setCurrentDevice(IgotuControl::defaultDevice());
        setCurrentUtcOffset(IgotuControl::defaultUtcOffset());
        setCurrentUpdateNotification
            (UpdateNotification::defaultUpdateNotification());
        syncDialogToPreferences();
    }
}

void PreferencesDialogPrivate::on_device_textEdited(const QString &text)
{
    setCurrentDevice(text);
}

void PreferencesDialogPrivate::on_utcOffset_currentIndexChanged(int index)
{
    const int seconds = index < -1 ? 0 : ui->utcOffset->itemData(index).toInt();
    setCurrentUtcOffset(seconds);
}

void PreferencesDialogPrivate::on_update_currentIndexChanged(int index)
{
    const int type = index < -1 ? 0 : ui->update->itemData(index).toInt();
    setCurrentUpdateNotification(UpdateNotification::Type(type));
}

void PreferencesDialogPrivate::setCurrentDevice(const QString &device)
{
    if (device != IgotuControl::defaultDevice())
        QSettings().setValue(DEVICE_PREF, device);
    else
        QSettings().remove(DEVICE_PREF);
    emit p->deviceChanged(device);
}

QString PreferencesDialogPrivate::currentDevice()
{
    return QSettings().value(DEVICE_PREF,
            IgotuControl::defaultDevice()).toString();
}

void PreferencesDialogPrivate::setCurrentUtcOffset(int offset)
{
    if (offset != IgotuControl::defaultUtcOffset())
        QSettings().setValue(OFFSET_PREF, offset);
    else
        QSettings().remove(OFFSET_PREF);
    emit p->utcOffsetChanged(offset);
}

int PreferencesDialogPrivate::currentUtcOffset()
{
    return QSettings().value(OFFSET_PREF,
            IgotuControl::defaultUtcOffset()).toInt();
}

void PreferencesDialogPrivate::setCurrentUpdateNotification
        (UpdateNotification::Type type)
{
    // Always save this value, do not use the default value as it can change
    // between devel and stable versions
    QSettings().setValue(UPDATE_PREF, QLatin1String(enumValueToKey
                (UpdateNotification::staticMetaObject, "Type", type)));
    emit p->updateNotificationChanged(type);
}

UpdateNotification::Type PreferencesDialogPrivate::currentUpdateNotification()
{
    // Store default value as it can change between devel and stable versions
    if (!QSettings().contains(UPDATE_PREF))
        QSettings().setValue(UPDATE_PREF, QLatin1String(enumValueToKey
                    (UpdateNotification::staticMetaObject, "Type",
                     UpdateNotification::defaultUpdateNotification())));

    return UpdateNotification::Type(enumKeyToValue
            (UpdateNotification::staticMetaObject, "Type",
             QSettings().value(UPDATE_PREF).toString().toAscii()));
}

void PreferencesDialogPrivate::syncDialogToPreferences()
{
    ui->utcOffset->setCurrentIndex
        (ui->utcOffset->findData(currentUtcOffset()));
    ui->device->setText(currentDevice());
    ui->update->setCurrentIndex(ui->update->findData
            (currentUpdateNotification()));
}

// PreferencesDialog ===========================================================

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    d(new PreferencesDialogPrivate)
{
    d->p = this;

    d->ui.reset(new Ui::PreferencesDialog);
    d->ui->setupUi(this);

    const int timeZones[] = { -1200, -1100, -1000, -900, -930, -800, -700, -600,
        -500, -400, -300, -330, -200, -230, -100, +0, +100, +200, +300, +330,
        +400, +430, +500, +530, +545, +600, +630, +700, +800, +900, +930, +1000,
        +1030, +1100, +1130, +1200, +1245, +1300, +1345};
    const unsigned timeZoneCount = sizeof(timeZones) / sizeof(timeZones[0]);

    for (unsigned i = 0; i < timeZoneCount; ++i) {
        const int timeZone = timeZones[i];
        const int seconds = (timeZone / 100) * 3600 + (timeZone % 100) * 60;
        if (timeZone == 0)
            //: Use unicode character U+00B1 PLUS-MINUS SIGN
            d->ui->utcOffset->addItem(trUtf8("GMT±%1:%2")
                    .arg(0, 2, 10, QLatin1Char('0'))
                    .arg(0, 2, 10, QLatin1Char('0')),
                    seconds);
        else if (timeZone < 0)
            //: Use unicode character U+2212 MINUS SIGN
            d->ui->utcOffset->addItem(trUtf8("GMT−%1:%2")
                    .arg(-timeZone / 100, 2, 10, QLatin1Char('0'))
                    .arg(-timeZone % 100, 2, 10, QLatin1Char('0')),
                    seconds);
        else
            d->ui->utcOffset->addItem(tr("GMT+%1:%2")
                    .arg(timeZone / 100, 2, 10, QLatin1Char('0'))
                    .arg(timeZone % 100, 2, 10, QLatin1Char('0')),
                    seconds);
    }

    d->ui->update->addItem(tr("Never", "no update notification"),
            UpdateNotification::NotifyNever);
    d->ui->update->addItem(tr("Stable releases"),
            UpdateNotification::StableReleases);
    d->ui->update->addItem(tr("Development snapshots"),
            UpdateNotification::DevelopmentSnapshots);

    d->syncDialogToPreferences();

    connectSlotsByNameToPrivate(this, d.get());
}

PreferencesDialog::~PreferencesDialog()
{
}

QString PreferencesDialog::currentDevice()
{
    return PreferencesDialogPrivate::currentDevice();
}

int PreferencesDialog::currentUtcOffset()
{
    return PreferencesDialogPrivate::currentUtcOffset();
}

UpdateNotification::Type PreferencesDialog::currentUpdateNotification()
{
    return PreferencesDialogPrivate::currentUpdateNotification();
}

#include "preferencesdialog.moc"
