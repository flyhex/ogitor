/*/////////////////////////////////////////////////////////////////////////////////
/// An
///    ___   ____ ___ _____ ___  ____
///   / _ \ / ___|_ _|_   _/ _ \|  _ \
///  | | | | |  _ | |  | || | | | |_) |
///  | |_| | |_| || |  | || |_| |  _ <
///   \___/ \____|___| |_| \___/|_| \_\
///                              File
///
/// Copyright (c) 2008-2015 Ismail TARIM <ismail@royalspor.com> and the Ogitor Team
///
/// The MIT License
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////*/

#include "application.hxx"
#include "mainwindow.hxx"
#include "shortcuts.hxx"

#include "paths.hxx"
#include "qtogitorsystem.hxx"
#include "BaseEditor.h"
#include "ViewportEditor.h"
#include "OgitorsRoot.h"
#include "OgitorsGlobals.h"
#include "ogrewidget.hxx"

#include <QtCore/QTextCodec>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>
#include <QtCore/QDir>

#include <QtWidgets/QSplashScreen>

#include <OgreApplicationContextQt.h>

extern bool    ViewKeyboard[1024];
extern QString ConvertToQString(Ogre::UTFString& value);

Ogre::Root           *mOgreRoot;
Ogitors::OgitorsRoot *mOgitorsRoot;
QtOgitorSystem       *mSystem;
Shortcuts            *mShortCuts;

static void setupOgitors();
static void readRecentFiles(QSettings& settings);

struct OgreContext : public OgreBites::ApplicationContextQt
{
    OgreContext() : OgreBites::ApplicationContextQt("Ogitor") {}

    void createRoot()
    {
        OgreBites::ApplicationContextQt::createRoot();
        mOgreRoot = getRoot();

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        // load additional plugins
        mOgreRoot->loadPlugin(Ogitors::Globals::LIBOGREOFSPLUGIN_PATH + "/libOgreOfsPlugin.so");
#else
        mOgreRoot->loadPlugin("OgreOfsPlugin");
#endif

        QSettings settings;
        setupOgitors();
        readRecentFiles(settings);
        useQtEventLoop(true);
        mOgitorMainWindow = new MainWindow();
        mOgitorMainWindow->show();
        injectMainWindow(mOgitorMainWindow->getOgreWidget()->windowHandle());
    }

    bool oneTimeConfig()
    {
        Ogre::LogManager::getSingleton().setLogDetail(Ogre::LL_NORMAL);

        QSettings settings;
        settings.beginGroup("preferences");
        QString renderer = settings.value("renderSystem", "").toString();

        Ogre::RenderSystemList::const_iterator pRend = mOgreRoot->getAvailableRenderers().begin();
        Ogre::RenderSystem *rsys = *pRend;

        while (pRend != mOgreRoot->getAvailableRenderers().end())
        {
            Ogre::String rName = (*pRend)->getName();

            if (rName == renderer.toStdString())
            {
                rsys = *pRend;
                break;
            }

            pRend++;
        }

        // Set the rendering system and initialise OGRE
        mOgreRoot->setRenderSystem(rsys);
        mOgreRoot->getRenderSystem()->setConfigOption("Full Screen", "No");

        int antialias = settings.value("antiAliasing", 4).toInt();
        bool vsync = settings.value("useVSync", true).toBool();
        mOgreRoot->getRenderSystem()->setConfigOption(
            "VSync", Ogre::StringConverter::toString(vsync));
        mOgreRoot->getRenderSystem()->setConfigOption("FSAA", std::to_string(antialias));
        settings.endGroup();

        return true;
    }

    void setupReal()
    {
        OgreBites::ApplicationContextQt::setup();
    }

    void locateResources()
    {
        // ogre.cfg already resolved, so:
        mFSLayer->setConfigPaths({resourcePath()});
        Ogre::ResourceGroupManager::getSingletonPtr()->setLoadingListener(new Ogitors::ResourceLoadingListener());
        OgreBites::ApplicationContextQt::locateResources();
    }
};

//-------------------------------------------------------------------------------------
static void setupOgitors()
{
    QSettings settings;
    settings.beginGroup("preferences");
    bool azerty = settings.value("useAZERTY", false).toBool();
    settings.endGroup();

    Ogitors::OgitorsSpecialKeys keys;
    /* Shortcuts class was created only with getInstance() and because of this it's destructor never ran */
    mShortCuts = new Shortcuts();//Shortcuts::getInstance();
    mShortCuts->load();
    //tmp->add(QKeySequence(Qt::Key_W).toString(), QString("Move Forward"), "SPK_FORWARD", Qt::Key_W);
    //tmp->add(QKeySequence(Qt::Key_A).toString(), QString("Move Left"), "SPK_LEFT", Qt::Key_A);
    //tmp->add(QKeySequence(Qt::Key_S).toString(), QString("Move Backward"), "SPK_BACKWARD", Qt::Key_S);
    //tmp->add(QKeySequence(Qt::Key_D).toString(), QString("Move Right"), "SPK_RIGHT", Qt::Key_D);

    if(azerty)
    {
        // azerty keys
        keys.SPK_LEFT = Qt::Key_Q;
        keys.SPK_RIGHT = Qt::Key_D;
        keys.SPK_FORWARD = Qt::Key_Z;
        keys.SPK_BACKWARD = Qt::Key_S;
        keys.SPK_UP = Qt::Key_E;
        keys.SPK_DOWN = Qt::Key_A;
    }
    else
    {
        // normal keys
        keys.SPK_LEFT = Qt::Key_A;
        keys.SPK_RIGHT = Qt::Key_D;
        keys.SPK_FORWARD = Qt::Key_W;
        keys.SPK_BACKWARD = Qt::Key_S;
        keys.SPK_UP = Qt::Key_E;
        keys.SPK_DOWN = Qt::Key_Q;
    }

    // special keys
    keys.SPK_FOCUS_OBJECT = Qt::Key_F;
    keys.SPK_DELETE = (Qt::Key_Delete & 0xFFF) + 0xFF;
    keys.SPK_SWITCH_AXIS = (Qt::Key_End & 0xFFF) + 0xFF;
    // shift
    keys.SPK_CLONE = (Qt::Key_Shift & 0xFFF) + 0xFF;
    keys.SPK_REVERSE_UPDATE = (Qt::Key_Shift & 0xFFF) + 0xFF;
    keys.SPK_ADD_TO_SELECTION = (Qt::Key_Shift & 0xFFF) + 0xFF;
    // control
    keys.SPK_SNAP = (Qt::Key_Control & 0xFFF) + 0xFF;;
    keys.SPK_SUBTRACT_FROM_SELECTION = (Qt::Key_Control & 0xFFF) + 0xFF;
    // alt
    keys.SPK_OBJECT_CENTERED_MOVEMENT = (Qt::Key_Alt & 0xFFF) + 0xFF;
    keys.SPK_ALWAYS_SELECT = (Qt::Key_Alt & 0xFFF) + 0xFF;

    Ogitors::CViewportEditor::SetKeyboard(ViewKeyboard, keys);

    mSystem = OGRE_NEW QtOgitorSystem();

    // Read the preferences concerning disabled plugins and pass on to OgitorsRoot
    Ogre::StringVector disabledPluginPaths;
    settings.beginGroup("preferences/disabledPlugins");
    QStringList keyList = settings.childKeys();
    for(int i = 0; i < keyList.count(); i++)
        disabledPluginPaths.push_back(settings.value(keyList[i]).toString().toStdString());
    settings.endGroup();
    mOgitorsRoot = OGRE_NEW Ogitors::OgitorsRoot(&disabledPluginPaths);
}
//------------------------------------------------------------------------------------
static void readRecentFiles(QSettings& settings)
{
    int recentFileCount = settings.value("recentfiles/count").toInt();
    Ogitors::UTFStringVector recentList;
    recentList.clear();
    for(int i = 0;i < recentFileCount;i++)
    {
        QString value = settings.value(QString("recentfiles/entry%1").arg(i)).toString();

        if(!QFile::exists(value))
            continue;

        recentList.push_back(value.toStdString());
    }

    Ogitors::OgitorsRoot::getSingletonPtr()->InitRecentFiles(recentList);
}
//-------------------------------------------------------------------------------------
void writeRecentFiles()
{
    QSettings settings;

    settings.beginGroup("recentfiles");

    Ogitors::UTFStringVector recentList;
    Ogitors::OgitorsRoot::getSingletonPtr()->GetRecentFiles(recentList);

    int recentFileCount = recentList.size();
    if(recentFileCount > 5)
        recentFileCount = 5;

    settings.setValue("count", recentFileCount);

    for(int i = 0;i < recentFileCount;i++)
    {
        QString value = ConvertToQString(recentList[i]);
        settings.setValue(QString("entry%1").arg(i), value);
    }
    settings.endGroup();
}
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    Application app(argc, argv);

    OgreContext ogreCtx;

    QDir::setCurrent(app.applicationDirPath());

    // See if we are loading something from the command line.
    QString fileArg("");
    QString suffix("");
    QString argument = app.arguments().last();
    QFileInfo info(app.arguments().last());
    if(info.exists() && info.isFile())
    {
        if(info.suffix() == "ofs")
        {
            // Yes we've got an .OFS file to load.
            fileArg = info.absoluteFilePath();
            suffix = info.suffix();
        }
        else if(info.baseName() != "qtOgitor" && info.baseName() != "qtOgitor_d")
        {
            // We are trying to load something we can't load
            // Exit the application.
            QMessageBox msg(QMessageBox::Critical, "Ogitor Error", "We can only load Ogitor Scenes.\nExiting...");
            msg.exec();
            return 0;
        }
    }

    app.setOrganizationName(QString("Ogitor"));
    app.setApplicationName(QString("Ogitor ") + QString(Ogitors::Globals::OGITOR_VERSION.c_str()));

    QSettings settings;
    QString languageFile = settings.value("preferences/customLanguage").toString();

    QTranslator qtTranslator;
    QTranslator ogitorTranslator;

    if(languageFile != "ogitor_en.qm")
    {
        QString lang = QString(Ogitors::Globals::LANGUAGE_PATH.c_str()) + QString("/qt_") + languageFile;
        lang.remove("ogitor_");

        QString lang2 = QString(Ogitors::Globals::LANGUAGE_PATH.c_str()) + QString("/") + languageFile;

        if(QFile::exists(lang) && QFile::exists(lang2))
        {
            if(qtTranslator.load(lang))
                app.installTranslator(&qtTranslator);

            if(ogitorTranslator.load(lang2))
                app.installTranslator(&ogitorTranslator);
        }
        else
        {
            // If the system-wide Qt translation file is present, load it.
            if(qtTranslator.load("qt_" + QLocale::system().name(),
                QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            {
                app.installTranslator(&qtTranslator);
            }
            // Otherwise: load our own Qt translation file.
            else if(qtTranslator.load(QString(Ogitors::Globals::LANGUAGE_PATH.c_str()) + QString("/qt_") + QLocale::system().name()))
            {
                app.installTranslator(&qtTranslator);
            }

            // Install qtOgitor translator
            if(ogitorTranslator.load(QString(Ogitors::Globals::LANGUAGE_PATH.c_str()) + QString("/ogitor_") + QLocale::system().name()))
            {
                app.installTranslator(&ogitorTranslator);
            }
        }
    }

    bool skipSplash = settings.value("preferences/skipSplash", false).toBool();

    QSplashScreen *splash;
    if(!skipSplash)
    {
        QPixmap pixmap(":/images/OgitorLogo.png");
        splash = new QSplashScreen(pixmap);
        splash->show();
    }

    ogreCtx.initApp();
    mOgitorsRoot->SetRenderWindow(ogreCtx.getRenderWindow());
    mOgitorMainWindow->getOgreWidget()->initializeOGRE();

    mOgitorMainWindow->setApplicationObject(&app);

    QString sceneToLoad = "";
    QString lastLoadedScene = settings.value("preferences/lastLoadedScene", "").toString();
    if((settings.value("preferences/loadLastLoadedScene", false).toBool()) && (fileArg.isEmpty()))
    {
        if(!lastLoadedScene.isEmpty())
        {
            sceneToLoad = lastLoadedScene;
        }
    }
    else if(!fileArg.isEmpty())
    {
        sceneToLoad = fileArg;
    }

    if(!sceneToLoad.isEmpty())
    {
        if(suffix == "material")
        {
        }
        else
        {
            Ogitors::OgitorsRoot::getSingletonPtr()->LoadScene(sceneToLoad.toStdString());
        }
    }

    mOgitorMainWindow->repaint();

    if(!skipSplash)
    {
        splash->finish(mOgitorMainWindow);
        delete splash;
    }

    int retval = app.exec();

    writeRecentFiles();

    delete mOgitorMainWindow;
    delete mShortCuts;
    OGRE_DELETE mOgitorsRoot;
    OGRE_DELETE mSystem;

    return retval;
}
//-------------------------------------------------------------------------------------
