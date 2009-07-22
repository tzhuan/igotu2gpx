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

#include "igotu/utils.h"

#include "plugindialog.h"
#include "pluginloader.h"
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
            const QString &path = QString());
    void scanPlugins();

public Q_SLOTS:
    void on_refresh_clicked();

public:
    QMap<QString, QTreeWidgetItem*> parents;
    Ui::PluginDialog ui;
};

// PluginDialogPrivate =========================================================

void PluginDialogPrivate::add(const QObject *object, const QString &what,
        const QString &text, const QString &tooltip, const QString &path)
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
            << QFileInfo(filePath).fileName());

    item->setToolTip(2, filePath);
    if (!tooltip.isEmpty()) {
        item->setToolTip(0, tooltip);
        item->setToolTip(1, tooltip);
    } else {
        item->setToolTip(0, className);
        item->setToolTip(1, text);
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
            Q_FOREACH (const QString &visualizer, creator->trackVisualizers())
                add(object, PluginDialog::tr("Track visualizer"), visualizer);
            added = true;
        }
        if (!added) {
            add(object, PluginDialog::tr("Unknown plugins"),
                    PluginDialog::tr("unknown"));
        }
    }

    QMapIterator<QString, QString> i(PluginLoader().pluginsWithErrors());
    while (i.hasNext()) {
        i.next();
        add(NULL, PluginDialog::tr("Invalid plugins"),
                PluginDialog::tr("unknown"), i.value(), i.key());
    }

    ui.treeWidget->resizeColumnToContents(0);
    ui.treeWidget->resizeColumnToContents(1);
    ui.treeWidget->resizeColumnToContents(2);
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

    d->ui.treeWidget->header()->hide();
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
