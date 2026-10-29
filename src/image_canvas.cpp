#include "image_canvas.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include <cmath>

ImageCanvas::ImageCanvas(QWidget* parent)
    : QGraphicsView(parent),
      m_scene(new QGraphicsScene(this)),
      m_imageItem(m_scene->addPixmap(QPixmap()))
{
    setScene(m_scene);
    setBackgroundBrush(QColor(QStringLiteral("#17202b")));
    setRenderHint(QPainter::SmoothPixmapTransform, false);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
}

void ImageCanvas::setImage(const QImage& image)
{
    const bool hadImage = !m_imageItem->pixmap().isNull();
    const bool shouldFit = !hadImage || m_fitOnResize;
    m_imageItem->setPixmap(QPixmap::fromImage(image));
    m_scene->setSceneRect(QRectF(QPointF(0, 0), QSizeF(image.size())));
    if (shouldFit) {
        fitImage();
    }
}

void ImageCanvas::fitImage()
{
    if (m_imageItem->pixmap().isNull()) {
        return;
    }
    resetTransform();
    fitInView(m_imageItem, Qt::KeepAspectRatio);
    m_fitOnResize = true;
}

void ImageCanvas::zoomBy(qreal factor)
{
    if (m_imageItem->pixmap().isNull()) {
        return;
    }
    scale(factor, factor);
    m_fitOnResize = false;
}

void ImageCanvas::wheelEvent(QWheelEvent* event)
{
    if (m_imageItem->pixmap().isNull()) {
        event->ignore();
        return;
    }

    const qreal factor = event->angleDelta().y() > 0 ? 1.25 : 0.8;
    zoomBy(factor);
    event->accept();
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* event)
{
    const QPointF scenePosition = mapToScene(event->position().toPoint());
    const QPoint pixel(static_cast<int>(std::floor(scenePosition.x())),
                       static_cast<int>(std::floor(scenePosition.y())));
    if (m_imageItem->boundingRect().contains(scenePosition)) {
        emit imagePixelHovered(pixel);
    } else {
        emit imagePixelHovered(QPoint(-1, -1));
    }
    QGraphicsView::mouseMoveEvent(event);
}

void ImageCanvas::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (m_fitOnResize) {
        fitImage();
    }
}
