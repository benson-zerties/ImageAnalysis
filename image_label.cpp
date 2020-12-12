#include "image_label.h"

#include <iostream>

#include <QPoint>

using namespace std;

/*******************************************************************************************/
/* ImageLabel */

ImageLabel::ImageLabel(QLabel *parent):QLabel(parent)
{
}

void ImageLabel::wheelEvent(QWheelEvent* event)
{
	QPoint numDegrees = event->angleDelta() / 8;
	// Mousewheel delta indication is
	//  * not 0
	//  * ctrl modifier key is pressed
	if ( !numDegrees.isNull() &&
		( event->modifiers() & Qt::ControlModifier ) )
	{
		if (numDegrees.y() < 0)
		{
			// scroll down
			emit mouseWheelDown();
		} else {
			// scroll up
			emit mouseWheelUp();
		}
		event->accept();
	} else {
		event->ignore();
	}
}
