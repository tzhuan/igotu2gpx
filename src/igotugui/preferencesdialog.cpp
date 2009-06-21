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

class PreferencesDialogPrivate : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_device_textEdited(const QString &text);
    void on_utcOffset_currentIndexChanged(int index);

public:
    PreferencesDialog *p;

    boost::scoped_ptr<Ui::PreferencesDialog> ui;
};

// PreferencesDialogPrivate ====================================================

void PreferencesDialogPrivate::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole) {
        QSettings().remove(QLatin1String("device"));
        const QString defaultDevice = IgotuControl::defaultDevice();
        ui->device->setText(defaultDevice);
        emit p->deviceChanged(defaultDevice);

        QSettings().remove(QLatin1String("utcOffset"));
        const int defaultUtcOffset = IgotuControl::defaultUtcOffset();
        ui->utcOffset->setCurrentIndex
            (ui->utcOffset->findData(defaultUtcOffset));
        emit p->utcOffsetChanged(defaultUtcOffset);
    }
}

void PreferencesDialogPrivate::on_device_textEdited(const QString &text)
{
    QSettings().setValue(QLatin1String("device"), text);
    emit p->deviceChanged(text);
}

void PreferencesDialogPrivate::on_utcOffset_currentIndexChanged(int index)
{
    const int seconds = index < -1 ? 0 : ui->utcOffset->itemData(index).toInt();

    if (seconds != 0)
        QSettings().setValue(QLatin1String("utcOffset"), seconds);
    else
        QSettings().remove(QLatin1String("utcOffset"));
    emit p->utcOffsetChanged(seconds);
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
            d->ui->utcOffset->addItem(trUtf8("GMT\xc2\xb1%1:%2")
                    .arg(0, 2, 10, QLatin1Char('0'))
                    .arg(0, 2, 10, QLatin1Char('0')),
                    seconds);
        else if (timeZone < 0)
            d->ui->utcOffset->addItem(trUtf8("GMT\xe2\x88\x92%1:%2")
                    .arg(-timeZone / 100, 2, 10, QLatin1Char('0'))
                    .arg(-timeZone % 100, 2, 10, QLatin1Char('0')),
                    seconds);
        else
            d->ui->utcOffset->addItem(tr("GMT+%1:%2")
                    .arg(timeZone / 100, 2, 10, QLatin1Char('0'))
                    .arg(timeZone % 100, 2, 10, QLatin1Char('0')),
                    seconds);
    }

    d->ui->utcOffset->setCurrentIndex
        (d->ui->utcOffset->findData(currentUtcOffset()));

    d->ui->device->setText(currentDevice());

    connectSlotsByNameToPrivate(this, d.get());
}

PreferencesDialog::~PreferencesDialog()
{
}

QString PreferencesDialog::currentDevice()
{
    return QSettings().contains(QLatin1String("device")) ?
        QSettings().value(QLatin1String("device")).toString() :
        IgotuControl::defaultDevice();
}

int PreferencesDialog::currentUtcOffset()
{
    return QSettings().contains(QLatin1String("utcOffset")) ?
        QSettings().value(QLatin1String("utcOffset")).toInt() :
        IgotuControl::defaultUtcOffset();
}

#include "preferencesdialog.moc"
