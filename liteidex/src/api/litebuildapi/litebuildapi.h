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
// Module: liteapi.h
// Creator: visualfc <visualfc@gmail.com>
// date: 2011-3-26
// $Id: liteapi.h,v 1.0 2011-4-22 visualfc Exp $

#ifndef __LITEBUILDAPI_H__
#define __LITEBUILDAPI_H__

#include "liteapi/liteapi.h"
#include <QProcessEnvironment>

namespace LiteApi {

class BuildAction
{  
public:
    BuildAction(): m_output(false) {}
    void setId(const QString &id) { m_id = id; }
    void setKey(const QString &key) { m_key = key; }
    void setCmd(const QString &bin) { m_cmd = bin; }
    void setArgs(const QString &args) { m_args = args; }
    void setSave(const QString &save) { m_save = save; }

    void setOutput(const QString &open) {
        m_output = QVariant(open).toBool();
    }
    void setCodec(const QString &codec) { m_codec = codec; }
    void setRegex(const QString &regex) { m_regex = regex; }
    void setImg(const QString &img) {m_img = img; }
    void setTask(const QStringList &task) { m_task = task; }
    QString id() const { return m_id; }
    QString key() const { return m_key; }
    QString cmd() const { return m_cmd; }
    QString args() const { return m_args; }
    QString save() const { return m_save; }
    bool output() const { return m_output; }
    QString codec() const { return m_codec; }
    QString regex() const { return m_regex; }
    QString img() const { return m_img; }
    QStringList task() const { return m_task; }
    void clear() {
        m_id.clear();
        m_cmd.clear();
        m_key.clear();
        m_args.clear();
        m_codec.clear();
        m_regex.clear();
        m_img.clear();
        m_save.clear();
        m_task.clear();
        m_output = false;
    }
    bool isEmpty() {
        return m_id.isEmpty();
    }

protected:
    QString m_id;
    QString m_key;
    QString m_cmd;
    QString m_args;
    QString m_codec;
    QString m_regex;
    QString m_save;
    QString m_img;
    QStringList m_task;
    bool    m_output;
};

class BuildLookup
{
public:
    BuildLookup() : m_top(1)
    {
    }
    void setMimeType(const QString &type) {m_type=type;}
    void setFile(const QString &file) {m_file=file;}
    void setTop(const QString &top) {
        if (top.isEmpty()) {
            return;
        }
        bool ok = false;
        int value = top.toInt(&ok);
        if (ok) {
            m_top=value;
        }
    }
    QString mimeType() const {return m_type;}
    QString file() const {return m_file;}
    int top() const {return m_top;}
protected:
    QString m_type;
    QString m_file;
    int     m_top;
};

class IBuild : public QObject
{
    Q_OBJECT
public:
    IBuild(QObject *parent = 0): QObject(parent) {}
    virtual ~IBuild() {}
    virtual QString mimeType() const = 0;
    virtual QString id() const = 0;
    virtual QList<BuildAction*> actionList() const = 0;
    virtual QList<BuildLookup*> lookupList() const = 0;
    virtual BuildAction *findAction(const QString &name) = 0;
    virtual QString actionCommand(BuildAction *act,QMap<QString,QString> &liteEnv, const QProcessEnvironment &env) = 0;
    virtual QString actionArgs(BuildAction *act,QMap<QString,QString> &liteEnv) = 0;
signals:
    void buildEnvChanged(QString);
};

class IBuildManager : public IManager
{
    Q_OBJECT
public:
    IBuildManager(QObject *parent = 0) : IManager(parent) {}
    virtual void addBuild(IBuild *build) = 0;
    virtual void removeBuild(IBuild *build) = 0;
    virtual IBuild *findBuild(const QString &mimeType) = 0;
    virtual QList<IBuild*> buildList() const = 0;
    virtual void setCurrentBuild(IBuild *build) = 0;
    virtual IBuild *currentBuild() const = 0;
signals:
    void buildChanged(LiteApi::IBuild*);
};

} //namespace LiteApi


#endif //__LITEBUILDAPI_H__

