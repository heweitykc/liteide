/**************************************************************************
** This file is part of LiteIDE
**
** Copyright (c) 2011 LiteIDE Team. All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** In addition, as a special exception,  that plugins developed for LiteIDE,
** are allowed to remain closed sourced and can be distributed under any license .
** These rights are included in the file LGPL_EXCEPTION.txt in this package.
**
**************************************************************************/
// Module: filebrowser.cpp
// Creator: visualfc <visualfc@gmail.com>
// date: 2011-6-21
// $Id: filebrowser.cpp,v 1.0 2011-7-12 visualfc Exp $

#include "filebrowser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QTreeView>
#include <QHeaderView>
#include <QDirModel>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QToolBar>
#include <QAction>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QProcess>
#include <QDebug>
//lite_memory_check_begin
#if defined(WIN32) && defined(_MSC_VER) &&  defined(_DEBUG)
     #define _CRTDBG_MAP_ALLOC
     #include <stdlib.h>
     #include <crtdbg.h>
     #define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
     #define new DEBUG_NEW
#endif
//lite_memory_check_end

class QSortFileSystemProxyModel : public QSortFilterProxyModel
{
public:
    QSortFileSystemProxyModel(QObject *parent) :
        QSortFilterProxyModel(parent)
    {
    }
    virtual bool lessThan( const QModelIndex & left, const QModelIndex & right ) const
    {
        QFileSystemModel *model = static_cast<QFileSystemModel*>(this->sourceModel());
        QFileInfo l = model->fileInfo(left);
        QFileInfo r = model->fileInfo(right);
        if (l.isDir() && r.isFile()) {
            return true;
        } else if (l.isFile() && r.isDir()) {
            return false;
        }
#ifdef Q_OS_WIN
        if (l.filePath().length() <= 3 || r.filePath().length() <= 3) {
            return l.filePath().at(0) < r.filePath().at(0);
        }
#endif
        return (l.fileName().compare(r.fileName(),Qt::CaseInsensitive) < 0);
    }
};

FileBrowser::FileBrowser(LiteApi::IApplication *app, QObject *parent) :
    QObject(parent),
    m_liteApp(app)
{
    m_widget = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setMargin(0);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    m_proxyModel = new QSortFileSystemProxyModel(this);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSourceModel(m_fileModel);
    m_proxyModel->sort(0);

    //create filter toolbar
    m_filterToolBar = new QToolBar(m_widget);
    m_filterToolBar->setIconSize(QSize(16,16));

    m_syncAct = new QAction(QIcon(":/images/sync.png"),tr("Synchronize with editor"),this);
    m_syncAct->setCheckable(true);

    m_filterCombo = new QComboBox;
    m_filterCombo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    m_filterCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    m_filterCombo->setEditable(true);
    m_filterCombo->addItem("*");
    m_filterCombo->addItem("Makefile;*.go;*.cgo;*.s;*.goc;*.y;*.e64;*.pro");
    m_filterCombo->addItem("*.sh;Makefile;*.go;*.cgo;*.s;*.goc;*.y;*.*.c;*.cpp;*.h;*.hpp;*.e64;*.pro");

    m_filterToolBar->addAction(m_syncAct);
    m_filterToolBar->addSeparator();
    m_filterToolBar->addWidget(m_filterCombo);

    //create root toolbar
    m_rootToolBar = new QToolBar(m_widget);
    m_rootToolBar->setIconSize(QSize(16,16));

    m_cdupAct = new QAction(QIcon(":/images/cdup.png"),tr("open to parent"),this);

    m_rootCombo = new QComboBox;
    m_rootCombo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    m_rootCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    m_rootCombo->setEditable(false);
    m_rootCombo->addItem(m_fileModel->myComputer().toString());

    m_rootToolBar->addAction(m_cdupAct);
    m_rootToolBar->addSeparator();
    m_rootToolBar->addWidget(m_rootCombo);

    //create treeview
    m_treeView = new QTreeView;
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setModel(m_proxyModel);

    m_treeView->setRootIsDecorated(true);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setTextElideMode(Qt::ElideNone);
    m_treeView->setAttribute(Qt::WA_MacShowFocusRect, false);

    m_treeView->setHeaderHidden(true);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // show horizontal scrollbar
    m_treeView->header()->setResizeMode(QHeaderView::ResizeToContents);
    m_treeView->header()->setStretchLastSection(false);
    //hide
    int count = m_treeView->header()->count();
    for (int i = 1; i < count; i++) {
        m_treeView->setColumnHidden(i,true);
    }

    mainLayout->addWidget(m_filterToolBar);
    mainLayout->addWidget(m_rootToolBar);
    mainLayout->addWidget(m_treeView);
    m_widget->setLayout(mainLayout);

    //create menu
    m_fileMenu = new QMenu;
    m_folderMenu = new QMenu;
    m_rootMenu = new QMenu;

    m_openFileAct = new QAction(tr("Open File"),this);
    m_newFileAct = new QAction(tr("New File"),this);
    m_renameFileAct = new QAction(tr("Rename File"),this);
    m_removeFileAct = new QAction(tr("Remove File"),this);

    m_setRootAct = new QAction(tr("Set Folder To Root"),this);
    m_newFolderAct = new QAction(tr("New Folder"),this);
    m_renameFolderAct = new QAction(tr("Rename Folder"),this);
    m_removeFolderAct = new QAction(tr("Remove Folder"),this);

    m_openShellAct = new QAction(tr("Open Terminal Here"),this);

    m_fileMenu->addAction(m_openFileAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_newFileAct);
    m_fileMenu->addAction(m_renameFileAct);
    m_fileMenu->addAction(m_removeFileAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_openShellAct);

    m_folderMenu->addAction(m_setRootAct);
    m_folderMenu->addSeparator();
    m_folderMenu->addAction(m_newFileAct);
    m_folderMenu->addAction(m_newFolderAct);
    m_folderMenu->addAction(m_renameFolderAct);
    m_folderMenu->addAction(m_removeFolderAct);
    m_folderMenu->addSeparator();
    m_folderMenu->addAction(m_openShellAct);

    m_rootMenu->addAction(m_cdupAct);
    m_rootMenu->addSeparator();
    m_rootMenu->addAction(m_newFileAct);
    m_rootMenu->addAction(m_newFolderAct);
    m_rootMenu->addSeparator();
    m_rootMenu->addAction(m_openShellAct);

    connect(m_openFileAct,SIGNAL(triggered()),this,SLOT(openFile()));
    connect(m_newFileAct,SIGNAL(triggered()),this,SLOT(newFile()));
    connect(m_renameFileAct,SIGNAL(triggered()),this,SLOT(renameFile()));
    connect(m_removeFileAct,SIGNAL(triggered()),this,SLOT(removeFile()));
    connect(m_newFolderAct,SIGNAL(triggered()),this,SLOT(newFolder()));
    connect(m_renameFolderAct,SIGNAL(triggered()),this,SLOT(renameFolder()));
    connect(m_removeFolderAct,SIGNAL(triggered()),this,SLOT(removeFolder()));
    connect(m_openShellAct,SIGNAL(triggered()),this,SLOT(openShell()));
    connect(m_setRootAct,SIGNAL(triggered()),this,SLOT(setFolderToRoot()));
    connect(m_cdupAct,SIGNAL(triggered()),this,SLOT(cdUp()));


    QDockWidget *dock = m_liteApp->dockManager()->addDock(m_widget,tr("FileBrowser"));
    connect(dock,SIGNAL(visibilityChanged(bool)),this,SLOT(visibilityChanged(bool)));
    connect(m_treeView,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(doubleClickedTreeView(QModelIndex)));
    connect(m_filterCombo,SIGNAL(activated(QString)),this,SLOT(activatedFilter(QString)));
    connect(m_rootCombo,SIGNAL(activated(QString)),this,SLOT(activatedRoot(QString)));
    connect(m_syncAct,SIGNAL(triggered(bool)),this,SLOT(syncFileModel(bool)));
    connect(m_liteApp->editorManager(),SIGNAL(currentEditorChanged(LiteApi::IEditor*)),this,SLOT(currentEditorChanged(LiteApi::IEditor*)));
    connect(m_treeView,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(treeViewContextMenuRequested(QPoint)));

    QString root = m_liteApp->settings()->value("FileBrowser/root",m_fileModel->myComputer().toString()).toString();
    addFolderToRoot(root);
    bool b = m_liteApp->settings()->value("FileBrowser/sync").toBool();
    if (b) {
        m_syncAct->toggle();
    }
}

FileBrowser::~FileBrowser()
{
    QString root = m_rootCombo->currentText();
    m_liteApp->settings()->setValue("FileBrowser/root",root);
    m_liteApp->settings()->setValue("FileBrowser/sync",m_syncAct->isChecked());
    delete m_fileMenu;
    delete m_folderMenu;
    delete m_rootMenu;
    delete m_widget;
}

void FileBrowser::visibilityChanged(bool)
{
}

void FileBrowser::doubleClickedTreeView(QModelIndex proxyIndex)
{
    QModelIndex index = m_proxyModel->mapToSource(proxyIndex);
    if (!index.isValid()) {
        return;
    }
    if (m_fileModel->isDir(index)) {
        return;
    }
    QString fileName = m_fileModel->filePath(index);
    if (fileName.isEmpty()) {
        return;
    }
    m_liteApp->fileManager()->openEditor(fileName);
}

void FileBrowser::activatedFilter(QString filter)
{
    m_fileModel->setNameFilters(filter.split(";",QString::SkipEmptyParts));
}

void FileBrowser::currentEditorChanged(LiteApi::IEditor *editor)
{
    if (!m_syncAct->isChecked()) {
        return;
    }
    if (!editor) {
        return;
    }
    LiteApi::IFile *file = editor->file();
    if (!file) {
        return;
    }
    QString fileName = file->fileName();
    if (fileName.isEmpty()) {
        return;
    }
    QString path = QFileInfo(fileName).filePath();
    QModelIndex index = m_fileModel->index(path);
    if (!index.isValid()) {
        return;
    }
    QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
    m_treeView->scrollTo(proxyIndex,QAbstractItemView::EnsureVisible);
    m_treeView->setCurrentIndex(proxyIndex);
}

void FileBrowser::syncFileModel(bool b)
{
    if (b == false) {
        return;
    } else {
        currentEditorChanged(m_liteApp->editorManager()->currentEditor());
    }
}

void FileBrowser::treeViewContextMenuRequested(const QPoint &pos)
{
    QModelIndex proxyIndex = m_treeView->indexAt(pos);
    QModelIndex index = m_proxyModel->mapToSource(proxyIndex);
    m_contextIndex = index;
    QFileInfo info = m_fileModel->fileInfo(index);
    showTreeViewContextMenu(m_treeView->mapToGlobal(pos),info);
}

void FileBrowser::showTreeViewContextMenu(const QPoint &globalPos, const QFileInfo &info)
{
    QMenu *contextMenu = 0;
    if (info.isDir()) {
        contextMenu = m_folderMenu;
    } else if (info.isFile()) {
        contextMenu = m_fileMenu;
    } else {
        contextMenu = m_rootMenu;
    }

    if (contextMenu && contextMenu->actions().count() > 0) {
        contextMenu->popup(globalPos);
    }
}

void FileBrowser::openFile()
{
    QString fileName = m_fileModel->filePath(m_contextIndex);
    if (!fileName.isEmpty()) {
        m_liteApp->fileManager()->openFile(fileName);
    }
}

void FileBrowser::newFile()
{
    QDir dir = contextDir();

    QString fileName = QInputDialog::getText(m_liteApp->mainWindow(),tr("Create File"),tr("File Name"));
    if (!fileName.isEmpty()) {
        QString filePath = QFileInfo(dir,fileName).filePath();
        if (QFile::exists(filePath)) {
            QMessageBox::information(m_liteApp->mainWindow(),tr("Create File"),
                                     tr("The filename is exists!"));
        } else {
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.close();
            } else {
                QMessageBox::information(m_liteApp->mainWindow(),tr("Create File"),
                                         tr("Failed to create the file!"));
            }
        }
    }
}

void FileBrowser::renameFile()
{
    QFileInfo info = contextFileInfo();
    if (!info.isFile()) {
        return;
    }
    QString fileName = QInputDialog::getText(m_liteApp->mainWindow(),
                                             tr("Rename File"),tr("File Name"),
                                             QLineEdit::Normal,info.fileName());
    if (!fileName.isEmpty() && fileName != info.fileName()) {
        QDir dir = contextDir();
        if (!QFile::rename(info.filePath(),QFileInfo(dir,fileName).filePath())) {
            QMessageBox::information(m_liteApp->mainWindow(),tr("Rename File"),
                                     tr("Failed to rename the file!"));
        }
    }
}

void FileBrowser::removeFile()
{
    QFileInfo info = contextFileInfo();
    if (!info.isFile()) {
        return;
    }

    int ret = QMessageBox::question(m_liteApp->mainWindow(),tr("Remove File"),
                          tr("Confirm remove the file and continue"),
                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (!QFile::remove(info.filePath())) {
            QMessageBox::information(m_liteApp->mainWindow(),tr("Remove File"),
                                     tr("Failed to remove the file!"));
        }
    }
}

void FileBrowser::newFolder()
{
    QString folderName = QInputDialog::getText(m_liteApp->mainWindow(),tr("Create Folder"),tr("Folder Name"));
    if (!folderName.isEmpty()) {
        QDir dir = contextDir();
        if (!dir.entryList(QStringList() << folderName,QDir::Dirs).isEmpty()) {
            QMessageBox::information(m_liteApp->mainWindow(),tr("Create Folder"),
                                     tr("The folder name is exists!"));
        } else if (!dir.mkpath(folderName)) {
            QMessageBox::information(m_liteApp->mainWindow(),tr("Create Folder"),
                                     tr("Failed to create the folder!"));
        }
    }
}

void FileBrowser::renameFolder()
{
    QFileInfo info = contextFileInfo();
    if (!info.isDir()) {
        return;
    }

    QString folderName = QInputDialog::getText(m_liteApp->mainWindow(),
                                               tr("Rename Folder"),tr("Folder Name"),
                                               QLineEdit::Normal,info.fileName());
    if (!folderName.isEmpty() && folderName != info.fileName()) {
        QDir dir = contextDir();
        dir.cdUp();
        if (!dir.rename(info.fileName(),folderName)) {
            QMessageBox::information(m_liteApp->mainWindow(),tr("Rename Folder"),
                                     tr("Failed to rename the folder!"));
        }
    }
}

void FileBrowser::removeFolder()
{
    QFileInfo info = contextFileInfo();
    if (!info.isDir()) {
        return;
    }

    int ret = QMessageBox::warning(m_liteApp->mainWindow(),tr("Remove Folder"),
                          tr("Confirm remove the foler and continue"),
                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (!m_fileModel->rmdir(m_contextIndex)) {
            QMessageBox::information(m_liteApp->mainWindow(),tr("Remove Folder"),
                                     tr("Failed to remove the folder!"));
        }
    }
}

QString FileBrowser::getShellCmd(LiteApi::IApplication *app)
{
    QString defCmd;
#if defined(Q_OS_WIN)
    defCmd = "cmd.exe";
#elif defined(Q_OS_MAC)
    defCmd = "/usr/bin/open";
#else
    defCmd = "/usr/bin/gnome-terminal";
#endif
    return app->settings()->value("filebrowser/shell_cmd",defCmd).toString();
}

QStringList FileBrowser::getShellArgs(LiteApi::IApplication *app)
{
    QStringList defArgs;
#if defined(Q_OS_MAC)
    defArgs << "-a" << "Terminal";
#endif
    return app->settings()->value("filebrowser/shell_args",defArgs).toStringList();
}

QFileInfo FileBrowser::contextFileInfo() const
{
    if (m_contextIndex.isValid()) {
        return m_fileModel->fileInfo(m_contextIndex);
    }
    return QFileInfo(m_fileModel->rootPath());
}

QDir FileBrowser::contextDir() const
{
    if (m_contextIndex.isValid()) {
        if (m_fileModel->isDir(m_contextIndex)) {
            return m_fileModel->filePath(m_contextIndex);
        } else {
            return m_fileModel->fileInfo(m_contextIndex).absoluteDir();
        }
    }
    return m_fileModel->rootDirectory();
}

void FileBrowser::openShell()
{
    QDir dir = contextDir();
    QString cmd = getShellCmd(m_liteApp);
    if (cmd.isEmpty()) {
        return;
    }
    QStringList args = getShellArgs(m_liteApp);
    QString path = dir.path();
#ifdef Q_OS_WIN
    if (path.length() == 2 && path.right(1) == ":") {
        path += "/";
    }
#endif
    QProcess::startDetached(cmd,args,path);
}

void FileBrowser::addFolderToRoot(const QString &path)
{
    int index = -1;
    for (int i = 0; i < m_rootCombo->count(); i++) {
        QString text = m_rootCombo->itemText(i);
        if (text == path) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        m_rootCombo->addItem(path);
        index = m_rootCombo->count()-1;
    }
    m_rootCombo->setCurrentIndex(index);
    activatedRoot(path);
}

void FileBrowser::setFolderToRoot()
{
    QDir dir = contextDir();
    addFolderToRoot(dir.path());
}

void FileBrowser::activatedRoot(QString path)
{
    QModelIndex index = m_fileModel->setRootPath(path);
    QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
    m_treeView->setRootIndex(proxyIndex);
}

void FileBrowser::cdUp()
{
    QDir dir = m_fileModel->rootDirectory();
    if (!dir.path().isEmpty() && dir.cdUp()) {
        addFolderToRoot(dir.path());
    }
}
