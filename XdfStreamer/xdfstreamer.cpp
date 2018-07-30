#include "xdfstreamer.h"
#include "ui_xdfstreamer.h"
#include "lsl_cpp.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <ctime>

XdfStreamer::XdfStreamer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::XdfStreamer)
{
    ui->setupUi(this);
    ui->spinBox->setMaximum(100000);
    ui->spinBox->setValue(10000);
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    ui->spinBox_2->setValue(32);
    ui->label_3->hide();
    ui->lineEdit_2->hide();
    ui->label_4->hide();
    ui->spinBox_2->hide();
    ui->label_5->hide();
    ui->lineEdit_3->setText("EEG");
    ui->lineEdit_3->hide();
    ui->groupBox_2->hide();

    QStringList header = {"Property", "Value"};
    ui->treeWidget->setHeaderLabels(header);
    ui->treeWidget->header()->hide();
    ui->treeWidget->setColumnCount(2);

    ui->treeWidget_2->setHeaderLabels(header);
    ui->treeWidget_2->header()->hide();
    ui->treeWidget_2->setColumnCount(2);
    ui->treeWidget_2->hide();

    setWindowTitle("XDF Streamer");
}

XdfStreamer::~XdfStreamer()
{
    delete ui;
}

void XdfStreamer::pushRandomSamples()
{
    QString streamName = ui->lineEdit_2->text();
    const int samplingRate = ui->spinBox->value();
    int channelCount = ui->spinBox_2->value();
    lsl::stream_info info(streamName.toStdString(), "EEG", channelCount, (double)samplingRate, lsl::cf_double64, "RT_Sender_SimulationPC");
    lsl::stream_outlet outlet(info);

    const double dSamplingInterval = 1.0 / samplingRate;
    std::vector<double> sample(channelCount);

    double starttime = ((double)clock()) / CLOCKS_PER_SEC;

    for (unsigned t = 0;; t++) {
        {
            std::lock_guard<std::mutex> guard(this->mutex_stop_thread);
            if (this->stop_thread) {
                return;
            }
        }

        while (((double)clock()) / CLOCKS_PER_SEC < starttime + t * dSamplingInterval);

        for (int c = 0; c < channelCount; c++) {
            sample[c] = (rand() % 1500) / 500.0 - 1.5;
        }

        outlet.push_sample(sample);
    }
}

void XdfStreamer::pushXdfData()
{
    std::string streamName = this->xdf->streams[this->stream_idx].info.name;
    const int samplingRate = ui->spinBox->value();
    size_t channelCount = this->xdf->streams[this->stream_idx].info.channel_count;
    lsl::stream_info info(streamName, "EEG", channelCount, (double)samplingRate, lsl::cf_double64, "RT_Sender_SimulationPC");
    lsl::stream_outlet outlet(info);

    const double dSamplingInterval = 1.0 / samplingRate;
    std::vector<double> sample(channelCount);

    double starttime = ((double)clock()) / CLOCKS_PER_SEC;

    for (unsigned t = 0; t < xdf->streams[this->stream_idx].time_series.front().size(); t++) {
        while (((double)clock()) / CLOCKS_PER_SEC < starttime + t * dSamplingInterval);

        for (int c = 0; c < channelCount; c++) {
            sample[c] = xdf->streams[stream_idx].time_series[c][t];
        }

        outlet.push_sample(sample);
    }
}

void XdfStreamer::clearCache()
{
    this->xdf.clear();
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setText("Load");
    ui->treeWidget->header()->hide();
    ui->treeWidget->clear();
    this->stream_ready = false;
}

void XdfStreamer::enableControlPanel(bool enabled)
{
    ui->label->setEnabled(enabled);
    ui->label_2->setEnabled(enabled);
    ui->label_3->setEnabled(enabled);
    ui->label_4->setEnabled(enabled);
    ui->label_5->setEnabled(enabled);
    ui->lineEdit->setEnabled(enabled);
    ui->lineEdit_2->setEnabled(enabled);
    ui->lineEdit_3->setEnabled(enabled);
    ui->toolButton->setEnabled(enabled);
    ui->pushButton_2->setEnabled(enabled);
    ui->checkBox->setEnabled(enabled);
    ui->spinBox->setEnabled(enabled);
    ui->spinBox_2->setEnabled(enabled);
}

void XdfStreamer::on_checkBox_stateChanged(int status)
{
    if (status == Qt::Checked) {
        ui->pushButton->setEnabled(true);
        ui->label_3->show();
        ui->lineEdit_2->show();
        ui->lineEdit_2->setText("ActiChamp-0");
        ui->label_4->show();
        ui->spinBox_2->show();
        ui->label_5->show();
        ui->lineEdit_3->show();
        ui->groupBox_2->show();
        ui->treeWidget->hide();
        ui->treeWidget_2->show();
        ui->treeWidget_2->header()->show();
        ui->treeWidget_2->setColumnWidth(0, std::round(0.5 * ui->treeWidget_2->width()));

        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget_2);
        item->setText(0, "Stream-" + QString::number(1));

        QTreeWidgetItem *subItem = new QTreeWidgetItem(item);
        subItem->setText(0, "Stream Name");
        subItem->setText(1, ui->lineEdit_2->text());
        item->addChild(subItem);

        subItem = new QTreeWidgetItem(item);
        subItem->setText(0, "Channel Format");
        subItem->setText(1, "Double");
        item->addChild(subItem);

        subItem = new QTreeWidgetItem(item);
        subItem->setText(0, "Samplign Rate");
        subItem->setText(1, QString::number(ui->spinBox->value()));
        item->addChild(subItem);

        subItem = new QTreeWidgetItem(item);
        subItem->setText(0, "Channel Count");
        subItem->setText(1, QString::number(ui->spinBox_2->value()));
        item->addChild(subItem);

        subItem = new QTreeWidgetItem(item);
        subItem->setText(0, "Stream Type");
        subItem->setText(1, ui->lineEdit_3->text());
        item->addChild(subItem);

        ui->treeWidget_2->addTopLevelItem(item);
        ui->treeWidget_2->expandAll();
    }
    else {
        ui->pushButton->setEnabled(this->stream_ready ? true : false);
        ui->label_3->hide();
        ui->lineEdit_2->hide();
        ui->label_4->hide();
        ui->spinBox_2->hide();
        ui->label_5->hide();
        ui->lineEdit_3->hide();
        ui->groupBox_2->hide();
        ui->treeWidget_2->clear();
        ui->treeWidget_2->hide();
        ui->treeWidget_2->header()->hide();
        ui->treeWidget->show();
    }

    bool enabled = status == Qt::Checked ? false : true;
    ui->label->setEnabled(enabled);
    ui->lineEdit->setEnabled(enabled);
    ui->toolButton->setEnabled(enabled);
    bool loadButtonEnabled = ui->lineEdit->text().isEmpty() ? false : enabled;
    ui->pushButton_2->setEnabled(loadButtonEnabled);
}

void XdfStreamer::on_toolButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open XDF File"), "", tr("XDF Files (*.xdf)"));

    if (!fileName.isEmpty()) {
        this->clearCache();
        ui->lineEdit->setText(fileName);
        on_pushButton_2_clicked();
    }
}

void XdfStreamer::on_pushButton_2_clicked()
{
    if (ui->pushButton_2->text().compare("Load") == 0) {
        this->xdf = QSharedPointer<Xdf>(new Xdf);
        this->xdf->load_xdf(ui->lineEdit->text().toStdString());

        if (this->xdf->streams.empty()) {
            QString msg = "Unable to find " + ui->lineEdit->text() + "\nPlease check your path";
            QMessageBox::warning(this, tr("XDF Streamer"), msg, QMessageBox::Ok);
        }
        else {
            ui->pushButton->setEnabled(true);
            ui->pushButton_2->setText("Unload");
            ui->treeWidget->header()->show();
            ui->treeWidget->setColumnWidth(0, std::round(0.5 * ui->treeWidget->width()));

            /* Get first stream that is not a string stream */
            for (size_t k = 0; k < this->xdf->streams.size(); k++) {
                if (this->xdf->streams[k].info.channel_format.compare("string") != 0) {
                    this->stream_idx = k;
                    ui->spinBox->setValue((int)this->xdf->streams[k].info.nominal_srate);
                    break;
                }
            }

            for (size_t k = 0; k < this->xdf->streams.size(); k++) {
                QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
                item->setText(0, "Stream-" + QString::number(k+1));
                item->setCheckState(0, (int)k == this->stream_idx ? Qt::Checked : Qt::Unchecked);
                item->setDisabled(this->xdf->streams[k].info.channel_format.compare("string") == 0 ? true : false);

                QTreeWidgetItem *subItem = new QTreeWidgetItem(item);
                subItem->setText(0, "Stream Name");
                subItem->setText(1, QString::fromStdString(this->xdf->streams[k].info.name));
                subItem->setDisabled(this->xdf->streams[k].info.channel_format.compare("string") == 0 ? true : false);
                item->addChild(subItem);

                subItem = new QTreeWidgetItem(item);
                subItem->setText(0, "Channel Format");
                subItem->setText(1, QString::fromStdString(this->xdf->streams[k].info.channel_format));
                subItem->setDisabled(this->xdf->streams[k].info.channel_format.compare("string") == 0 ? true : false);
                item->addChild(subItem);

                subItem = new QTreeWidgetItem(item);
                subItem->setText(0, "Sampling Rate");
                subItem->setText(1, QString::number(this->xdf->streams[k].info.nominal_srate));
                subItem->setDisabled(this->xdf->streams[k].info.channel_format.compare("string") == 0 ? true : false);
                item->addChild(subItem);

                subItem = new QTreeWidgetItem(item);
                subItem->setText(0, "Channel Count");
                subItem->setText(1, QString::number(this->xdf->streams[k].info.channel_count));
                subItem->setDisabled(this->xdf->streams[k].info.channel_format.compare("string") == 0 ? true : false);
                item->addChild(subItem);

                subItem = new QTreeWidgetItem(item);
                subItem->setText(0, "Stream Type");
                subItem->setText(1, QString::fromStdString(this->xdf->streams[k].info.type));
                subItem->setDisabled(this->xdf->streams[k].info.channel_format.compare("string") == 0 ? true : false);
                item->addChild(subItem);

                ui->treeWidget->addTopLevelItem(item);
            }
            ui->treeWidget->expandAll();
            this->stream_ready = this->stream_idx != -1 ? true : false;
        }
    }
    else {
        this->clearCache();
    }
}

void XdfStreamer::on_lineEdit_textChanged(const QString &path)
{
    if (path.isEmpty()) {
        ui->pushButton_2->setEnabled(false);
    }
    else {
        ui->pushButton_2->setEnabled(true);
    }
}

void XdfStreamer::on_pushButton_clicked()
{
    if (ui->pushButton->text().compare("Stream") == 0) {
        ui->pushButton->setText("Stop");
        this->enableControlPanel(false);

        if (ui->checkBox->isChecked()) {
            qDebug() << "Generating synthetic signals";

            this->pushThread = new std::thread(&XdfStreamer::pushRandomSamples, this);
        }
        else {
            qDebug() << "Load XDF";

            this->pushThread = new std::thread(&XdfStreamer::pushXdfData, this);
        }
        ui->treeWidget->setEnabled(false);
        ui->treeWidget_2->setEnabled(false);
    }
    else {
        this->mutex_stop_thread.lock();
        this->stop_thread = true;
        this->mutex_stop_thread.unlock();

        ui->treeWidget->setEnabled(true);
        ui->treeWidget_2->setEnabled(true);
        ui->pushButton->setText("Stream");
        this->pushThread->join();
        delete this->pushThread;
        this->pushThread = nullptr;
        this->enableControlPanel(true);
    }
}

void XdfStreamer::on_treeWidget_itemClicked(QTreeWidgetItem *item)
{
    QTreeWidgetItemIterator it(ui->treeWidget);

    if (item->checkState(0) == Qt::Checked) {
        for (int k = 0; k < ui->treeWidget->topLevelItemCount(); k++) {
            if (ui->treeWidget->topLevelItem(k) == item) {
                this->stream_idx = k;
                ui->spinBox->setValue(std::round(this->xdf->streams[k].info.nominal_srate));
            }
            else {
                ui->treeWidget->topLevelItem(k)->setCheckState(0, Qt::Unchecked);
            }
        }
    }
}
