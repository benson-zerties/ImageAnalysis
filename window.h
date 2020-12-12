#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QChart>
#include <QChartView>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QLabel>
#include <QAction>
#include <QScrollArea>
#include <QScrollBar>
#include <QLineSeries>
#include <QValueAxis>
#include <QAreaSeries>
#include <QTimer>
#include <QCheckBox>

#include <memory>

#include "image_label.h"
#include "annotations.h"

using namespace QtCharts;
using namespace std;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    /**
     * \brief Enable/disable menu buttons depending on the application state.
     */
    void updateActions();
    
    /**
     * \brief Load a protobuf file, containing metadate of detection results.
     */
    bool loadFile(const QString &fileName);

     /**
     * \brief Load a protobuf file, containing metadate of detection results.
     */
    bool saveToCsv(const QString &fileName);
  
    /**
     * \brief Setup application menu: Create actions and populate the menu.
     */
    void createActions();
    
    /**
     * \brief Update image size
     */
    void scaleImage(double);
    
    void adjustScrollBar( QScrollBar* scrollBar, double factor );
    
    /**
     * \brief Update pixmap with new image
     */
    void setImage(const QImage &newImage);

    /**
     * \brief Highlight background of m_scrollArea
     */
    void highlightImgBackground();

    /**
     * \brief Reset background color of m_scrollArea
     */
    void resetImgBackground();

    double m_scaleFactor;
    double m_lastScaleFactor;
    uint32_t m_currentImgIdx = 0;
    unique_ptr<Annotations> m_annotations;
    fs::path m_imgPath;

    QWidget* m_mainWidget;
    QChart* m_numDetectionsChart;
    QLineSeries* m_detectionsSeries;
    QAreaSeries* m_shadedArea;
    QChartView* m_numDetectionsView;
    QValueAxis* axisX;
    ImageLabel* m_imageWidget;
    QScrollArea* m_scrollArea;
    QVBoxLayout* m_layout;
    QSlider* m_slider;
    QPushButton* m_prevPoiButton;
    QPushButton* m_nextPoiButton;
    QPushButton* m_resetNumDetectionsButton;
    QCheckBox* m_checkBox;
    unique_ptr<QTimer> m_zoomTmr;
    
    // Declare actions
    QAction* m_openAct;
    QAction* m_exportCsvAct;
    QAction* m_zoomInAct;
    QAction* m_zoomOutAct;
    QAction* m_normalSizeAct;
    QAction* m_fitToWindowAct;

    QImage m_image;

private slots:
    /**
     * \brief Slot for File->Open dialog
     */
    void open();

    /**
     * \brief Export file to csv.
     */ 
    void exportFile();
 
    /**
     * \brief Increase m_scaleFactor
     */
    void zoomIn();

    /**
     * \brief Reduce m_scaleFactor
     */
    void zoomOut();

    /**
     * \brief Fit to current window size
     */
    void fitToWindow();

    /**
     * \brief Undo all zooming
     */
    void normalSize();

    /**
     * \brief Show some info text about the application
     */
    void about();

    /**
     * \brief Change the displayed image
     */
    void updateImage(int);

    void updateDetectionSeries(QLineSeries* newSeries);
    
    /**
     * \brief Query next point of interest from data model
     */
    void getNextPointOfInterest();

    /**
     * \brief Query previous point of interest from data model
     */
    void getPreviousPointOfInterest();

    /**
     * \brief Setup new image:
     * 
     * Apply last zoom factor and highlight misdetections.
     */ 
    void setupImgDelayed();

    /**
     * \brief Flag image as misdetection
     */ 
    void flagAsMisdetection(bool);

    void resetNumDetectionsView();

signals:
    void detectionSeriesUpdated(QLineSeries* newSeries);
    

public slots:
};

#endif // WINDOW_H
