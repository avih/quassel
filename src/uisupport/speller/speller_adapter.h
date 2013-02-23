#ifndef _SPELLCHECK_ADAPTER_H
#define _SPELLCHECK_ADAPTER_H

#include <QStringList>
class QTextDocument;

class SpellCheck_Adapter{
public:

    class Range{
    public:
        int index;
        int length;

        Range(const Range& other) : index(other.index), length(other.length){}
        Range(int index, int length) : index(index), length(length){}
        bool operator==(const Range &other) const {return (index == other.index && length == other.length);}
    };

    SpellCheck_Adapter() : _spellerEnabled(false), _thesaurusEnabled(false), _thesaurusAvailable(false){}
    virtual ~SpellCheck_Adapter(){}

    virtual void enableSpeller(bool enable=true){_spellerEnabled=enable;}   //false would release resources used by speller/thesaurus.
    virtual bool isSpellerEnabled(){return _spellerEnabled;}

    //return language codes for display/selection (empty list -> no "languages" subsection at the context menu)
    virtual QStringList getAvailableLanguages(){return QStringList();}

    //must be one of the available languages or empty
    virtual QString     getCurrentLanguage(){return "";}

    //constructed without initial language, so must invoke this to be usable.
    virtual void        setLanguage(const QString& langCode){Q_UNUSED(langCode);}

    //mandatory, should be relatively fast (used synchronously to highlight typos as-you-type). document might be useful.
    virtual QList<Range> getErrorRanges(const QString& text, QTextDocument *document=0) =0;

    //mandatory, possibly slower (finding actual suggestions, typically as a context menu)
    virtual QStringList getSpellingSuggestions(const QString& word) =0;

    //implement and forward if the speller supports learning huristics
    virtual void        suggestionAccepted(const QString& suggestion, const QString& forWord)
      {Q_UNUSED(suggestion); Q_UNUSED (forWord);}

    //possible properties: IGNORE_WORD, ADD_WORD, THESAURUS, MULTI_LANG (=Can switch languages).
    // supported options are usully reflected at the context menu.
    virtual bool        isSupported(const QString& propertyName){Q_UNUSED(propertyName); return false;}

    //temporarily accept word as valid (till this object dies). document might be useful.
    virtual void        ignoreWord(const QString& word, QTextDocument *document=0)
      {Q_UNUSED(word); Q_UNUSED(document);}

    //add word to user's dictionary
    virtual void        addWord(const QString& word){Q_UNUSED(word);}


    //Thesaurus methods (valid when isSupported("THESAURUS") is true).
    virtual void enableThesaurus(bool enable=true){_thesaurusEnabled=enable;}
    virtual bool isThesaurusEnabled(){return isSpellerEnabled()
                                             && isSupported("THESAURUS")
                                             && _thesaurusEnabled;}
    virtual bool isThesaurusAvailable(){return isThesaurusEnabled() && _thesaurusAvailable;}
    //Each list starts with the meaning name, and then the alternatives.
    virtual QList<QStringList> getThesaurusSuggestions(const QString& word, QString* out_actualWord=0)
      {Q_UNUSED(word); Q_UNUSED(out_actualWord); return QList<QStringList>();}


    //utilities (for external/derived), possibly overridden with better algo or more suitible algo for a specific implementation.
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

    // Useful for use as a sample from available suggestions (e.g. for top-level popup menu)
    // Tries to return different meanings before different suggestions per meaning.
    // Words are unique, and different than the original word.
    virtual QStringList getThesaurusSuggestionSample(const QString& originalWord,
                                                     const QList<QStringList>& suggestions,
                                                     int upToCount,
                                                     bool* out_moreAvailable=0){
        QStringList result; // distinct words from the suggestions list
        QStringList tmpLower; tmpLower << originalWord.toLower().trimmed(); // manages case insensitive uniqueness
        if(out_moreAvailable) *out_moreAvailable = false;
        for (int k=0; k<2; k++) // first iteration for different meanings, 2nd for the rest of the suggestions
            for (int i = 0; i<suggestions.length(); i++)
                for (int j = 1; j<suggestions.at(i).length(); j++)
                    if(!tmpLower.contains(suggestions.at(i).at(j).toLower())) {
                        if(result.length()>=upToCount){ // the '>' part guards negative upToCount
                            if(out_moreAvailable) *out_moreAvailable = true;
                            return result; // Enough words and we know moreAvailable's value
                        }
                        tmpLower << suggestions.at(i).at(j).toLower();
                        result << suggestions.at(i).at(j);
                        if(!k) break; // First iteration and we got a suggestion from this meaning. Skip to next meaning.
                    }

        return result;
    }

    virtual QString strFromList(const QStringList& list){
        QString result = "";
        for (int i = 0; i<list.length(); i++)
            result += (i?", ":"") + list.at(i);
        return result;
    }

private:
    volatile bool _spellerEnabled; // For both speller and thesaurus
    volatile bool _thesaurusEnabled;

protected:
    volatile bool _thesaurusAvailable; // If thesaurus files where found for current language
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
