/******************************************************************************
 * Copyright (C) 2009  Michael Hofmann <mh21@mh21.de>                         *
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

#include "igotu/igotuconfig.h"
#include "igotu/utils.h"

#include "configurationdialog.h"
#include "ui_configurationdialog.h"

using namespace igotu;

class ConfigurationDialogPrivate : public QObject
{
public:
    void syncDialogToConfiguration();

public:
    ConfigurationDialog *p;

    IgotuConfig config;
    ScheduleTableEntry entry;

    boost::scoped_ptr<Ui::ConfigurationDialog> ui;
};

// ConfigurationDialogPrivate ====================================================

void ConfigurationDialogPrivate::syncDialogToConfiguration()
{
    ui->interval->setTime(QTime().addSecs(entry.logInterval()));
    ui->changedInterval->setTime(QTime().addSecs(entry.changedLogInterval()));
    const double speed = entry.intervalChangeSpeed();
    ui->changeSpeed->setValue(speed != 0.0 ? qRound(speed) : 10);
    ui->changeEnabled->setChecked(speed != 0.0);
}

// ConfigurationDialog ===========================================================

ConfigurationDialog::ConfigurationDialog(const IgotuConfig &config, QWidget *parent) :
    QDialog(parent),
    d(new ConfigurationDialogPrivate)
{
    d->p = this;

    d->config = config;
    d->entry = config.scheduleTableEntries(1).at(0);
    // TODO fail if (!igotuConfig.isScheduleTableEnabled())
    // TODO getter/setter for the config, copy from igotucontrol

    d->ui.reset(new Ui::ConfigurationDialog);
    d->ui->setupUi(this);

    d->ui->interval->setMinimumTime(QTime().addSecs(1));
    d->ui->interval->setMaximumTime(QTime().addSecs(256));
    d->ui->changedInterval->setMinimumTime(QTime().addSecs(1));
    d->ui->changedInterval->setMaximumTime(QTime().addSecs(256));

    d->syncDialogToConfiguration();

    connectSlotsByNameToPrivate(this, d.get());
}

ConfigurationDialog::~ConfigurationDialog()
{
}

QVariantMap ConfigurationDialog::config() const
{
    QVariantMap result;

    unsigned newLogInterval =
        QTime().secsTo(d->ui->interval->time());
    unsigned newIntervalChangeSpeed =
        d->ui->changeEnabled->isChecked() ? d->ui->changeSpeed->value() : 0;
    unsigned newChangedLogInterval =
        QTime().secsTo(d->ui->changedInterval->time());

    if (d->entry.logInterval() != newLogInterval)
        result.insert(QLatin1String("interval"), newLogInterval);
    if (d->entry.intervalChangeSpeed() != newIntervalChangeSpeed)
        result.insert(QLatin1String("changespeed"), newIntervalChangeSpeed);
    if (d->entry.changedLogInterval() != newChangedLogInterval)
        result.insert(QLatin1String("changedinterval"), newChangedLogInterval);

    return result;
}
