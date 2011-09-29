#include "graphdelegate.h"
#include <QComboBox>

GraphDelegate::GraphDelegate(GraphSettingsModel *gsm, QObject *parent)
   : QItemDelegate(parent)
{
    gsmodel = gsm;
}


QWidget *GraphDelegate::createEditor(QWidget *parent,
    const QStyleOptionViewItem &/*option*/,
    const QModelIndex &/*index*/) const
{
    QComboBox *editor = new QComboBox(parent);
    for(int i = 0; i < gsmodel->getPlots().count(); i++)
    {
        QString text = QString("Graph %1").arg(i+1);
        editor->insertItem(i, text);
    }
    return editor;
}

void GraphDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    QString value = index.data().toString();

    QComboBox *comboBox = static_cast<QComboBox*>(editor);

    if(QString::compare(value,"") == 0) comboBox->setCurrentIndex(index.row());
    else
    {
        for(int i = 0; i < comboBox->count(); i++)
        {
            if(comboBox->itemText(i) == value)
                comboBox->setCurrentIndex(i);
        }
    }
}

void GraphDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    QString value = comboBox->currentText();

    model->setData(index, value, Qt::EditRole);
}

void GraphDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}