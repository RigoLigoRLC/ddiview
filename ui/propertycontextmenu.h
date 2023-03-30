#ifndef PROPERTYCONTEXTMENU_H
#define PROPERTYCONTEXTMENU_H

#include <QMenu>
#include <QListWidgetItem>

class PropertyContextMenu : public QMenu
{
    Q_OBJECT
public:
    PropertyContextMenu(QWidget *parent, QListWidgetItem *item);

private:
    QAction *m_actCopyAsDisplayed,
            *m_actCopyAsHexSeq,
            *m_actDispPropName;

    QListWidgetItem *m_item;

private slots:
    void CopyAsDisplayed(bool checked);
    void CopyAsHexSequence(bool checked);
};

#endif // PROPERTYCONTEXTMENU_H
