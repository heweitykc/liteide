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
// Module: build.cpp
// Creator: visualfc <visualfc@gmail.com>
// date: 2011-3-26
// $Id: build.cpp,v 1.0 2011-5-12 visualfc Exp $

#include "build.h"
#include "fileutil/fileutil.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QFileInfo>
#include <QDir>
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

Build::Build(QObject *parent)
    : LiteApi::IBuild(parent)
{
}

Build::~Build()
{
    qDeleteAll(m_lookupList);
    qDeleteAll(m_actionList);
    qDeleteAll(m_configList);
    qDeleteAll(m_customList);
}

QString Build::mimeType() const
{
    return m_mimeType;
}

QString Build::id() const
{
    return m_id;
}

QList<BuildAction*> Build::actionList() const
{
    return m_actionList;
}

QList<BuildLookup*> Build::lookupList() const
{
    return m_lookupList;
}

QList<BuildConfig*> Build::configList() const
{
    return m_configList;
}

QList<BuildCustom*> Build::customList() const
{
    return m_customList;
}

BuildAction *Build::findAction(const QString &id)
{
    foreach(BuildAction *act, m_actionList) {
        if (act->id() == id) {
            return act;
        }
    }
    return 0;
}

void Build::setType(const QString &mimeType)
{
    m_mimeType = mimeType;
}

void Build::setId(const QString &id)
{
    m_id = id;
}

void Build::appendAction(BuildAction *act)
{
    m_actionList.append(act);
}

void Build::appendLookup(BuildLookup *lookup)
{
    m_lookupList.append(lookup);
}

void Build::appendConfig(BuildConfig *config)
{
    m_configList.append(config);
}

void Build::appendCustom(BuildCustom *custom)
{
    m_customList.append(custom);
}

bool Build::loadBuild(LiteApi::IBuildManager *manager, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        return false;
    }
    return Build::loadBuild(manager,&file,fileName);
}

bool Build::loadBuild(LiteApi::IBuildManager *manager, QIODevice *dev, const QString &fileName)
{
    QXmlStreamReader reader(dev);
    QXmlStreamAttributes attrs;
    QDir dir = QFileInfo(fileName).absoluteDir();
    Build *build = 0;
    BuildAction *act = 0;
    BuildLookup *lookup = 0;
    BuildConfig *config = 0;
    BuildCustom *custom = 0;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            attrs = reader.attributes();
            if (reader.name() == "mime-type" && build == 0) {
                build = new Build;
                build->setType(attrs.value("type").toString());
                build->setId(attrs.value("id").toString());
            } else if (reader.name() == "lookup" && lookup == 0) {
                lookup = new BuildLookup;
                lookup->setMimeType(attrs.value("mime-type").toString());
                lookup->setFile(attrs.value("file").toString());
                lookup->setTop(attrs.value("top").toString());
            } else if (reader.name() == "action" && act == 0) {
                act = new BuildAction;
                act->setId(attrs.value("id").toString());
                act->setKey(attrs.value("key").toString());
                act->setCmd(attrs.value("cmd").toString());
                act->setArgs(attrs.value("args").toString());
                act->setSave(attrs.value("save").toString());
                act->setOutput(attrs.value("output").toString());
                act->setReadline(attrs.value("readline").toString());
                act->setCodec(attrs.value("codec").toString());
                act->setRegex(attrs.value("regex").toString());
                QString img = attrs.value("img").toString();
                if (!img.isEmpty()) {
                    if (img.at(0) != ':') {
                        img = QFileInfo(dir,img).canonicalFilePath();
                    }
                    act->setImg(img);
                }
                QString task = attrs.value("task").toString();
                if (!task.isEmpty()) {
                    act->setTask(task.split(";",QString::SkipEmptyParts));
                }
            } else if (reader.name() == "config" && config == 0) {
                config = new BuildConfig;
                config->setId(attrs.value("id").toString());
                config->setName(attrs.value("name").toString());
                config->setValue(attrs.value("value").toString());
            } else if (reader.name() == "custom" && custom == 0) {
                custom = new BuildCustom;
                custom->setId(attrs.value("id").toString());
                custom->setName(attrs.value("name").toString());
                custom->setValue(attrs.value("value").toString());
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader.name() == "mime-type") {
                if (build) {
                     manager->addBuild(build);
                }
                build = 0;
            } else if (reader.name() == "action") {
                if (build && act) {
                    build->appendAction(act);
                }
                act = 0;
            } else if (reader.name() == "lookup") {
                if (build && lookup) {
                    build->appendLookup(lookup);
                }
                lookup = 0;
            } else if (reader.name() == "config") {
                if (build && config) {
                    build->appendConfig(config);
                }
                config = 0;
            } else if (reader.name() == "custom") {
                if (build && custom) {
                    build->appendCustom(custom);
                }
                custom = 0;
            }
            break;
        default:
            break;
        }
    }
    return true;
}

QString Build::actionCommand(BuildAction *act,QMap<QString,QString> &liteEnv, const QProcessEnvironment &env)
{
    QString cmd = act->cmd();
    QMapIterator<QString,QString> i(liteEnv);
    while(i.hasNext()) {
        i.next();
        cmd.replace(i.key(),i.value());
    }
    QRegExp rx("\\$\\{(\\w+)\\}");
    int pos = 0;
    QStringList list;
    while ((pos = rx.indexIn(cmd, pos)) != -1) {
         list << rx.cap(1);
         pos += rx.matchedLength();
    }
    foreach (QString str, list) {
        if (env.contains(str)) {
            cmd.replace("${"+str+"}",env.value(str));
        }
    }

    return FileUtil::lookPath(cmd,env,true);
}

QString Build::actionArgs(BuildAction *act,QMap<QString,QString> &liteEnv)
{
    QString args =  act->args();
    QMapIterator<QString,QString> i(liteEnv);
    while(i.hasNext()) {
        i.next();
        args.replace(i.key(),i.value());
    }
    return args;
}

