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

#include "igotu/igotuconfig.h"
#include "igotu/utils.h"

#include "configurationdialog.h"
#include "ui_configurationdialog.h"

using namespace igotu;

class ConfigurationDialogPrivate : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void on_interval_timeChanged(const QTime &time);
    void on_changedInterval_timeChanged(const QTime &time);
    void on_changeEnabled_toggled(bool enabled);
    void on_changeSpeed_valueChanged(int value);

public:
    void syncDialogToConfiguration();

public:
    ConfigurationDialog *p;

    IgotuConfig config;
    ScheduleTableEntry entry;

    boost::scoped_ptr<Ui::ConfigurationDialog> ui;
};

// ConfigurationDialogPrivate ====================================================

void ConfigurationDialogPrivate::on_interval_timeChanged(const QTime &time)
{
    entry.setLogInterval(QTime().secsTo(time));
}


void ConfigurationDialogPrivate::on_changedInterval_timeChanged(const QTime &time)
{
    entry.setChangedLogInterval(QTime().secsTo(time));
}


void ConfigurationDialogPrivate::on_changeEnabled_toggled(bool enabled)
{
    entry.setIntervalChangeSpeed(enabled ? ui->changeSpeed->value() : 0);
}

void ConfigurationDialogPrivate::on_changeSpeed_valueChanged(int value)
{
    entry.setIntervalChangeSpeed(ui->changeEnabled->isChecked() ? value : 0);
}

void ConfigurationDialogPrivate::syncDialogToConfiguration()
{
    ui->interval->setTime(QTime().addSecs(entry.logInterval()));
    ui->changedInterval->setTime(QTime().addSecs(entry.changedLogInterval()));
    const double speed = entry.intervalChangeSpeed();
    ui->changeSpeed->setValue(speed != 0.0 ? speed : 10);
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

IgotuConfig ConfigurationDialog::config() const
{
    IgotuConfig result = d->config;
    result.setScheduleTableEntry(1, 0, d->entry);
    return result;
}

#include "configurationdialog.moc"
