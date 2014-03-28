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

        ../../3rdparty/hunspell/affentry.cxx
        ../../3rdparty/hunspell/affixmgr.cxx
        ../../3rdparty/hunspell/csutil.cxx
        ../../3rdparty/hunspell/dictmgr.cxx
        ../../3rdparty/hunspell/filemgr.cxx
        ../../3rdparty/hunspell/hashmgr.cxx
        ../../3rdparty/hunspell/hunspell.cxx
        ../../3rdparty/hunspell/hunzip.cxx
        ../../3rdparty/hunspell/phonet.cxx
        ../../3rdparty/hunspell/replist.cxx
        ../../3rdparty/hunspell/suggestmgr.cxx

        ../../3rdparty/mythes/mythes.cxx
    )
    set(MOC_HDRS ${MOC_HDRS}
        speller/speller_qtextedit.h
    )
    set(HEADERS ${HEADERS}
        speller/speller_adapter.h

        ../../3rdparty/hunspell/affentry.hxx
        ../../3rdparty/hunspell/affixmgr.hxx
        ../../3rdparty/hunspell/atypes.hxx
        ../../3rdparty/hunspell/baseaffix.hxx
        ../../3rdparty/hunspell/config.h
        ../../3rdparty/hunspell/csutil.hxx
        ../../3rdparty/hunspell/dictmgr.hxx
        ../../3rdparty/hunspell/filemgr.hxx
        ../../3rdparty/hunspell/hashmgr.hxx
        ../../3rdparty/hunspell/htypes.hxx
        ../../3rdparty/hunspell/hunspell.h
        ../../3rdparty/hunspell/hunspell.hxx
        ../../3rdparty/hunspell/hunvisapi.h
        ../../3rdparty/hunspell/hunzip.hxx
        ../../3rdparty/hunspell/langnum.hxx
        ../../3rdparty/hunspell/phonet.hxx
        ../../3rdparty/hunspell/replist.hxx
        ../../3rdparty/hunspell/suggestmgr.hxx
        ../../3rdparty/hunspell/w_char.hxx

        ../../3rdparty/mythes/mythes.hxx
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
