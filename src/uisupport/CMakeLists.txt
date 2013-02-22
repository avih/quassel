# Builds the uisupport module

set(SOURCES
    aboutdata.cpp
    abstractbuffercontainer.cpp
    abstractitemview.cpp
    action.cpp
    actioncollection.cpp
    bufferhotlistfilter.cpp
    bufferview.cpp
    bufferviewfilter.cpp
    bufferviewoverlayfilter.cpp
    clearablelineedit.cpp
    clickable.cpp
    clickablelabel.cpp
    colorbutton.cpp
    contextmenuactionprovider.cpp
    flatproxymodel.cpp
    fontselector.cpp
    graphicalui.cpp
    multilineedit.cpp
    networkmodelcontroller.cpp
    nickview.cpp
    nickviewfilter.cpp
    qssparser.cpp
    resizingstackedwidget.cpp
    settingspage.cpp
    styledlabel.cpp
    tabcompleter.cpp
    toolbaractionprovider.cpp
    treeviewtouch.cpp
    uisettings.cpp
    uistyle.cpp

    # needed for automoc
    abstractnotificationbackend.h
)

if (WITH_KDE4)
    include_directories(${KDE4_INCLUDES})
    add_definitions(-DHAVE_KDE ${KDE4_DEFINITIONS})
    set(SOURCES ${SOURCES} kcmdlinewrapper.cpp)
endif()

if(CUSTOM_SPELLER)
    set(SOURCES ${SOURCES}
        speller/speller_qtextedit.cpp
        speller/hunspell_adapter.cpp

        ../ext/hunspell/affentry.cxx
        ../ext/hunspell/affixmgr.cxx
        ../ext/hunspell/csutil.cxx
        ../ext/hunspell/dictmgr.cxx
        ../ext/hunspell/filemgr.cxx
        ../ext/hunspell/hashmgr.cxx
        ../ext/hunspell/hunspell.cxx
        ../ext/hunspell/hunzip.cxx
        ../ext/hunspell/phonet.cxx
        ../ext/hunspell/replist.cxx
        ../ext/hunspell/suggestmgr.cxx

        ../ext/mythes/mythes.cxx
    )
    set(MOC_HDRS ${MOC_HDRS}
        speller/speller_qtextedit.h
    )
    set(HEADERS ${HEADERS}
        speller/speller_adapter.h

        ../ext/hunspell/affentry.hxx
        ../ext/hunspell/affixmgr.hxx
        ../ext/hunspell/atypes.hxx
        ../ext/hunspell/baseaffix.hxx
        ../ext/hunspell/config.h
        ../ext/hunspell/csutil.hxx
        ../ext/hunspell/dictmgr.hxx
        ../ext/hunspell/filemgr.hxx
        ../ext/hunspell/hashmgr.hxx
        ../ext/hunspell/htypes.hxx
        ../ext/hunspell/hunspell.h
        ../ext/hunspell/hunspell.hxx
        ../ext/hunspell/hunvisapi.h
        ../ext/hunspell/hunzip.hxx
        ../ext/hunspell/langnum.hxx
        ../ext/hunspell/phonet.hxx
        ../ext/hunspell/replist.hxx
        ../ext/hunspell/suggestmgr.hxx
        ../ext/hunspell/w_char.hxx

        ../ext/mythes/mythes.hxx
    )
endif(CUSTOM_SPELLER)

include_directories(${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/client
)

if (USE_QT5)
    list(APPEND qt_modules Widgets)
endif()

add_library(mod_uisupport STATIC ${SOURCES})
qt_use_modules(mod_uisupport Core Gui Network ${qt_modules})

target_link_libraries(mod_uisupport mod_client mod_common)

if (WITH_KDE4)
    target_link_libraries(mod_uisupport ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBRARY})
endif()

if (WITH_KF5)
    target_link_libraries(mod_uisupport KF5::CoreAddons KF5::TextWidgets KF5::XmlGui)
elseif (KF5Sonnet_FOUND)
    add_definitions(-DHAVE_SONNET)
    target_link_libraries(mod_uisupport KF5::SonnetUi)
endif()
