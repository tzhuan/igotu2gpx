/******************************************************************************
 * Copyright (C) 2008  Michael Hofmann <mh21@piware.de>                       *
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

#include "igotu/dataconnection.h"
#include "igotu/fileexporter.h"
#include "igotu/pluginloader.h"
#include "igotu/utils.h"

#include "plugindialog.h"
#include "trackvisualizer.h"
#include "ui_plugindialog.h"

#include <QFileInfo>
#include <QHeaderView>
#include <QPushButton>
#include <QSettings>

using namespace igotu;

class PluginDialogPrivate : public QObject
{
    Q_OBJECT
public:
    void add(const QObject *object, const QString &what, const QString &text,
            const QString &tooltip = QString(),
            const QString &path = QString(),
            const QString &priority = QString());
    void scanPlugins();

public Q_SLOTS:
    void on_refresh_clicked();

public:
    QMap<QString, QTreeWidgetItem*> parents;
    Ui::PluginDialog ui;
};

// PluginDialogPrivate =========================================================

void PluginDialogPrivate::add(const QObject *object, const QString &what,
        const QString &text, const QString &tooltip, const QString &path,
        const QString &priority)
{
    QTreeWidgetItem *parent = parents.value(what);
    if (!parent) {
        parent = new QTreeWidgetItem(ui.treeWidget,
                QStringList() << what);
        parent->setExpanded(true);
        parents.insert(what, parent);
    }

    QString className(PluginDialog::tr("Unknown"));
    QString filePath(path);

    if (object) {
        className = QString::fromLatin1(object->metaObject()->className());
        if (className.endsWith(QLatin1String("Creator")))
            className.resize(className.size() - 7);
        filePath = PluginLoader().pluginPath(object);
    }

    QTreeWidgetItem * const item = new QTreeWidgetItem(parent, QStringList()
            << className
            << text
            << priority
            << QFileInfo(filePath).fileName());

    item->setToolTip(3, filePath);
    if (!tooltip.isEmpty()) {
        item->setToolTip(0, tooltip);
        item->setToolTip(1, tooltip);
        item->setToolTip(2, tooltip);
    }
}

void PluginDialogPrivate::scanPlugins()
{
    ui.treeWidget->clear();
    parents.clear();

    Q_FOREACH (const QObject * const object,
            PluginLoader().allAvailablePlugins()) {
        bool added = false;
        if (const TrackVisualizerCreator * const creator =
                qobject_cast<TrackVisualizerCreator*>(object)) {
            add(object, PluginDialog::tr("Display"),
                    creator->trackVisualizer(), QString(), QString(),
                    QString::number(creator->visualizerPriority()));
            added = true;
        } else if (const DataConnectionCreator * const creator =
                qobject_cast<DataConnectionCreator*>(object)) {
            add(object, PluginDialog::tr("Connection"),
                    creator->dataConnection(), QString(), QString(),
                    QString::number(creator->connectionPriority()));
            added = true;
        } else if (const FileExporter * const exporter =
                qobject_cast<FileExporter*>(object)) {
            add(object, PluginDialog::tr("Exporter"),
                    exporter->formatName(), QString(), QString(),
                    QString::number(exporter->exporterPriority()));
            added = true;
        }
        if (!added) {
            add(object, PluginDialog::tr("Unknown plugins"),
                    PluginDialog::tr("unknown"));
        }
    }

    QStringList ignored;
#if defined(Q_OS_LINUX)
    ignored << QLatin1String("libigotu.so");
#elif defined(Q_OS_WIN32)
    ignored << QLatin1String("igotu.dll");
#elif defined(Q_OS_MACX)
    ignored << QLatin1String("libigotu.1.dylib");
#endif

    QMapIterator<QString, QString> i(PluginLoader().pluginsWithErrors());
    while (i.hasNext()) {
        i.next();
        if (ignored.contains(QFileInfo(i.key()).fileName()))
            continue;
        add(NULL, PluginDialog::tr("Invalid plugins"),
                PluginDialog::tr("unknown"), i.value(), i.key());
    }

    ui.treeWidget->resizeColumnToContents(0);
    ui.treeWidget->resizeColumnToContents(1);
    ui.treeWidget->resizeColumnToContents(2);
    ui.treeWidget->resizeColumnToContents(3);
}

void PluginDialogPrivate::on_refresh_clicked()
{
    QSettings settings(QLatin1String("Trolltech"));
    Q_FOREACH (const QString &group, settings.childGroups())
        if (group.startsWith(QLatin1String("Qt Plugin Cache ")))
            settings.remove(group);
    PluginLoader().reloadPlugins();
    scanPlugins();
}

// PluginDialog ================================================================

PluginDialog::PluginDialog(QWidget *parent) :
    QDialog(parent),
    d(new PluginDialogPrivate)
{
    d->ui.setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    QPushButton *button = d->ui.buttonBox->addButton(tr("Refresh plugins"),
            QDialogButtonBox::ActionRole);
    button->setObjectName(QLatin1String("refresh"));
    connectSlotsByNameToPrivate(this, d.get());

    d->scanPlugins();
}

PluginDialog::~PluginDialog()
{
}

#include "plugindialog.moc"
