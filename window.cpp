#include <memory>
#include <iostream>

#include <QMessageBox>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QStandardPaths>
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QScreen>
#include <QCategoryAxis>
#include <QHBoxLayout>

#include "window.h"
#include "data_model.h"
#include "data_model_protobuf.h"
#include "eval_fast_rcnn_resnet101.h"

#include "data_vector.h"

using namespace QtCharts;
using namespace std;

std::mutex g_display_mutex;
/*******************************************************************************************/
/* free function */

/*******************************************************************************************/
/* Window */

Window::Window(QWidget *parent):QMainWindow(parent), 
                                m_scaleFactor(1),
                                m_lastScaleFactor(1),
                                m_mainWidget(new QWidget),
                                m_numDetectionsChart(new QChart),
                                m_detectionsSeries(new QLineSeries),
                                m_shadedArea(new QAreaSeries),
                                m_numDetectionsView(new QChartView),
                                axisX(new QValueAxis),
                                m_imageWidget(new ImageLabel),
                                m_scrollArea(new QScrollArea),
                                m_layout(new QVBoxLayout),
                                m_slider(new QSlider),
                                m_prevPoiButton(new QPushButton),
                                m_nextPoiButton(new QPushButton),
                                m_resetNumDetectionsButton(new QPushButton),
                                m_checkBox(new QCheckBox)
{
    // Configure image widget and scroll area
    m_imageWidget->setBackgroundRole(QPalette::Base);
    m_imageWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    // ensure that scaling the QLabel also scales the pixmap that is mapped onto the label
    m_imageWidget->setScaledContents(true);

    m_scrollArea->setAutoFillBackground(true);
    m_scrollArea->setWidget(m_imageWidget);
    // Center image, that is mapped onto QLabel
    m_scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_scrollArea->setVisible(false);

    // Add shaded area in graph
    QPen pen(0x059605);
    pen.setWidth(3);
    m_shadedArea->setPen(pen);

    // Add detections chart
    m_detectionsSeries->append(QPoint(0, 0));
    m_detectionsSeries->append(QPoint(1, 0));
    m_detectionsSeries->append(QPoint(2, 0));
    m_detectionsSeries->append(QPoint(3, 4));
    m_detectionsSeries->append(QPoint(4, 3));
    m_detectionsSeries->append(QPoint(5, 10));
    m_numDetectionsChart->addSeries(m_shadedArea);
    m_numDetectionsChart->addSeries(m_detectionsSeries);
    m_numDetectionsChart->legend()->setVisible(false);
    m_numDetectionsChart->setMargins(QMargins(1,1,1,1));

    axisX->setGridLineVisible(false);
    axisX->setTickCount(30);
    axisX->setRange(0, 40);  
    //m_detectionsSeries->attachAxis(axisX);
    m_numDetectionsChart->addAxis(axisX, Qt::AlignBottom);

    m_numDetectionsView->setChart(m_numDetectionsChart);
    m_numDetectionsView->setRenderHint(QPainter::Antialiasing);
    m_numDetectionsView->setRubberBand(QChartView::HorizontalRubberBand);
    /* reduce height of the widget */
    m_numDetectionsView->setMaximumHeight(100);
    m_numDetectionsView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_numDetectionsView->setVisible(false);

    // Create horizontal slider
    m_slider->setOrientation(Qt::Horizontal);
    m_slider->setRange(0,100);
    m_slider->setValue(0);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_prevPoiButton->setText("prev POI");
    m_nextPoiButton->setText("next POI");
    m_resetNumDetectionsButton->setText("Reset");
    m_prevPoiButton->setFocusPolicy(Qt::NoFocus);
    m_nextPoiButton->setFocusPolicy(Qt::NoFocus);
    m_resetNumDetectionsButton->setFocusPolicy(Qt::NoFocus);
    m_resetNumDetectionsButton->setMaximumWidth(50);
    m_resetNumDetectionsButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_checkBox->setText("Mis&detection");
    m_checkBox->setEnabled(false);
    buttonLayout->addWidget(m_prevPoiButton);
    buttonLayout->addWidget(m_nextPoiButton);
    buttonLayout->addWidget(m_checkBox);
    buttonLayout->addWidget(m_resetNumDetectionsButton);
    
    // add QWidgets to layout
    m_layout->addWidget(m_scrollArea);
    m_layout->addWidget(m_numDetectionsView);
    m_layout->addWidget(m_slider);
    m_layout->addLayout(buttonLayout);
    m_mainWidget->setLayout(m_layout);

    setCentralWidget(m_mainWidget);
    createActions();

    // setup event filters
    m_scrollArea->installEventFilter(this);

    // setup timer
    m_zoomTmr = make_unique<QTimer>();
    m_zoomTmr->setSingleShot(true);

    // connect signals
    connect( m_slider, SIGNAL( valueChanged(int) ), this, SLOT( updateImage(int) ) );
    connect( m_imageWidget, SIGNAL( mouseWheelUp() ), this, SLOT( zoomIn() ) );
    connect( m_imageWidget, SIGNAL( mouseWheelDown() ), this, SLOT( zoomOut() ) );
    connect( this, SIGNAL( detectionSeriesUpdated(QLineSeries*) ), this, SLOT( updateDetectionSeries(QLineSeries*) ) );
    connect( m_nextPoiButton, SIGNAL( clicked() ), this, SLOT( getNextPointOfInterest() ) );
    connect( m_prevPoiButton, SIGNAL( clicked() ), this, SLOT( getPreviousPointOfInterest() ) );
    connect( m_resetNumDetectionsButton, SIGNAL( clicked() ), this, SLOT( resetNumDetectionsView() ) );
    connect( m_checkBox, SIGNAL( clicked(bool) ), this, SLOT( flagAsMisdetection(bool) ) );
    connect( m_zoomTmr.get(), SIGNAL( timeout() ), this, SLOT( setupImgDelayed() ) );

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

bool Window::loadFile(const QString& fileName)
{
    std::cout << "opening file " << fileName.toStdString() << std::endl;
    shared_ptr< DataModel<DataModelProtoBuf <EvalFastRcnnResnet101>> > model = DataModelProtoBuf<EvalFastRcnnResnet101>::getInstance();
    
    model->open(fileName.toStdString());
    /* load data asynchronously */
    auto futptr = std::make_shared<std::future<void>>();
    /* create line object, to be filled in thread (all QWidgets need to be owned by the main thread) */
    QLineSeries* series = new QLineSeries;
    *futptr = std::async(std::launch::async, [this,futptr,model,series](){
        model->load();
        auto dets = model->getNumDetections(0);
        size_t numDataLoad = dets.size();
        for (unsigned x = 0; x < numDataLoad; x++)
        {
            series->append(QPoint(x, dets[x]));
        }
        emit this->detectionSeriesUpdated(series);
    });

    QString qimgPathStr = QString::fromStdString( model->getItemByIdx(0).string() );
    QImageReader reader(qimgPathStr);
    // apply rotation portrait/landscape according to EXIF metadata
    reader.setAutoTransform(true);
    // TODO: Images to be stored in a cache???
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        std::cout << "imag is null" << std::endl;
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                tr("Cannot load %1: %2")
                                .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }
    setImage(newImage);
    setWindowFilePath(fileName);
    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
        .arg(QDir::toNativeSeparators(fileName)).arg(m_image.width()).arg(m_image.height()).arg(m_image.depth());
    statusBar()->showMessage(message);
    // create annotation object
    auto path = fs::path(fileName.toStdString()).parent_path();
    auto anno_file = path / string("annotations");
    m_annotations = make_unique<Annotations>(anno_file);
    return true;
}

bool Window::saveToCsv(const QString& fileName)
{
    return m_annotations->dumpToFile(fs::path(fileName.toStdString()));
}

void Window::setImage(const QImage &newImage)
{
    // start time to resize the image if it has not bee updated
    // for some milli-seconds to reduce processing time for rescaling
    m_zoomTmr->start(100);
    m_image = newImage;
    m_imageWidget->setPixmap( QPixmap::fromImage(m_image) );
    m_scrollArea->setVisible(true);
    m_numDetectionsView->setVisible(true);
    m_fitToWindowAct->setEnabled(true);
    updateActions();

    if (!m_fitToWindowAct->isChecked())
    {
        m_imageWidget->adjustSize();
    }
}

void Window::createActions()
{
    // File menu
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    m_openAct = fileMenu->addAction(tr("&Open"), this, &Window::open);
    m_openAct->setShortcut(QKeySequence::Open);
    m_exportCsvAct = fileMenu->addAction(tr("&Export to csv"), this, &Window::exportFile);
    m_exportCsvAct->setEnabled(false);
    fileMenu->addSeparator();
    unique_ptr<QAction> exitAct( fileMenu->addAction(tr("E&xit"), this, &Window::close) );
    // View menu
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    m_zoomInAct = viewMenu->addAction(tr("Zoom &In (25%)"), this, &Window::zoomIn);
    m_zoomInAct->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAct->setEnabled(false);
    m_zoomOutAct = viewMenu->addAction(tr("Zoom &Out (25%)"), this, &Window::zoomOut);
    m_zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAct->setEnabled(false);
    m_normalSizeAct = viewMenu->addAction(tr("&Normal Size"), this, &Window::normalSize);
    m_normalSizeAct->setShortcut(tr("Ctrl+S"));
    m_normalSizeAct->setEnabled(false);
    viewMenu->addSeparator();
    m_fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this, &Window::fitToWindow);
    m_fitToWindowAct->setEnabled(false);
    m_fitToWindowAct->setCheckable(true);
    m_fitToWindowAct->setShortcut(tr("Ctrl+F"));
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction( tr("&About"), this, &Window::about );
}

void Window::updateActions()
{
    // Once an image is loaded, the zooming actions get enabled
    // but only if fit-to-window is not ticked
    m_zoomInAct->setEnabled(!m_fitToWindowAct->isChecked());
    m_zoomOutAct->setEnabled(!m_fitToWindowAct->isChecked());
    m_normalSizeAct->setEnabled(!m_fitToWindowAct->isChecked());
    m_checkBox->setEnabled(true);
    m_exportCsvAct->setEnabled(true);
}

void Window::scaleImage(double factor)
{
    m_scaleFactor *= factor;
    m_imageWidget->resize(m_scaleFactor * m_imageWidget->pixmap(Qt::ReturnByValue).size());
    adjustScrollBar(m_scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(m_scrollArea->verticalScrollBar(), factor);

    // bind scaling buttons to reasonable ranges
    m_zoomInAct->setEnabled(m_scaleFactor < 3.0);
    m_zoomOutAct->setEnabled(m_scaleFactor > 0.333);
}

void Window::adjustScrollBar( QScrollBar* scrollBar, double factor )
{
    scrollBar->setValue( int(factor * scrollBar->value()
                         + ((factor - 1) * scrollBar->pageStep()/2)) );
}

void Window::highlightImgBackground()
{
    QPalette pal = m_scrollArea->palette();
    pal.setColor(m_scrollArea->backgroundRole(), Qt::red);
    m_scrollArea->setPalette(pal);
}

void Window::resetImgBackground()
{
    QPalette pal = m_scrollArea->palette();
    pal.setColor(m_scrollArea->backgroundRole(), Qt::gray);
    m_scrollArea->setPalette(pal);
}

bool Window::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int sliderPos = m_slider->sliderPosition();
        switch(keyEvent->key())
        {
            case Qt::Key_Left:
                if ( keyEvent->modifiers() & Qt::ShiftModifier )
                {
                    getPreviousPointOfInterest();
                }
                else
                {
                    m_slider->setValue( std::max(m_slider->minimum(),sliderPos-1) );
                }
                return true;
            case Qt::Key_Right:
                if ( keyEvent->modifiers() & Qt::ShiftModifier )
                {
                    getNextPointOfInterest();
                }
                else
                {
                    m_slider->setValue( std::min(m_slider->maximum(),sliderPos+1) );
                }
                return true;
        }
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
}

/*******************************************************************************************/
/* Slots */

void Window::open()
{
    QFileDialog dialog(this, tr("Open File"));
    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}

void Window::exportFile()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export csv"));
	saveToCsv(fileName);
}

void Window::zoomIn()
{
    scaleImage(1.25);
}

void Window::zoomOut()
{
    scaleImage(0.8);
}

void Window::fitToWindow()
{
    bool fitToWindow = m_fitToWindowAct->isChecked();
    m_scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow)
    {
        normalSize();
    }
    updateActions();
}

void Window::normalSize()
{
    m_imageWidget->adjustSize();
    m_scaleFactor = 1.0;
}

void Window::about()
{
    QMessageBox::about(this, tr("This is a tool for object detection analysis"),
                        tr("there is not much more to tell ..."));
}

void Window::updateImage(int value)
{
    m_currentImgIdx = value;
    m_slider->setValue(value);
    shared_ptr< DataModel<DataModelProtoBuf <EvalFastRcnnResnet101>> > model = DataModelProtoBuf<EvalFastRcnnResnet101>::getInstance();
    m_imgPath = model->getItemByIdx(value);
    QString qimgPathStr = QString::fromStdString( m_imgPath.string() );

    QImageReader reader(qimgPathStr);
    // apply rotation portrait/landscape according to EXIF metadata
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (!newImage.isNull()) 
    {
        QLineSeries* top    = new QLineSeries();
        QLineSeries* bottom = new QLineSeries();
        top->append(QPoint(0,3));
        top->append(QPoint(value,3));
        bottom->append(QPoint(0,0));
        bottom->append(QPoint(value,0));
        m_shadedArea->setLowerSeries(bottom);
        m_shadedArea->setUpperSeries(top);
        if ( !(m_shadedArea->attachedAxes()).count() )
        {
            m_shadedArea->attachAxis(this->axisX);
        }
        setImage(newImage);
    }
}

void Window::updateDetectionSeries(QLineSeries* newSeries)
{
    auto numDataLoad = newSeries->count();
    this->m_slider->setRange(0,numDataLoad);
    this->axisX->setTickCount(11);
    this->axisX->setRange(0, numDataLoad);  
    // update series
    if (m_detectionsSeries)
    {
        m_numDetectionsChart->removeSeries(m_detectionsSeries);
        delete m_detectionsSeries;
    }
    m_numDetectionsChart->addSeries(newSeries);
    m_detectionsSeries = newSeries;
}

void Window::getNextPointOfInterest()
{
    auto model = DataModelProtoBuf<EvalFastRcnnResnet101>::getInstance();
    unsigned nextPoiIdx = model->nextPoi(m_currentImgIdx);
    m_slider->setValue( nextPoiIdx );
    updateImage( nextPoiIdx );
}

void Window::getPreviousPointOfInterest()
{
    auto model = DataModelProtoBuf<EvalFastRcnnResnet101>::getInstance();
    unsigned prevPoiIdx = model->prevPoi(m_currentImgIdx);
    m_slider->setValue( prevPoiIdx );
    updateImage( prevPoiIdx );
}

void Window::setupImgDelayed()
{
    bool isFlaggedAsMisdetect = m_annotations->isFlaggedAsMisdetection( m_imgPath.filename().string());
    if ( isFlaggedAsMisdetect )
    {
        highlightImgBackground();
        m_checkBox->setCheckState(Qt::Checked);
    }
    else
    {
        resetImgBackground();
        m_checkBox->setCheckState(Qt::Unchecked);
    }
    scaleImage(1);
}

void Window::flagAsMisdetection(bool isChecked)
{
    std::cout << "flagAsMisdetection(): check-state: " << isChecked << std::endl;
    if (isChecked)
    {
        highlightImgBackground();
        m_annotations->setFlagMisdetectByName( m_imgPath.filename() );
    }
    else
    {
        resetImgBackground();
        m_annotations->clearFlagMisdetectByName( m_imgPath.filename() );
    }
}

void Window::resetNumDetectionsView()
{
    m_numDetectionsChart->zoomReset();
}
