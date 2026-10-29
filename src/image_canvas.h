#pragma once

#include <QGraphicsView>
#include <QImage>

class QGraphicsPixmapItem;
class QGraphicsScene;

class ImageCanvas final : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageCanvas(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void fitImage();
    void zoomBy(qreal factor);

signals:
    void imagePixelHovered(const QPoint& pixel);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QGraphicsScene* m_scene = nullptr;
    QGraphicsPixmapItem* m_imageItem = nullptr;
    bool m_fitOnResize = true;
};
