/****************************************************************************
**
** Copyright (C) 2007 Trolltech ASA. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef SPELLER_QTEXTEDIT_H
#define SPELLER_QTEXTEDIT_H

#include <QTextEdit>
#include <QTextCursor>
class QAction;
class SpellCheckHighlighter;
class SpellCheck_Adapter;

typedef void(*SaveLangCallbackType)(const QString&);

class Speller_QTextEdit: public QTextEdit
{
Q_OBJECT
public:    
  Speller_QTextEdit(QWidget *parent =0, const QString& initialLanguage ="", SaveLangCallbackType callbackSaveLang =0);
    ~Speller_QTextEdit();
    void setCheckSpellingEnabled(bool enable = true);// Mimics KTextEdit
    void enableThesaurus(bool enable = true);
	
protected:
    void contextMenuEvent(QContextMenuEvent * e);

private slots:
    void spellerContextAction(QAction *action);

private:
    QTextCursor checkedTextCursor;
    QString currentWord;
    SpellCheckHighlighter *spellhighlighter;

    QAction *ignoreWordAction;
    QAction *addWordAction;
    QList<QAction *> replaceWordActions;
    QList<QAction *> setLangActions;
    void clearPermMenus();

    SpellCheck_Adapter* pSpeller;
    SaveLangCallbackType _callbackSaveLang;
};

#endif
