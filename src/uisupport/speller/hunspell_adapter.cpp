#include "speller_adapter.h"
#include "../../3rdparty/hunspell/hunspell.hxx"
#include "../../3rdparty/mythes/mythes.hxx"

//Shared<Hunspell_Adapter> SharedSpeller;

#include <QHash>
#include <QTextCharFormat>
#include <QTextCodec>
#include <QFileInfo>
#include <QDir>
#include <quassel.h> //For config and data dirs

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
    void                enumerateDictionaryFiles(bool forceRe=false);
    bool m_dictsEnumerated;
    QStringList m_langs;

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
    //void        addWord( const QString &word, QTextDocument *document=0 );

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
  _LangData() : name(""), dictFilesSansExt(""), thesFilesSansExt(""){}
} LangData;

// Our canonical form is ab-XY (e.g. en-US). Try to detect it from filename.
QString getNormalizedLang(const QString& canonicalFilename, bool isSpeller){
    QString f = canonicalFilename;
    if(f.lastIndexOf("/")>=0) f=f.mid(f.lastIndexOf("/")+1);
    if(f.lastIndexOf(".") >0) f=f.mid(0, f.lastIndexOf("."));
    if(!isSpeller && f.mid(0,3)=="th_") f=f.mid(3);
    if(f[2]=='_') f[2]='-';
    if((f.length()==5 || (f[5]=='_' && f[6]=='v' && f[7].isDigit())) && f[2]=='-')
        f=f.mid(0,2).toLower()+"-"+f.mid(3,2).toUpper();

    return f;
}

QString getCanonicalSansExt(const QString& canonicalFilename){
    QString f(canonicalFilename);
    if(f.lastIndexOf(".")>(f.lastIndexOf("/")+1))
        f=f.mid(0,f.lastIndexOf("."));
    return f;
}

int findLangItem(QList<_LangData>& current, const QString& lang){
    for (int i = 0; i<current.length(); i++)
        if(current[i].name==lang) return i;
    return -1;
}

// Search order is from global (system) to local (user), so later data overrides earlier data.
void newFilesFound(QList<_LangData>& current, const QString& canonicalFilename, bool isSpeller){ //!isSpeller is thesaurus
    QString lang = getNormalizedLang(canonicalFilename, isSpeller);
    int i = findLangItem(current, lang);
    if(i<0) {
        current.append(_LangData());
        i=current.length()-1;
        current[i].name = lang;
    }
    if(isSpeller) current[i].dictFilesSansExt = getCanonicalSansExt(canonicalFilename);
    else          current[i].thesFilesSansExt = getCanonicalSansExt(canonicalFilename);
}

QString norPath(QString path){
    path.replace('\\', '/');
    return path.endsWith("/")?path:path+"/";
}

QStringList getSearchPaths() {
    // system data dirs need to be searched at /myspell/dicts, /hunspell
    // quassel and user data dirs need /dicts

    // we create the list in order of descending importance,
    // then filter for uniqueness at the same order (so more important override less imp.),
    // then reverse such that the most important is last
    QStringList tmpAll;
    QStringList tmp;

    // user dir, quassel data dirs
    tmp.clear();
    tmp << Quassel::configDirPath() // user's, add /dict
        << Quassel::dataDirPaths(); // quassel specific, add /dict
    for(int i=0; i<tmp.length(); i++){
        tmpAll.append(norPath(tmp[i])+"dicts/");
    }

    // system data dirs
    tmp.clear();
    tmp.append(QString(::qgetenv("XDG_DATA_DIRS")).split(":", QString::SkipEmptyParts));
    tmp << "/usr/local/share/"
        << "/usr/share/";
    for(int i=0; i<tmp.length(); i++){
        tmpAll.append(norPath(tmp[i])+"hunspell/");
        tmpAll.append(norPath(tmp[i])+"myspell/dicts/");
    }

    for(int i=0; i<tmpAll.length(); i++)
        qDebug("tmp non unique [%d]: %s", i, tmpAll[i].toLatin1().data());

    // make unique, earlier items preffered over later ones
    tmp.clear();
    for (int i=0; i<tmpAll.length(); i++)
        if (!tmp.contains(tmpAll[i])) tmp.append(tmpAll[i]);

    tmpAll.clear();
    for(int i=tmp.length()-1; i>=0; i--)
        if(QDir(tmp[i]).exists())
            tmpAll.append(tmp[i]);

    for(int i=0; i<tmpAll.length(); i++)
        qDebug("unique, ascending [%d]: %s", i, tmpAll[i].toLatin1().data());

    return tmpAll;
}

void Hunspell_Adapter::enumerateDictionaryFiles(bool forceRe){
    if (!forceRe && m_dictsEnumerated)
        return;
    QList<_LangData> tmp;

    QStringList paths = getSearchPaths();
    for (int i=0; i<paths.length(); i++) {
        QDir dir(paths[i]);
        dir.setFilter(QDir::Files);
        dir.setSorting(QDir::Name);//sort by name, such that xx_v2 comes after v1 | QDir::Reversed);
        QStringList fspec;
        QFileInfoList list;

        for (int k=0; k<2; k++){ // first iteration for speller files(dic+aff), 2nd for thesaurus (dat+idx)
            fspec.clear();
            fspec << (k?"*.dat":"*.dic");
            dir.setNameFilters(fspec);
            list = dir.entryInfoList();
            for(int j=0; j<list.length(); j++) {
                QString canon = dir.canonicalPath() +"/"+ list[j].fileName();
                if(dir.exists(getCanonicalSansExt(canon)+(k?".idx":".aff")))
                    newFilesFound(tmp, canon, !k);
            }
        }
    }

    m_langs.clear();
    langData.clear();
    // keep only langs which have at least a speller.
    // hunspell can work without mythes, but not the other way around)
    for(int i=0; i<tmp.length(); i++){
        if (tmp[i].dictFilesSansExt!="") {
            langData.append(tmp[i]);
            m_langs.append(tmp[i].name);
            qDebug("%s: [d]%s [t]%s", langData[i].name.toLatin1().data(), langData[i].dictFilesSansExt.toLatin1().data(), langData[i].thesFilesSansExt.toLatin1().data());
        } else {
            qDebug("Lang: %s only has a thesaurus, ignoring.", tmp[i].name.toLatin1().data());
        }
    }

    m_dictsEnumerated = true;
}

//dictionaries/thesauri files and availability are detected once when the app loads.
//QList<LangData> Hunspell_Adapter::langData = createLangData();

Hunspell* Hunspell_Adapter::createNewSpeller(const QString& lang){
    int i = findLangItem(langData, lang);
    if(i<0){
        qDebug("Internal error: requested language '%s' but doesn't exist.", lang.toLatin1().data());
        return 0;
    }
    QString aff=langData[i].dictFilesSansExt+".aff";
    QString dic=langData[i].dictFilesSansExt+".dic";
    QFileInfo fa(aff);
    QFileInfo fd(dic);

    if (!(fa.exists() && fa.isReadable() && fd.exists() && fd.isReadable())){
        qDebug("issue with dict file for lang %s (trying %s, %s)", lang.toLocal8Bit().data(), dic.toLatin1().data(), aff.toLatin1().data());
        return 0;
    }

    qDebug("dict file for lang: OK");
    return new Hunspell(aff.toLatin1(), dic.toLatin1());
}

MyThes* Hunspell_Adapter::createNewThesaurus(const QString& lang){
    int i = findLangItem(langData, lang);
    if(i<0){
        qDebug("Internal error: requested language '%s' but doesn't exist.", lang.toLatin1().data());
        return 0;
    }
    QString dat=langData[i].thesFilesSansExt+".dat";
    QString idx=langData[i].thesFilesSansExt+".idx";
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
    : currentLang("NOT AVAILABLE")
    , pSpeller(0)
    , pThes(0)
    , m_dictsEnumerated(false)
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


QStringList Hunspell_Adapter::getAvailableLanguages(){
    enumerateDictionaryFiles(); // returns immidiately if already enumerated
    return m_langs;
  //return QStringList()<<"en_US"<<"en_GB"<<"he_IL";
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
      QStringList meaning; meaning << QString(pm->defn).trimmed();
      for (int j=0; j < pm->count; j++) {
        char ** gen;
        int l = 0;
        if (pH && oldbuf[0])
          l = pH->generate(&gen, pm->psyns[j], oldbuf);

        if (l) {
          int k;
          for (k = 0; k < l; k++)
            if (!meaning.contains(gen[k]))
              meaning << QString(gen[k]).trimmed();

          myfreelist(&gen, l);
        } else
          if (!meaning.contains(pm->psyns[j]))
            meaning << QString(pm->psyns[j]).trimmed();
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

    if(pThes){
      qDebug("Thes ok, encoding: %s\n", pThes->get_th_encoding());
      this->_thesaurusAvailable = true;
    } else {
      qDebug("Thes not available for lang\n");
      this->_thesaurusAvailable = false;
    }

    QString encoding=pSpeller->get_dic_encoding();
    m_codec = QTextCodec::codecForName(encoding.toLatin1());

    qDebug("speller: codec: %s\n", m_codec->name().data());
}

bool Hunspell_Adapter::isWordSpelledOk(const QString& word){
    return !pSpeller || pSpeller->spell(m_codec->fromUnicode(word));
}

QList<SpellCheck_Adapter::Range> Hunspell_Adapter::getErrorRanges(const QString& text, QTextDocument *document){
  Q_UNUSED(document);
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
// user dictionary is unsupported for now
void Hunspell_Adapter::addWord( const QString &word, QTextDocument *document ATTR_UNUSED){
}
*/

void Hunspell_Adapter::ignoreWord(const QString& word, QTextDocument *document){
  Q_UNUSED(document);
  if(pSpeller)
    pSpeller->add(m_codec->fromUnicode(word));
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// our singletone

Hunspell_Adapter _theSpeller;
SpellCheck_Adapter& theSpeller=_theSpeller;
