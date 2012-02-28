#ifndef GOPATHMODEL_H
#define GOPATHMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QIcon>
#include <QFileInfo>

class GopathModel;
class PathNode
{
public:
    PathNode(GopathModel *model);
    PathNode(GopathModel *model,const QString &path, PathNode *parent);
    ~PathNode();
    PathNode* parent();
    PathNode* child(int row);
    int childCount();
    int row() const;
    QList<PathNode*>* children();
    QString path() const;
    QString text() const;
    QFileInfo fileInfo() const;
    bool isDir() const;
    bool isFile() const;
    void clear();
    void reload();
    PathNode *findPath(const QString &path);
protected:
    GopathModel *m_model;
    PathNode *m_parent;
    QList<PathNode*> *m_children;
    QString m_path;
    QString m_text;
};

class QFileIconProvider;
class QFileSystemWatcher;
class QTreeView;
class GopathModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit GopathModel(QObject *parent = 0);
    ~GopathModel();
    void setPathList(const QStringList &pathList);
    QList<QModelIndex> findPath(const QString &path) const;
    QList<QModelIndex> findFile(const QString &path) const;
    QString filePath(const QModelIndex &index) const;
    PathNode *nodeFromIndex(const QModelIndex &index) const;    
    void setStartIndex(const QModelIndex &index);
    QModelIndex startIndex() const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual QModelIndex index(int row, int column,const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;    
    QFileSystemWatcher* fileWatcher() const;
signals:
    
public slots:
    void directoryChanged(const QString&);
protected:
    QModelIndex findPathHelper(const QString &path, const QModelIndex &parentIndex) const;
    QModelIndex findFileHelper(const QString &path, const QModelIndex &parentIndex) const;
    QStringList m_pathList;
    PathNode *m_rootNode;
    QModelIndex m_startIndex;
    QFileIconProvider *m_iconProvider;
    QFileSystemWatcher *m_fileWatcher;
    QTreeView *m_treeView;
};

#endif // GOPATHMODEL_H