#ifndef _SPELLCHECK_ADAPTER_H
#define _SPELLCHECK_ADAPTER_H

#ifndef ATTR_UNUSED /* suppress compiler warning */
#define ATTR_UNUSED __attribute__ ((__unused__))
// VS2010 doesn't like the above. Just live with the warnings by using the next line instead:
//#define ATTR_UNUSED
#endif

#include <QStringList>
class QTextDocument;

class SpellCheck_Adapter{
public:

    class Range{
    public:
        int index;
        int length;

        Range(const Range& range)  :index(range.index), length(range.length){}
        Range(int index, int length) :index(index), length(length){}
        bool operator==(const Range &other) const {return (index == other.index && length == other.length);}
    };

    SpellCheck_Adapter():_thesaurusEnabled(false), _spellerEnabled(false){}
    virtual ~SpellCheck_Adapter(){}

    virtual void enableSpeller(bool enable=true){_spellerEnabled=enable;}   //false would release resources used by speller/thesaurus.
    virtual bool isSpellerEnabled(){return _spellerEnabled;}

    //return language codes for display/selection (empty list -> no "languages" subsection at the context menu)
    virtual QStringList getAvailableLanguages(){return QStringList();}

    //must be one of the available languages or empty
    virtual QString     getCurrentLanguage(){return "";}

    //constructed without initial language, so must invoke this to be usable.
    virtual void        setLanguage(const QString& langCode ATTR_UNUSED){}

    //mandatory, should be relatively fast (used synchronously to highlight typos as-you-type). document might be useful.
    virtual QList<Range> getErrorRanges(const QString& text, QTextDocument *document=0) =0;

    //mandatory, possibly slower (finding actual suggestions, typically as a context menu)
    virtual QStringList getSpellingSuggestions(const QString& word) =0;

    //implement and forward if the speller supports learning huristics
    virtual void        suggestionAccepted(const QString& suggestion ATTR_UNUSED, const QString& forWord ATTR_UNUSED){}

    //possible properties: IGNORE_WORD, ADD_WORD, THESAURUS, MULTI_LANG (=Can switch languages).
    // supported options are usully reflected at the context menu.
    virtual bool        isSupported(const QString& propertyName ATTR_UNUSED){return false;}

    //temporarily accept word as valid (till this object dies). document might be useful.
    virtual void        ignoreWord(const QString& word ATTR_UNUSED, QTextDocument *document ATTR_UNUSED=0){}

    //add word to user's dictionary
    virtual void        addWord(const QString& word ATTR_UNUSED){}


    //Thesaurus methods (valid when isSupported("THESAURUS") is true).
    virtual void enableThesaurus(bool enable=true){_thesaurusEnabled=enable;}
    virtual bool isThesaurusEnabled(){return isSpellerEnabled() && isSupported("THESAURUS") && _thesaurusEnabled;}
    //Each list starts with the meaning name, and then the alternatives.
    virtual QList<QStringList> getThesaurusSuggestions(const QString& word ATTR_UNUSED, QString* out_actualWord ATTR_UNUSED=0){return QList<QStringList>();}


    //utility (for external/derived), possibly overridden with better algo or more suitible algo for a specific implementation.
    virtual QList<Range> getWordsLocations(const QString& text){
        QString _text=text+" "; // removes the need for EOL logic. Probably not a good tradeoff if text is very long (since it's copied here).
        QList<Range> result;
        int from=0, len;
        while((from=_text.indexOf(QRegExp("[\\w`']"), from)) >=0){
            len=_text.indexOf(QRegExp("[^\\w`']"), from)-from;
            result.append(Range(from, len));
            from+=len;
        }
        return result;
    }

private:
    volatile bool _thesaurusEnabled;
    volatile bool _spellerEnabled;

};

///////////// Quassel specific ////////////////
//
// Within Quassel, we use a single, shared (abstract reference to) SpellCheck_Adapter.
// For the sake of simplicity, we're declaring it here and defining it at hunspell_adapter.cpp, effectively creatning a singleton.
// This structure makes it impossible to instanciate a speller object, and forces usage of 'theSpeller'
//  (SpellCheck_Adapter is abstract, and Hunspell_Adapted declaration was intentionally put at hunspell_adapter.cpp only)
//
// If needed in a non-quassel context, a proper general usage would be this speller_Adapter.h file,
// and concrete h/cpp files for specific implementations (e.g. hunspell_Adapter.h/.cpp), possibly with an abtract factory, etc.

extern SpellCheck_Adapter& theSpeller;

#endif
