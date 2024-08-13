#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QProcess>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QTime>
#include <QRegularExpression>
#include <QMessageBox>

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
    QLabel *qualityLabel; // Add a QLabel pointer
    int totalSeconds; // Duración total del video en segundos

    int getVideoDuration(const QString &filePath);
    // Helper function to generate the output filename with .mp4 extension
    QString generateOutputFilename(const QString &baseName) const;
};

VideoConverter::VideoConverter(QWidget *parent) : QWidget(parent), totalSeconds(0) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    QPushButton *browseButton = new QPushButton("Browse", this);
    layout->addWidget(browseButton);

    filePathEdit = new QLineEdit(this);
    layout->addWidget(filePathEdit);

    qualityLabel = new QLabel("Calidad (bitrate) :", this); // Create the label
    layout->addWidget(qualityLabel); // Add the label to the layout

    bitrateSlider = new QSlider(Qt::Horizontal, this);
    bitrateSlider->setRange(500, 5000);
    layout->addWidget(bitrateSlider);

    bitrateSpinBox = new QSpinBox(this);
    bitrateSpinBox->setRange(500, 5000);
    layout->addWidget(bitrateSpinBox);

    connect(bitrateSlider, &QSlider::valueChanged, bitrateSpinBox, &QSpinBox::setValue);
    connect(bitrateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), bitrateSlider, &QSlider::setValue);

    QCheckBox *resolutionCheckbox = new QCheckBox("Enable Resolution Adjustment", this);
    layout->addWidget(resolutionCheckbox);

    resolutionComboBox = new QComboBox(this);
    resolutionComboBox->addItem("1920x1080", QVariant("1920:1080"));
    resolutionComboBox->addItem("1280x720", QVariant("1280:720"));
    resolutionComboBox->addItem("640x480", QVariant("640:480"));
    resolutionComboBox->addItem("320x240", QVariant("320:240"));
    resolutionComboBox->setEnabled(false);
    layout->addWidget(resolutionComboBox);

    connect(resolutionCheckbox, &QCheckBox::toggled, resolutionComboBox, &QComboBox::setEnabled);

    QPushButton *convertButton = new QPushButton("Convert", this);
    layout->addWidget(convertButton);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    layout->addWidget(progressBar);

    connect(browseButton, &QPushButton::clicked, this, &VideoConverter::browse);
    connect(convertButton, &QPushButton::clicked, this, &VideoConverter::convert);

    conversionProcess = new QProcess(this);
    connect(conversionProcess, &QProcess::readyReadStandardError, this, &VideoConverter::updateProgress);
    connect(conversionProcess, &QProcess::errorOccurred, this, &VideoConverter::handleError);

    progressTimer = new QTimer(this);
    connect(progressTimer, &QTimer::timeout, this, &VideoConverter::updateProgress);

    setLayout(layout);
}

void VideoConverter::browse() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Video File", "", "Video Files (*.mp4 *.avi *.mov)");
    if (!filePath.isEmpty()) {
        filePathEdit->setText(filePath);
        totalSeconds = getVideoDuration(filePath);  // Obtener la duración real
        if (totalSeconds <= 0) {
            QMessageBox::warning(this, "Error", "No se pudo obtener la duración del video.");
            return;
        }
    }
}

int VideoConverter::getVideoDuration(const QString &filePath) {
    QProcess process;
    process.start("ffprobe", QStringList() << "-v" << "error" << "-show_entries" << "format=duration" << "-of" << "default=noprint_wrappers=1:nokey=1" << filePath);
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput().trimmed();
    return output.toDouble();  // Duración en segundos
}

QString VideoConverter::generateOutputFilename(const QString &baseName) const {
  // This function builds the output filename with .mp4 extension
  return QString("%1.mp4").arg(baseName);
}

void VideoConverter::convert() {
    QString inputFile = filePathEdit->text();
    if (inputFile.isEmpty()) return;

    // Get the base filename without extension (assuming no path separators in filename)
    QString baseName = QFileInfo(inputFile).baseName();

    // Generate the output filename with .mp4 extension using the helper function
    QString outputFile = generateOutputFilename(baseName);

    int bitrate = bitrateSlider->value();
    QStringList arguments;
    arguments << "-i" << inputFile;

    if (resolutionComboBox->isEnabled()) {
        QString resolution = resolutionComboBox->currentData().toString();
        arguments << "-vf" << QString("scale=%1").arg(resolution);
    }

    arguments << "-b:v" << QString("%1k").arg(bitrate)
              << "-c:v" << "libx264"
              << "-c:a" << "aac"
              << outputFile;

    progressBar->setValue(0);
    progressTimer->start(1000); // Actualizar cada segundo
    conversionProcess->start("ffmpeg", arguments);
}



void VideoConverter::updateProgress() {
    QString output = conversionProcess->readAllStandardError();  // Leer la salida de error estándar (donde ffmpeg escribe el progreso)
    QRegularExpression timeRegex("time=([\\d:.]+)");
    QRegularExpressionMatch match = timeRegex.match(output);
    
    if (match.hasMatch()) {
        QString timeString = match.captured(1);
        QStringList timeParts = timeString.split(':');
        double secondsElapsed = 0;
        
        if (timeParts.size() == 3) {
            secondsElapsed = timeParts[0].toDouble() * 3600 + timeParts[1].toDouble() * 60 + timeParts[2].toDouble();
        } else if (timeParts.size() == 2) {
            secondsElapsed = timeParts[0].toDouble() * 60 + timeParts[1].toDouble();
        }

        int progress = static_cast<int>((secondsElapsed / totalSeconds) * 100);
        progressBar->setValue(progress);
    }

    if (conversionProcess->state() == QProcess::NotRunning) {
        progressTimer->stop();
        progressBar->setValue(100);  // Completar la barra de progreso
    }
}

void VideoConverter::handleError(QProcess::ProcessError error) {
    // Manejar diferentes tipos de errores
    QString errorMessage;

    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "Failed to start the conversion process.";
            break;
        case QProcess::Crashed:
            errorMessage = "The conversion process crashed.";
            break;
        case QProcess::Timedout:
            errorMessage = "The conversion process timed out.";
            break;
        case QProcess::WriteError:
            errorMessage = "Write error occurred during conversion.";
            break;
        case QProcess::ReadError:
            errorMessage = "Read error occurred during conversion.";
            break;
        case QProcess::UnknownError:
        default:
            errorMessage = "An unknown error occurred.";
            break;
    }

    progressTimer->stop();
    progressBar->setValue(0);
    QMessageBox::critical(this, "Conversion Error", errorMessage);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    VideoConverter converter;
    converter.setWindowTitle("CIEM: Conversor de video");
    converter.resize(300, 200);
    converter.show();

    return app.exec();
}
