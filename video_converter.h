#ifndef VIDEO_CONVERTER_H
#define VIDEO_CONVERTER_H

#include <QWidget>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QProcess>
#include <QTimer>

class VideoConverter : public QWidget {
    Q_OBJECT

public:
    VideoConverter(QWidget *parent = nullptr);

private slots:
    void browse();
    void convert();
    void updateProgress();
    void handleError(QProcess::ProcessError error);

private:
    QLineEdit *filePathEdit;
    QSlider *bitrateSlider;
    QSpinBox *bitrateSpinBox;
    QComboBox *resolutionComboBox;
    QProgressBar *progressBar;
    QProcess *conversionProcess;
    QTimer *progressTimer;
    int totalSeconds;
};

#endif // VIDEO_CONVERTER_H

