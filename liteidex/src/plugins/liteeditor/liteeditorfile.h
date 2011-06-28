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
// Module: liteeditorfile.h
// Creator: visualfc <visualfc@gmail.com>
// date: 2011-3-26
// $Id: liteeditorfile.h,v 1.0 2011-6-28 visualfc Exp $

#ifndef LITEEDITORFILE_H
#define LITEEDITORFILE_H

#include "liteapi.h"

class QTextDocument;
class LiteEditorFile : public LiteApi::IFile
{
    Q_OBJECT
public:
    LiteEditorFile(LiteApi::IApplication *app, QObject *parent = 0);

    virtual bool open(const QString &fileName, const QString &mimeType);
    virtual bool reload(bool externalModify);
    virtual bool save(const QString &fileName);
    virtual QString fileName() const;
    virtual QString mimeType() const;
public:
    void setDocument(QTextDocument *document);
    QTextDocument  *document();
    void setTextCodec(const QString &name);
    QString textCodec() const;
    bool reloadByCodec(const QString &codecName);
    bool open(const QString &fileName, const QString &mimeType, bool bCheckCodec);
protected:
    enum LineTerminatorMode {
        LFLineTerminator,
        CRLFLineTerminator,
        NativeLineTerminator =
#if defined (Q_OS_WIN)
        CRLFLineTerminator
#else
        LFLineTerminator
#endif
    };
    LineTerminatorMode m_lineTerminatorMode;
protected:
    bool m_hasDecodingError;
    LiteApi::IApplication *m_liteApp;
    QString        m_fileName;
    QString        m_mimeType;
    QTextDocument *m_document;
    QTextCodec    *m_codec;
};

#endif //LITEEDITORFILE_H
