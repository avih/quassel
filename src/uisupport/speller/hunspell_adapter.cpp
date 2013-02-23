#include "speller_adapter.h"
#include "../ext/hunspell/hunspell.hxx"
#include "../ext/mythes/mythes.hxx"

//Shared<Hunspell_Adapter> SharedSpeller;

#include <QHash>
#include <QTextCharFormat>
#include <QTextCodec>
#include <QFileInfo>

//search paths taken from hunspell/src/tools/hunspell.cxx.
//OOODIR and USEROOODIR (open office) removed because seem quite deprecated.
//DICBASENAME is for the user's private dictionary (learned/ignored/etc).
//TODO: dict/thes files and user's file should probably respect -c for config folder...
#ifdef WIN32
  #define LIBDIR "C:\\Hunspell\\"
  #define HOME "%USERPROFILE%\\"
  #define DICBASENAME "hunspell_"
  #define DIRSEPCH '\\'
  #define DIRSEP "\\"
  #define PATHSEP ";"
#else
  // Not Windows -> *nix (what about OSX? ...)
  #define LIBDIR \
      "/usr/share/hunspell:" \
      "/usr/share/myspell:" \
      "/usr/share/myspell/dicts:" \
      "/Library/Spelling"
  #define HOME getenv("HOME")
  #define DICBASENAME ".hunspell_"
  #define DIRSEPCH '/'
  #define DIRSEP "/"
  #define PATHSEP ":"
#endif


//class Hunspell; // The official Hunspell export
//class MyThes;   // The official MyThes[aurus] export
struct _LangData;
class Hunspell_Adapter: public SpellCheck_Adapter{
private:
    typedef SpellCheck_Adapter parent;
    QString currentLang;
    Hunspell* pSpeller;
    MyThes* pThes;
    bool isWordSpelledOk(const QString& word);
    QTextCodec* m_codec;
    Hunspell*   createNewSpeller(const QString& lang);
    MyThes*     createNewThesaurus(const QString& lang);

    QList<_LangData>    langData;  //available dictionaries/thesauri names/files
    void                enumerateDictionaryFiles();

public:
    Hunspell_Adapter();
    ~Hunspell_Adapter();

    void        enableSpeller(bool enable=false);
    QStringList	getAvailableLanguages();
    QString     getCurrentLanguage();
    void        setLanguage( const QString& lang );
    QList<Range> getErrorRanges( const QString& text, QTextDocument *document=0 );
    QStringList getSpellingSuggestions( const QString& word );
    QList<QStringList> getThesaurusSuggestions( const QString& word, QString* out_actualWord=0);
    void        ignoreWord( const QString &word, QTextDocument *document=0 );
    void        addWord( const QString &word, QTextDocument *document=0 );

    bool        isSupported( const QString& propertyName ){
        return (QStringList()
            <<"IGNORE_WORD"
            //<<"ADD_WORD" //no user dictionary for now
            <<"THESAURUS"
            <<"MULTI_LANG"
        ).contains(propertyName);
    }
};



//const char* cs(QString s){return s.toLatin1().data();}

typedef struct _LangData{
  QString name;
  QString dictFilesSansExt; //such that the full paths are dictFilesSansExt .dic/.aff
  QString thesFilesSansExt; //full paths are thesFilesSansExt .idx/.dat , Non existing thes -> empty
} LangData;


void Hunspell_Adapter::enumerateDictionaryFiles(){
  //QList<LangData> result;
  langData.empty();
  //TODO: blah blah blah
}

//dictionaries/thesauri files and availability are detected once when the app loads.
//QList<LangData> Hunspell_Adapter::langData = createLangData();

Hunspell* Hunspell_Adapter::createNewSpeller(const QString& lang){
    QString aff=lang+".aff";
    QString dic=lang+".dic";
    QFileInfo fa(aff);
    QFileInfo fd(dic);

    if (!(fa.exists() && fa.isReadable() && fd.exists() && fd.isReadable())){
        qDebug("issue with dict file for lang %s", lang.toLocal8Bit().data());
        return 0;
    }

    qDebug("dict file for lang: OK");
    return new Hunspell(aff.toLatin1(), dic.toLatin1());
}

MyThes* Hunspell_Adapter::createNewThesaurus(const QString& lang){
    QString dat=lang+".dat";
    QString idx=lang+".idx";
    QFileInfo fd(dat);
    QFileInfo fi(idx);

    if (!(fd.exists() && fd.isReadable() && fi.exists() && fi.isReadable())){
        qDebug("issue with thes file for lang %s", lang.toLocal8Bit().data());
        return 0;
    }

    qDebug("thes file for lang: OK");
    return new MyThes(idx.toLatin1(), dat.toLatin1());
}

Hunspell_Adapter::Hunspell_Adapter()
    :currentLang("NOT AVAILABLE")
    ,pSpeller(0)
    ,pThes(0)
{
/*
    if (getAvailableLanguages().length())
        setLanguage(getAvailableLanguages().at(0));
*/
}
	
Hunspell_Adapter::~Hunspell_Adapter(){
    if(pSpeller)
        delete pSpeller;
    pSpeller=0;

    if(pThes)
      delete pThes;
    pThes=0;
}
/*
QStringList Hunspell_Adapter::searchDictionaries(){
  QList<DictionaryPtr> m_dics;//should be member?

  //--sudocode:

  for(path in searchPaths)
  {
    for each(dic in currentDics=getAllFileNames("*.dic")){
      if(dic in m_dics || !exists(dic+".aff")) //skip existing names or names with partial files
        continue;

      m_dics+={path, dic};
    }
  }

  return QStringList();
}
*/

QStringList Hunspell_Adapter::getAvailableLanguages(){
/*
  if(!m_searchedDictionaries){
    m_searchedDictionaries = true;
    m_availableDictionaries = getDictionaries();
  }
  return m_availableDictionaries;
*/
  return QStringList()<<"en_US"<<"en_GB"<<"he_IL";
}

QString Hunspell_Adapter::getCurrentLanguage(){
    return currentLang;
}


//myfreelist and getThesSuggestions are adaptations from example.cxx of MyThes, and bear MyThes' license (BSD)
void myfreelist(char *** list, int n)
{
   if (list && (n > 0)) {
      for (int i = 0; i < n; i++) if ((*list)[i]) free((*list)[i]);
      free(*list);
      *list = NULL;
   }
}
//each string-list has the 1st string as the name/type of specific meaning of the word, and the rest of the strings as the synonyms with this meaning
QList<QStringList> getThesSuggestions(MyThes* pMT, const QString& word, Hunspell* pH=0, QString* out_actualWord=0){
  if(out_actualWord) *out_actualWord=word;
  QList<QStringList> result;
  if(!pMT) return result;

  char buf[101]={0};
  char oldbuf[101]={0};
  mentry * pmean;

  strncpy(buf, (const char*)word.toLatin1().data(), 100);

  oldbuf[0] = '\0';
  int len = strlen(buf);
  int count = pMT->Lookup(buf,len,&pmean);
  // don't change value of pmean
  // or count since needed for CleanUpAfterLookup routine
  if (!count && pH) {
    char **stem;
    int stemcount = pH->stem(&stem, buf);

    //avih: (original used stem[0] only): Use the first stem with non-zero count meanings
    //  (if exists, and assumming stems are ordered from best to worst match)
    for(int i=0; i<stemcount; i++){

      if(!oldbuf[0])//keep the original at most once
        strcpy(oldbuf,buf);
      strcpy(buf, stem[i]);
      len = strlen(buf);
      count = pMT->Lookup(buf, len, &pmean);

      if(count){
        if(out_actualWord){
          *out_actualWord=QString::fromLatin1(buf, len);
          if(word.trimmed().toLower()==(*out_actualWord).trimmed().toLower())
            *out_actualWord=word;
        }
        break;
      }
    }
    myfreelist(&stem, stemcount);

  } else
    oldbuf[0] = '\0';

  mentry* pm = pmean;
  if (count) {
    for (int  i=0; i < count; i++) {
      QStringList meaning; meaning << pm->defn;
      for (int j=0; j < pm->count; j++) {
        char ** gen;
        int l = 0;
        if (pH && oldbuf[0])
          l = pH->generate(&gen, pm->psyns[j], oldbuf);

        if (l) {
          int k;
          for (k = 0; k < l; k++)
            if (!meaning.contains(gen[k]))
              meaning << gen[k];

          myfreelist(&gen, l);
        } else
          if (!meaning.contains(pm->psyns[j]))
            meaning << pm->psyns[j];
      }

      pm++;

      QString tmp=meaning.at(0);
      for(int i=2; i<meaning.length(); i++)//1st meaning is the same as the meaning 0, just without the description (e.g. (noun) Go, Go, travel, ... )
        tmp += QString(", ")+meaning.at(i);

      qDebug("  %s", tmp.toLocal8Bit().data());

      if(!result.contains(meaning))
        result << meaning;
    }
    // now clean up all allocated memory
    pMT->CleanUpAfterLookup(&pmean,count);

  }/* else
    qDebug("\"%s\" is not in thesaurus!\n",buf);
    */

  return result;
}

void Hunspell_Adapter::enableSpeller(bool enable){
    parent::enableSpeller(enable);
    if(enable){
        qDebug("supposedly allocating speller/thes...");
        setLanguage(currentLang);

    } else {
        qDebug("releasing speller/thes allocations...");
        if(pSpeller)
            delete (pSpeller);
        pSpeller=0;

        if(pThes)
          delete pThes;
        pThes=0;
    }


}


void Hunspell_Adapter::setLanguage(const QString& lang){
    qDebug("Setting lang: %s", lang.toLocal8Bit().data());

    if(lang==getCurrentLanguage() && pSpeller){
      qDebug("no need for change...");
      return;
    }

    if( !getAvailableLanguages().contains(lang) ){
      qDebug("Invaid lang, aborting..");
      return;
    }

    currentLang=lang;

    if(!isSpellerEnabled())
      return;

    if(pSpeller)
      delete (pSpeller);
    pSpeller=0;

    if(pThes)
      delete pThes;
    pThes=0;

    if( !(pSpeller=createNewSpeller(currentLang)) )
      return;

    pThes=createNewThesaurus(currentLang);

    if(pThes)
      qDebug("Thes ok, encoding: %s\n", pThes->get_th_encoding());

    QString encoding=pSpeller->get_dic_encoding();
    m_codec = QTextCodec::codecForName(encoding.toLatin1());

    qDebug("speller: codec: %s\n", m_codec->name().data());
}

bool Hunspell_Adapter::isWordSpelledOk(const QString& word){
    return !pSpeller || pSpeller->spell(m_codec->fromUnicode(word));
}

QList<SpellCheck_Adapter::Range> Hunspell_Adapter::getErrorRanges(const QString& text, QTextDocument *document ATTR_UNUSED){
    QList<Range> result, words = getWordsLocations(text);

    if (!pSpeller || text.simplified().isEmpty())
        return result;

    for(int i=0; i<words.length(); i++)
      if(!isWordSpelledOk(text.mid(words.at(i).index, words.at(i).length )))
          result.append(words.at(i));

    return result;
}

QList<QStringList> Hunspell_Adapter::getThesaurusSuggestions(const QString& word, QString* out_actualWord){
  return getThesSuggestions(pThes, word, pSpeller, out_actualWord);
}

QStringList Hunspell_Adapter::getSpellingSuggestions(const QString& word){
    QStringList result;

    if(!pSpeller || isWordSpelledOk((word)))
        return result;
    
    char ** wlst;
    int ns = pSpeller->suggest(&wlst, m_codec->fromUnicode(word));
    if(ns<=0)
        return result;
    
    for (int i=0; i < ns; i++)
        result.append(m_codec->toUnicode(wlst[i]));
    
    pSpeller->free_list(&wlst, ns);
    
    return result;
}

/*
void Hunspell_Adapter::addWord( const QString &word, QTextDocument *document ATTR_UNUSED){
}
*/

void Hunspell_Adapter::ignoreWord(const QString& word, QTextDocument *document ATTR_UNUSED){
  if(pSpeller)
    pSpeller->add(m_codec->fromUnicode(word));
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// our singletone

Hunspell_Adapter _theSpeller;
SpellCheck_Adapter& theSpeller=_theSpeller;
