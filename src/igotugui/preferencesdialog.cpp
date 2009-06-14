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
    }
}

void PreferencesDialogPrivate::on_device_textEdited(const QString &text)
{
    QSettings().setValue(QLatin1String("device"), text);
    emit p->deviceChanged(text);
}

// PreferencesDialog ===========================================================

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    d(new PreferencesDialogPrivate)
{
    d->p = this;

    d->ui.reset(new Ui::PreferencesDialog);
    d->ui->setupUi(this);

    connectSlotsByNameToPrivate(this, d.get());

    d->ui->device->setText(currentDevice());
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

#include "preferencesdialog.moc"
