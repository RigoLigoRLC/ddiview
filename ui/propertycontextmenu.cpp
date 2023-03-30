
#include <QGuiApplication>
#include <QClipboard>
#include "chunk/basechunk.h"
#include "propertycontextmenu.h"

PropertyContextMenu::PropertyContextMenu(QWidget *parent, QListWidgetItem* item) :
    QMenu(parent),
    m_item(item)
{
    m_actDispPropName = new QAction(item->text().section('\n', 0, 0), this);
    m_actCopyAsDisplayed = new QAction(tr("Copy value as displayed"), this);
    m_actCopyAsHexSeq = new QAction(tr("Copy as raw hex sequence"), this);

    auto dispFont = m_actDispPropName->font();
    dispFont.setBold(true);
    m_actDispPropName->setFont(dispFont);
    m_actDispPropName->setEnabled(false);

    addAction(m_actDispPropName);
    addSeparator();
    addAction(m_actCopyAsDisplayed);
    addAction(m_actCopyAsHexSeq);

    connect(m_actCopyAsDisplayed, &QAction::triggered,
            this, &PropertyContextMenu::CopyAsDisplayed);
    connect(m_actCopyAsHexSeq, &QAction::triggered,
            this, &PropertyContextMenu::CopyAsHexSequence);
}

void PropertyContextMenu::CopyAsDisplayed(bool checked)
{
    QGuiApplication::clipboard()->setText(m_item->text().section('\n', 1));
}

void PropertyContextMenu::CopyAsHexSequence(bool checked)
{
    QGuiApplication::clipboard()->setText(m_item->data(BaseChunk::ItemPropDataRole).toByteArray().toHex());
}
