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

#include "mainwindow.h"
#include "ui_igotugui.h"
#include "waitdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMetaMethod>
#include <QPointer>
#include <QPushButton>

using namespace igotu;

class MainWindowPrivate : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void on_reload_clicked();
    void on_save_clicked();

    void on_control_infoStarted();
    void on_control_infoFinished(const QString &info);
    void on_control_infoFailed(const QString &message);

public:
    MainWindow *p;

    boost::scoped_ptr<Ui::IgotuDialog> ui;
    IgotuControl *control;
    WaitDialog *waiter;
};

// copied and modified from qobject.cpp
void connectSlotsByNameToPrivate(QObject *publicObject, QObject *privateObject)
{
    if (!publicObject)
        return;
    const QMetaObject *mo = privateObject->metaObject();
    Q_ASSERT(mo);
    const QObjectList list = qFindChildren<QObject*>(publicObject, QString());
    for (int i = 0; i < mo->methodCount(); ++i) {
        const char *slot = mo->method(i).signature();
        Q_ASSERT(slot);
        if (slot[0] != 'o' || slot[1] != 'n' || slot[2] != '_')
            continue;
        bool foundIt = false;
        for(int j = 0; j < list.count(); ++j) {
            const QObject *co = list.at(j);
            QByteArray objName = co->objectName().toAscii();
            int len = objName.length();
            if (!len || qstrncmp(slot + 3, objName.data(), len) ||
                    slot[len+3] != '_')
                continue;
            const QMetaObject *smo = co->metaObject();
            int sigIndex = smo->indexOfMethod(slot + len + 4);
            if (sigIndex < 0) { // search for compatible signals
                int slotlen = qstrlen(slot + len + 4) - 1;
                for (int k = 0; k < co->metaObject()->methodCount(); ++k) {
                    if (smo->method(k).methodType() != QMetaMethod::Signal)
                        continue;

                    if (!qstrncmp(smo->method(k).signature(), slot + len + 4,
                                slotlen)) {
                        sigIndex = k;
                        break;
                    }
                }
            }
            if (sigIndex < 0)
                continue;
            if (QMetaObject::connect(co, sigIndex, privateObject, i)) {
                foundIt = true;
                break;
            }
        }
        if (foundIt) {
            // we found our slot, now skip all overloads
            while (mo->method(i + 1).attributes() & QMetaMethod::Cloned)
                  ++i;
        } else if (!(mo->method(i).attributes() & QMetaMethod::Cloned)) {
            qWarning("connectSlotsByName: No matching signal for %s", slot);
        }
    }
}

void MainWindowPrivate::on_save_clicked()
{
    /*
    if (contents.isEmpty())
        return;

    IgotuPoints igotuPoints(contents, count);
    QByteArray gpxData = igotuPoints.gpx().toUtf8();

    QString filePath = QFileDialog::getSaveFileName(this, QString(), QString(),
            tr("GPX files (*.gpx)"));

    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, QString(),
                tr("Unable to create file: %1")
                .arg(file.errorString()));
        return;
    }
    if (file.write(gpxData) != gpxData.length())
        QMessageBox::critical(this, QString(),
                tr("Unable to save to file: %1")
                .arg(file.errorString()));
                */
}

void MainWindowPrivate::on_reload_clicked()
{
    control->info();
}

void MainWindowPrivate::on_control_infoStarted()
{
    waiter = new WaitDialog(tr("Retrieving info..."),
            tr("Please wait..."), p);
    waiter->exec();
}

void MainWindowPrivate::on_control_infoFinished(const QString &info)
{
    delete waiter;

    ui->textBrowser->setText(info);
}

void MainWindowPrivate::on_control_infoFailed(const QString &message)
{
    delete waiter;

    ui->textBrowser->clear();
    QMessageBox::critical(p, QString(),
            tr("Unable to connect to gps tracker: %1")
            .arg(message));
}

// MainWindow ==================================================================

MainWindow::MainWindow() :
    d(new MainWindowPrivate)
{
    d->p = this;

    d->ui.reset(new Ui::IgotuDialog);
    d->ui->setupUi(this);

    QPushButton * const saveButton = d->ui->buttonBox->addButton(tr("Save..."),
            QDialogButtonBox::ActionRole);
    saveButton->setObjectName(QLatin1String("save"));
    QPushButton * const reloadButton = d->ui->buttonBox->addButton(tr("Reload"),
            QDialogButtonBox::ActionRole);
    reloadButton->setObjectName(QLatin1String("reload"));

    d->control = new IgotuControl(this);
    d->control->setObjectName(QLatin1String("control"));

    connectSlotsByNameToPrivate(this, d.get());
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    while (!d->control->queuesEmpty()) {
        // We should never come here, as all commands are showing their own
        // dialogs, but anyway
        QPointer<QDialog> waiter(new WaitDialog(tr("Please wait until all tasks are finished..."),
                tr("Please wait..."), this));
        d->control->notify(waiter, "reject");
        waiter->exec();
        delete waiter;
    }
}

#include "mainwindow.moc"
