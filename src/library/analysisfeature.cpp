// preparefeature.cpp
// Created 8/23/2009 by RJ Ryan (rryan@mit.edu)
// Forked 11/11/2009 by Albert Santoni (alberts@mixxx.org)

#include <QtDebug>

#include "library/analysisfeature.h"
#include "library/librarytablemodel.h"
#include "library/trackcollection.h"
#include "dlganalysis.h"
#include "widget/wlibrary.h"
#include "mixxxkeyboard.h"
#include "analyserqueue.h"
#include "soundsourceproxy.h"
#include "util/dnd.h"
#include "util/debug.h"

const QString AnalysisFeature::m_sAnalysisViewName = QString("Analysis");

AnalysisFeature::AnalysisFeature(QObject* parent,
                               ConfigObject<ConfigValue>* pConfig,
                               TrackCollection* pTrackCollection) :
        LibraryFeature(parent),
        m_pConfig(pConfig),
        m_pTrackCollection(pTrackCollection),
        m_pAnalyserQueue(NULL),
        m_iOldBpmEnabled(0),
        m_analysisTitleName(tr("Analyze")),
        m_pAnalysisView(NULL) {
	setTitleDefault();
}

AnalysisFeature::~AnalysisFeature() {
    // TODO(XXX) delete these
    //delete m_pLibraryTableModel;
    cleanupAnalyser();
}

// Sets the title of this feature to the default name, given by m_sAnalysisTitleName
void AnalysisFeature::setTitleDefault() {
	m_Title = m_analysisTitleName;
	emit(featureIsLoading(this, false));	// Signals a change in title
}

// Sets the title of this feature to the default name followed by (x / y)
// where x is the current track being analyzed and y is the total number of tracks in the job
void AnalysisFeature::setTitleProgress(int trackNum, int totalNum) {
	QString title = m_analysisTitleName;
	title.append(" (");
	title.append(QString::number(trackNum));
	title.append(" / ");
	title.append(QString::number(totalNum));
	title.append(")");
	m_Title = title;
	emit(featureIsLoading(this, false));	// Signals a change in title
}

QVariant AnalysisFeature::title() {
    return m_Title;
}

QIcon AnalysisFeature::getIcon() {
    return QIcon(":/images/library/ic_library_prepare.png");
}

void AnalysisFeature::bindWidget(WLibrary* libraryWidget,
                                MixxxKeyboard* keyboard) {
    m_pAnalysisView = new DlgAnalysis(libraryWidget,
                                    m_pConfig,
                                    m_pTrackCollection);
    connect(m_pAnalysisView, SIGNAL(loadTrack(TrackPointer)),
            this, SIGNAL(loadTrack(TrackPointer)));
    connect(m_pAnalysisView, SIGNAL(loadTrackToPlayer(TrackPointer, QString)),
            this, SIGNAL(loadTrackToPlayer(TrackPointer, QString)));
    connect(m_pAnalysisView, SIGNAL(analyzeTracks(QList<int>)),
            this, SLOT(analyzeTracks(QList<int>)));
    connect(m_pAnalysisView, SIGNAL(stopAnalysis()),
            this, SLOT(stopAnalysis()));

    connect(this, SIGNAL(analysisActive(bool)),
            m_pAnalysisView, SLOT(analysisActive(bool)));
    connect(this, SIGNAL(trackAnalysisStarted(int)),
            m_pAnalysisView, SLOT(trackAnalysisStarted(int)));

    m_pAnalysisView->installEventFilter(keyboard);

    // Let the DlgAnalysis know whether or not analysis is active.
    bool bAnalysisActive = m_pAnalyserQueue != NULL;
    emit(analysisActive(bAnalysisActive));

    libraryWidget->registerView(m_sAnalysisViewName, m_pAnalysisView);
}

TreeItemModel* AnalysisFeature::getChildModel() {
    return &m_childModel;
}

void AnalysisFeature::refreshLibraryModels() {
    if (m_pAnalysisView) {
        m_pAnalysisView->onShow();
    }
}

void AnalysisFeature::activate() {
    //qDebug() << "AnalysisFeature::activate()";
    emit(switchToView(m_sAnalysisViewName));
    if (m_pAnalysisView) {
        emit(restoreSearch(m_pAnalysisView->currentSearch()));
    }
}

void AnalysisFeature::analyzeTracks(QList<int> trackIds) {
    if (m_pAnalyserQueue == NULL) {
        // Save the old BPM detection prefs setting (on or off)
        m_iOldBpmEnabled = m_pConfig->getValueString(ConfigKey("[BPM]","BPMDetectionEnabled")).toInt();
        // Force BPM detection to be on.
        m_pConfig->set(ConfigKey("[BPM]","BPMDetectionEnabled"), ConfigValue(1));
        // Note: this sucks... we should refactor the prefs/analyser to fix this hacky bit ^^^^.

        m_pAnalyserQueue = AnalyserQueue::createAnalysisFeatureAnalyserQueue(m_pConfig, m_pTrackCollection);

        connect(m_pAnalyserQueue, SIGNAL(trackProgress(int)),
                m_pAnalysisView, SLOT(trackAnalysisProgress(int)));
        connect(m_pAnalyserQueue, SIGNAL(trackFinished(int)),
                this, SLOT(slotProgressUpdate(int)));
        connect(m_pAnalyserQueue, SIGNAL(trackFinished(int)),
                m_pAnalysisView, SLOT(trackAnalysisFinished(int)));

        connect(m_pAnalyserQueue, SIGNAL(queueEmpty()),
                this, SLOT(cleanupAnalyser()));
        emit(analysisActive(true));
    }

    foreach(int trackId, trackIds) {
        TrackPointer pTrack = m_pTrackCollection->getTrackDAO().getTrack(trackId);
        if (pTrack) {
            //qDebug() << this << "Queueing track for analysis" << pTrack->getLocation();
            m_pAnalyserQueue->queueAnalyseTrack(pTrack);
        }
    }
    if(trackIds.size() > 0)
    	setTitleProgress(0, trackIds.size());
    emit(trackAnalysisStarted(trackIds.size()));
}

void AnalysisFeature::slotProgressUpdate(int num_left) {
	int num_tracks = m_pAnalysisView->getNumTracks();
	setTitleProgress(num_tracks - num_left, num_tracks);
}

void AnalysisFeature::stopAnalysis() {
    //qDebug() << this << "stopAnalysis()";
    if (m_pAnalyserQueue != NULL) {
        m_pAnalyserQueue->stop();
    }
}

void AnalysisFeature::cleanupAnalyser() {
	setTitleDefault();
    emit(analysisActive(false));
    if (m_pAnalyserQueue != NULL) {
        m_pAnalyserQueue->stop();
        m_pAnalyserQueue->deleteLater();
        m_pAnalyserQueue = NULL;
        // Restore old BPM detection setting for preferences...
        m_pConfig->set(ConfigKey("[BPM]","BPMDetectionEnabled"), ConfigValue(m_iOldBpmEnabled));
    }
}

bool AnalysisFeature::dropAccept(QList<QUrl> urls, QObject* pSource) {
    Q_UNUSED(pSource);
    QList<QFileInfo> files = DragAndDropHelper::supportedTracksFromUrls(urls, false, true);
    // Adds track, does not insert duplicates, handles unremoving logic.
    QList<int> trackIds = m_pTrackCollection->getTrackDAO().addTracks(files, true);
    analyzeTracks(trackIds);
    return trackIds.size() > 0;
}

bool AnalysisFeature::dragMoveAccept(QUrl url) {
    QFileInfo file(url.toLocalFile());
    return SoundSourceProxy::isFilenameSupported(file.fileName());
}