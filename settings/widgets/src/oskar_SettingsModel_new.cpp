/*
 * Copyright (c) 2015, The University of Oxford
 * All rights reserved.
 *
 * This file is part of the OSKAR package.
 * Contact: oskar at oerc.ox.ac.uk
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Oxford nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <oskar_SettingsModel_new.hpp>
#include <oskar_SettingsTree.hpp>
#include <oskar_settings_types.hpp>

#include <QtGui/QApplication>
#include <QtGui/QFontMetrics>
#include <QtGui/QIcon>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QModelIndex>
#include <QtCore/QSize>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

using namespace std;

namespace oskar {

SettingsModel::SettingsModel(SettingsTree* settings, QObject* parent)
: QAbstractItemModel(parent),
  settings_(settings),
  lastModified_(QDateTime::currentDateTime()),
  displayKey_(false)
{
}

SettingsModel::~SettingsModel()
{
}

void SettingsModel::beginReset()
{
    beginResetModel();
}

void SettingsModel::endReset()
{
    endResetModel();
}

int SettingsModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 2;
}

QVariant SettingsModel::data(const QModelIndex& index, int role) const
{
    // Get a pointer to the item.
    if (!index.isValid())
        return QVariant();

    const SettingsNode* node = get_node(index);
    const string key = node->key();

    // Check for roles common to all columns.
    switch (role)
    {
    case Qt::ForegroundRole:
    {
        bool disabled = !settings_->dependencies_satisfied(key);
        if (disabled)
            return QColor(Qt::lightGray);
        else if (settings_->is_critical(key))
            return QColor(Qt::white);
        else if (node->value_or_child_set())
            return QColor(Qt::blue);
        else if (node->is_required())
            return QColor(Qt::red);
        return QColor(64, 64, 64);
    }
    case Qt::BackgroundRole:
    {
        bool disabled = !settings_->dependencies_satisfied(key);
        if (settings_->is_critical(key) && !disabled)
        {
            if (index.column() == 0)
                return QColor(0, 48, 255, 160);
            else if (node->item_type() != SettingsItem::LABEL)
                return QColor(255, 64, 64, 255);
        }
        if (index.column() == 1)
            return QColor(0, 0, 192, 12);
        break;
    }
    case Qt::ToolTipRole:
    {
        QString tooltip = QString::fromStdString(node->description());
        if (!tooltip.isEmpty())
        {
            tooltip = "<p>" + tooltip + "</p>";
            if (node->is_required())
                tooltip.append(" [Required]");
//            if (node->item_type() == SettingsItem::SETTING)
//                tooltip.append(" [" + QString::fromStdString(
//                        node->value().type_name()) + "]");
        }
        return tooltip;
    }
    case KeyRole:
        return QString::fromStdString(node->key());
    case ValueRole:
        return QString::fromStdString(node->value().get_value());
    case DefaultRole:
        return QString::fromStdString(node->value().get_default());
    case TypeRole:
        return node->value().type();
    case ItemTypeRole:
        return node->item_type();
    case RangeRole:
    {
        const SettingsValue& value = node->value();
        QList<QVariant> range;
        switch (value.type())
        {
        case SettingsValue::INT_RANGE:
        {
            range.append(value.get<IntRange>().min());
            range.append(value.get<IntRange>().max());
            return range;
        }
        case SettingsValue::DOUBLE_RANGE:
        {
            range.append(value.get<DoubleRange>().min());
            range.append(value.get<DoubleRange>().max());
            return range;
        }
        default:
            break;
        }
        return range;
    }
    case ExtRangeRole:
    {
        const SettingsValue& value = node->value();
        QList<QVariant> range;
        switch (value.type())
        {
        case SettingsValue::INT_RANGE_EXT:
        {
            range.append(value.get<IntRangeExt>().min());
            range.append(value.get<IntRangeExt>().max());
            range.append(QString::fromStdString(value.get<IntRangeExt>().ext_min()));
            range.append(QString::fromStdString(value.get<IntRangeExt>().ext_max()));
            return range;
        }
        case SettingsValue::DOUBLE_RANGE_EXT:
        {
            range.append(value.get<DoubleRangeExt>().min());
            range.append(value.get<DoubleRangeExt>().max());
            range.append(QString::fromStdString(value.get<DoubleRangeExt>().ext_min()));
            range.append(QString::fromStdString(value.get<DoubleRangeExt>().ext_max()));
            return range;
        }
        default:
            break;
        }
        return range;
    }
    case OptionsRole:
    {
        const SettingsValue& value = node->value();
        QStringList options;
        if (value.type() == SettingsValue::OPTION_LIST)
        {
            const OptionList& l = value.get<OptionList>();
            for (int i = 0; i < l.size(); ++i)
                options.push_back(QString::fromStdString(l.option(i)));
        }
        return options;
    }
    default:
        break;
    }

    // Check for roles in specific columns.
    if (index.column() == 0)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            if (displayKey_)
                return QString::fromStdString(node->key());
            return QString::fromStdString(node->label());
        }
        case Qt::DecorationRole:
        {
            // Note: Maybe icons should be disabled unless there is an icon
            // for everything. This would avoid indentation level problems with
            // option trees of depth greater than 1.
            //
            // Alternatively, figure out how to move the icon to the
            // right-hand end of the label?
            if (node->value().type() == SettingsValue::INPUT_FILE ||
                    node->value().type() == SettingsValue::INPUT_FILE_LIST ||
                    node->value().type() == SettingsValue::INPUT_DIRECTORY)
                return QIcon(":/icons/open.png");
            else if (node->value().type() == SettingsValue::OUTPUT_FILE)
                return QIcon(":/icons/save.png");
            break;
        }
        default:
            break;
        }
    }
    else if (index.column() == 1)
    {
        switch (role)
        {
        case Qt::DisplayRole:
            if (node->item_type() == SettingsItem::SETTING) {
                switch (node->value().type())
                {
                    default:
                        return QString::fromStdString(node->value().get_value());
                }
            }
            break;
        case Qt::EditRole:
            if (node->item_type() == SettingsItem::SETTING) {
                return QString::fromStdString(node->value().get_value());
            }
            break;
        case Qt::CheckStateRole:
        {
            if (node->value().type() == SettingsValue::BOOL)
            {
                QVariant val = node->value().get<Bool>().value();
                return val.toBool() ? Qt::Checked : Qt::Unchecked;
            }
            break;
        }
        case Qt::SizeHintRole:
        {
            int width = QApplication::fontMetrics().width(
                    QString::fromStdString(node->label())) + 10;
            return QSize(width, 26);
        }
        default:
            break;
        }
    }

    return QVariant();
}

Qt::ItemFlags SettingsModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    const SettingsNode* node = get_node(index);
    if (!settings_->dependencies_satisfied(node->key()))
        return Qt::ItemIsSelectable;
    if (index.column() == 0 || node->item_type() == SettingsItem::LABEL)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == 1 && node->value().type() == SettingsValue::BOOL)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable |  Qt::ItemIsUserCheckable;

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant SettingsModel::headerData(int section,
        Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        if (section == 0)
            return "Setting";
        else if (section == 1)
            return "Value";
    }

    return QVariant();
}

QModelIndex SettingsModel::index(int row, int column,
        const QModelIndex& parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    const SettingsNode* child_node = get_node(parent)->child(row);
    // FIXME(BM) find way to avoid the const cast?!
    SettingsNode* node = const_cast<SettingsNode*>(child_node);
    if (child_node)
        return createIndex(row, column, static_cast<void*>(node));
    else
        return QModelIndex();
}

void SettingsModel::load_settings_file(const QString& filename)
{
    if (!filename.isEmpty()) filename_ = filename;
    vector<pair<string, string> > failed;
    settings_->load(failed, filename.toStdString());
    refresh(QModelIndex());
}

void SettingsModel::save_settings_file(const QString& filename)
{
    if (!filename.isEmpty()) filename_ = filename;
    settings_->save(filename.toStdString());
}

QModelIndex SettingsModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    const SettingsNode* parent = get_node(index)->parent();
    if (parent == settings_->root_node())
        return QModelIndex();

    SettingsNode* node = const_cast<SettingsNode*>(parent);
    return createIndex(parent->child_number(), 0, static_cast<void*>(node));
}

void SettingsModel::refresh()
{
    refresh(QModelIndex());
}

int SettingsModel::rowCount(const QModelIndex& parent) const
{
    return get_node(parent)->num_children();
}

bool SettingsModel::setData(const QModelIndex& idx, const QVariant& value,
                            int role)
{
    // Check for roles that do not depend on the index.
    if (role == CheckExternalChangesRole)
    {
        if (!QFile::exists(filename_)) return false;
        QFileInfo fileInfo(filename_);
        if (fileInfo.lastModified() > lastModified_.addMSecs(200))
        {
            load_settings_file(filename_);
            lastModified_ = QDateTime::currentDateTime();
            emit fileReloaded();
        }
        return true;
    }
    else if (role == DisplayKeysRole)
    {
        displayKey_ = value.toBool();
        refresh(QModelIndex());
        return true;
    }

    if (!idx.isValid())
        return false;

    // Get a pointer to the item.
    const SettingsNode* node = get_node(idx);

    // Get model indexes for the row.
    QModelIndex topLeft = idx.sibling(idx.row(), 0);
    QModelIndex bottomRight = idx.sibling(idx.row(), columnCount() - 1);

    // Check for role type.
    if (role == Qt::EditRole || role == Qt::CheckStateRole)
    {
        QVariant data = value;
        if (role == Qt::CheckStateRole)
            data = value.toBool() ? QString("true") : QString("false");

        if (idx.column() == 1)
        {
            lastModified_ = QDateTime::currentDateTime();
            std::string value;
            if (node->value().type() == SettingsValue::INPUT_FILE_LIST) {
                QStringList l = data.toStringList();
                for (int i = 0; i < l.size(); ++i) {
                    value += l[i].toStdString();
                    if (i < l.size()) value += ",";
                }
            }
            else {
                value = data.toString().toStdString();
            }
            settings_->set_value(node->key(), value);

            QModelIndex i(idx);
            while (i.isValid())
            {
                emit dataChanged(i.sibling(i.row(), 0),
                                 i.sibling(i.row(), columnCount()-1));
                i = i.parent();
            }
            emit dataChanged(topLeft, bottomRight);
            return true;
        }
    }
    else if (role == SettingsModel::ResetGroupRole)
    {
        if (idx.column() == 1) {
            reset_group_(node);
            lastModified_ = QDateTime::currentDateTime();
            emit dataChanged(topLeft, bottomRight);
            return true;
        }
    }
    return false;
}


// Private methods.

void SettingsModel::reset_group_(const SettingsNode* node)
{
    // NOTE(BM) if there was a way to the Qt index of the child node
    // could call setData() ...
    // ... could work on the QModelIndex methods for this?
    for (int i = 0; i < node->num_children(); ++i) {
        const SettingsNode* child = node->child(i);
        settings_->set_value(child->key(), child->value().get_default());
        reset_group_(child);
    }
}

const SettingsNode* SettingsModel::get_node(const QModelIndex& index) const
{
    if (index.isValid())
    {
        SettingsNode* node = static_cast<SettingsNode*>(index.internalPointer());
        if (node) return node;
    }
    return settings_->root_node();
}

void SettingsModel::refresh(const QModelIndex& parent)
{
    int rows = rowCount(parent);
    for (int i = 0; i < rows; ++i)
    {
        QModelIndex idx = index(i, 0, parent);
        if (idx.isValid())
        {
            emit dataChanged(idx, idx.sibling(idx.row(), 1));
            refresh(idx);
        }
    }
}


SettingsModelFilter::SettingsModelFilter(QObject* parent)
: QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

SettingsModelFilter::~SettingsModelFilter()
{
}

QVariant SettingsModelFilter::data(const QModelIndex& index, int role) const
{
    if (!filterRegExp().isEmpty())
    {
        if (role == Qt::BackgroundRole && index.column() == 0)
        {
            QString label = QSortFilterProxyModel::data(index,
                    Qt::DisplayRole).toString();
            if (label.contains(filterRegExp().pattern(), Qt::CaseInsensitive))
                return QColor("#FFFF9F");
        }
    }
    return QSortFilterProxyModel::data(index, role);
}

// Protected methods.

bool SettingsModelFilter::filterAcceptsChildren(int sourceRow,
        const QModelIndex& sourceParent) const
{
    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!idx.isValid())
        return false;

    int childCount = idx.model()->rowCount(idx);
    for (int i = 0; i < childCount; ++i)
    {
        if (filterAcceptsCurrentRow(i, idx))
            return true;
        if (filterAcceptsChildren(i, idx))
            return true;
    }
    return false;
}

bool SettingsModelFilter::filterAcceptsCurrentRow(const QModelIndex& idx) const
{
    QString labelCurrent = sourceModel()->data(idx, Qt::DisplayRole).toString();
    return labelCurrent.contains(filterRegExp().pattern(), Qt::CaseInsensitive);
}

bool SettingsModelFilter::filterAcceptsCurrentRow(int sourceRow,
            const QModelIndex& sourceParent) const
{
    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    return filterAcceptsCurrentRow(idx);
}

bool SettingsModelFilter::filterAcceptsRow(int sourceRow,
            const QModelIndex& sourceParent) const
{
    // Check if filter accepts this row.
    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (filterAcceptsCurrentRow(idx))
        return true;

    // Check if filter accepts any parent.
    QModelIndex parent = sourceParent;
    while (parent.isValid())
    {
        if (filterAcceptsCurrentRow(parent.row(), parent.parent()))
            return true;
        parent = parent.parent();
    }

    // Check if filter accepts any child.
    if (filterAcceptsChildren(sourceRow, sourceParent))
        return true;

    return false;
}

} // namespace oskar
