#ifndef IMAGE_LABEL_H
#define IMAGE_LABEL_H

#include <QLabel>
#include <QWheelEvent>

#include <memory>

using namespace std;

class ImageLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ImageLabel(QLabel *parent = nullptr);

protected:
	void wheelEvent(QWheelEvent* event);

signals:
	void mouseWheelUp();
	void mouseWheelDown();

};
 
#endif /* IMAGE_LABEL_H */
