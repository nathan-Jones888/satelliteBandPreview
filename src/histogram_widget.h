#pragma once

#include <QWidget>

#include <array>

class HistogramWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit HistogramWidget(QWidget* parent = nullptr);

    void setHistograms(
        const std::array<std::array<quint64, 256>, 3>& histograms);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::array<std::array<quint64, 256>, 3> m_histograms{};
    bool m_hasData = false;
};
