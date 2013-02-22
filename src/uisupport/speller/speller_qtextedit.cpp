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
#include "speller_adapter.h"
#include "speller_qtextedit.h"

#include <QList>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

#include <QSyntaxHighlighter>
#include <QTextCursor>

class SpellCheckHighlighter : public QSyntaxHighlighter
{
public:
    SpellCheckHighlighter(QTextDocument *textDocument, SpellCheck_Adapter* s)
        : QSyntaxHighlighter(textDocument)
        //,enabled(true)
        ,adapter(s) {}
    SpellCheckHighlighter(QTextEdit *textEdit, SpellCheck_Adapter* s)
        : QSyntaxHighlighter(textEdit)
        //,enabled(true)
        ,adapter(s) {}

    void highlightBlock(const QString &text);
    //bool enabled;

private:
    SpellCheck_Adapter* adapter;
};

void SpellCheckHighlighter::highlightBlock(const QString &text)
{
  if(!adapter || !adapter->isSpellerEnabled())
    return;

    QTextCharFormat spellingErrorFormat;
    spellingErrorFormat.setUnderlineColor(QColor(Qt::red));
    spellingErrorFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

    QList<SpellCheck_Adapter::Range> ranges = adapter->getErrorRanges(text);
    foreach(SpellCheck_Adapter::Range spellingErrorIndex, ranges){
        setFormat(spellingErrorIndex.index, spellingErrorIndex.length, spellingErrorFormat);
        //qDebug("setting highlight: %d, %d\n", spellingErrorIndex.index, spellingErrorIndex.length);
    }
}

Speller_QTextEdit::Speller_QTextEdit(QWidget *parent, const QString& initialLanguage, SaveLangCallbackType callbackSaveLang)
:QTextEdit(parent)
,ignoreWordAction(0)
,addWordAction(0)
,pSpeller(&theSpeller)
,_callbackSaveLang(callbackSaveLang)
{
  QString lang=initialLanguage;
  if(lang.length())
    pSpeller->setLanguage(lang);
  else if (pSpeller->getAvailableLanguages().length())
    pSpeller->setLanguage(pSpeller->getAvailableLanguages().at(0));

  spellhighlighter = new SpellCheckHighlighter(this, pSpeller);

  //for some reason the tooltips don't show...
  //ignoreSpellingAction->setToolTip(tr("Consider as valid for this session only"));
  //learnSpellingAction->setToolTip(tr("Consider as valid and add to your personal dictionary file"));
}

void Speller_QTextEdit::clearPermMenus(){
  if(ignoreWordAction)
    delete ignoreWordAction;
  ignoreWordAction=0;

  if(addWordAction)
    delete addWordAction;
  addWordAction=0;
}

Speller_QTextEdit::~Speller_QTextEdit(){
  delete spellhighlighter; pSpeller=NULL;
}


static const QString SUG_PREFIX=" >  ";
static const QString LANG_PREFIX="";
static const int MAX_SUBMENU_ITEMS=15;// specific number of suggestions the user can choose from (and max number of meanings in thes)
static const int MAX_WORDS_PREVIEW=5;//in thesaurus mode, max words shown in a single submenu line.

//returns the index of the longest range which contains cursorIx (with possibly overlapping ranges). or -1 for not found
int getBestRange(const int cursorIx, const QList<SpellCheck_Adapter::Range>& fromRanges){
  int foundLen=0, foundIx=-1;
  for( int i=0; i<fromRanges.length(); i++){
      SpellCheck_Adapter::Range r = fromRanges.at(i);
      if(  cursorIx >= r.index && cursorIx <= r.index+r.length && r.length>foundLen){
          foundLen=r.length;
          foundIx=i;
      }
  }
  return foundIx;
}

void Speller_QTextEdit::contextMenuEvent(QContextMenuEvent * e)
{
    setLangActions.clear();
    replaceWordActions.clear();

    QMenu *menu = createStandardContextMenu();  // The standard menu.

    if(!pSpeller || !pSpeller->isSpellerEnabled()){
        menu->exec(e->globalPos());
        delete menu;
        return;
    }


    //select the highlighted word under the cursor
    QTextCursor cursor = cursorForPosition(e->pos());
    QString text=this->toPlainText();

    //It's best to offer suggestion to a marked section of the text. So we use the same logic that the highlighter used
    //(the speller adapter object) and look for an error range which contains our cursor.
    //Also, the Hunspell adapter doesn't emitt overlapping error ranges, but other adapters might, and if so, take the longest.
    currentWord="";
    QList<SpellCheck_Adapter::Range> rs = pSpeller->getErrorRanges(text);
    int wIx=getBestRange(cursor.position(), rs);
    if(wIx>=0){
      //currentWord=text.mid(rs.at(wIx).index, rs.at(wIx).length);
      cursor.setPosition(rs.at(wIx).index, QTextCursor::MoveAnchor);
      cursor.setPosition(rs.at(wIx).index+rs.at(wIx).length, QTextCursor::KeepAnchor);
      currentWord = cursor.selectedText();
    }

    menu->addSeparator();
    qDebug("(speller) context for: '%s'\n", currentWord.toLocal8Bit().data());

    if(currentWord.length()){
      const int dispMax=10;
      QString dispWord=currentWord;//not found word, truncated to dispMax chars.
      if(dispWord.length()>dispMax)
        dispWord=dispWord.mid(0,dispMax-2)+"...";

      delete (menu);  //Typo -> only Spell checker menu
      menu = new QMenu(this->parentWidget());//createStandardContextMenu();


      const QStringList suggestions = pSpeller->getSpellingSuggestions(currentWord);
      qDebug("Got %d spelling suggestions", suggestions.length());
      if (suggestions.isEmpty()) {
        menu->addAction(tr("< No Corrections: '")+dispWord+"' >")->setEnabled(false);
      } else {
        int i=0;
          foreach (const QString &suggestion, suggestions){
            if(i++<MAX_SUBMENU_ITEMS)
              replaceWordActions << menu->addAction(SUG_PREFIX + suggestion);
          }
      }

      menu->addSeparator();

      clearPermMenus();
      menu->addAction(ignoreWordAction=new QAction(tr("Ignore temporarily: '")+dispWord+"'", this));
      menu->addAction(addWordAction=new QAction(tr("Add to Dictionary : '")+dispWord+"'", this));


    } else if(pSpeller->isSupported("THESAURUS") && pSpeller->isThesaurusEnabled()){
      // no spelling error, try to invoke thesaurus, if exists (at its own sub menu from the normal menu)

      QList<SpellCheck_Adapter::Range> wordRanges=pSpeller->getWordsLocations(text);
      QString word="";

      int wIx=getBestRange(cursor.position(), wordRanges);
      if(wIx>=0)
        word=text.mid(wordRanges.at(wIx).index, wordRanges.at(wIx).length);

      if(word.length()){
        QString actual, changedMarker;
        QList<QStringList> alts = pSpeller->getThesaurusSuggestions(word, &actual);
        if(actual != word) changedMarker="->";

        qDebug("Word: '%s': %d alts", actual.toLocal8Bit().data(), alts.length());

        if(!alts.length()){
          //no need for the shorter dispWord since we're here after no spelling errors found,
          //so it's not an endless string
          menu->addAction(tr("Thesaurus")+" "+changedMarker+"'"+actual+"': < None >")->setEnabled(false);

        } else {
          qDebug("Got %d thes meanings", alts.length());
          cursor.setPosition(wordRanges.at(wIx).index, QTextCursor::MoveAnchor);
          cursor.setPosition(wordRanges.at(wIx).index+wordRanges.at(wIx).length, QTextCursor::KeepAnchor);

          QMenu *thes = new QMenu(QString()+" > "+tr("Thesaurus")+" "+changedMarker+"'"+actual+"': "+alts.at(0).at(1)+", ...", menu);
          menu->addMenu(thes);

          for(int i=0; i<std::min(MAX_SUBMENU_ITEMS, alts.length()); i++){
            QString item=alts.at(i).at(0);
            for(int j=2; j<std::min(MAX_WORDS_PREVIEW+1, alts.at(i).length()); j++)
              item+=QString(", ")+alts.at(i).at(j);
            if(alts.at(i).length()>MAX_WORDS_PREVIEW+1 && MAX_WORDS_PREVIEW<MAX_SUBMENU_ITEMS)
              item+=", ...";

            QMenu *meaning = new QMenu(item, menu);
            for(int j=1; j<std::min(MAX_SUBMENU_ITEMS+1, alts.at(i).length()); j++)
              replaceWordActions << meaning->addAction(SUG_PREFIX+alts.at(i).at(j));
            thes->addMenu(meaning);//->setEnabled(false);
          }
        }
      }
    }


    if(pSpeller->isSupported("MULTI_LANG")){
      QStringList al=pSpeller->getAvailableLanguages();
      if(al.length()){
          QMenu *langs = new QMenu(tr("Dictionaries")+" ["+pSpeller->getCurrentLanguage()+"]", menu);
          menu->addMenu(langs);
          for(int i=0; i<al.length(); i++){
              QAction* a=langs->addAction(LANG_PREFIX+ al.at(i));
              a->setCheckable(true);
              a->setChecked(pSpeller->getCurrentLanguage()==al.at(i));
              setLangActions << a;
          }
      }
    }

    
    checkedTextCursor = cursor;
    connect(menu, SIGNAL(triggered(QAction *)), SLOT(spellerContextAction(QAction *)));

    menu->exec(e->globalPos());
    delete menu;
 }

void Speller_QTextEdit::setCheckSpellingEnabled(bool enable){
  if(pSpeller)
    pSpeller->enableSpeller(enable);

  spellhighlighter->rehighlight();
}

void Speller_QTextEdit::enableThesaurus(bool enable){
  if(pSpeller)
    pSpeller->enableThesaurus(enable);
}

//#include "quassel.h"
void Speller_QTextEdit::spellerContextAction(QAction *action){

  qDebug("Action*: %x, replaceActions.length(): %d", (unsigned int)action, replaceWordActions.length());
    if (action == ignoreWordAction) {
        qDebug("Context -> Ignore");
        pSpeller->ignoreWord(currentWord);

    } else if (action == addWordAction) {
        qDebug("Context -> Learn");
        pSpeller->addWord(currentWord);

    } else if (replaceWordActions.contains(action)){
        qDebug("Context -> replace word");
        QString txt=action->text().remove(0,SUG_PREFIX.length());
        checkedTextCursor.insertText(txt);

    } else if(setLangActions.contains(action)){
        qDebug("Context -> setLang");
        QString txt=action->text().remove(0,LANG_PREFIX.length());
        pSpeller->setLanguage(txt);
        //qDebug("config: %s", Quassel::configDirPath().toLocal8Bit().data());
        if(_callbackSaveLang)
          _callbackSaveLang(txt);
    }

    spellhighlighter->rehighlight();
}

