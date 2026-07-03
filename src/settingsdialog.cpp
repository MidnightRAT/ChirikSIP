#include "settingsdialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QApplication>
#include <QAudioDevice>
#include <QMediaDevices>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumWidth(300);
    setWindowIcon(QApplication::windowIcon());

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout();
    m_serverEdit = new QLineEdit(this);
    m_serverEdit->setPlaceholderText("sip.example.com");
    form->addRow("Server:", m_serverEdit);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("username");
    form->addRow("User:", m_usernameEdit);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("password");
    form->addRow("Pass:", m_passwordEdit);

    m_portEdit = new QSpinBox(this);
    m_portEdit->setRange(0, 65535);
    m_portEdit->setSpecialValueText("Auto");
    form->addRow("Port:", m_portEdit);

    m_echoCancelCheck = new QCheckBox("Enable echo cancellation", this);
    m_echoCancelCheck->setChecked(true);
    form->addRow("", m_echoCancelCheck);

    m_echoAggressivenessCombo = new QComboBox(this);
    m_echoAggressivenessCombo->addItem("Conservative", 0);
    m_echoAggressivenessCombo->addItem("Moderate", 1);
    m_echoAggressivenessCombo->addItem("Aggressive", 2);
    m_echoAggressivenessCombo->setCurrentIndex(1);
    form->addRow("AEC level:", m_echoAggressivenessCombo);

    m_inputDeviceCombo = new QComboBox(this);
    const auto inputs = QMediaDevices::audioInputs();
    for (const QAudioDevice &dev : inputs)
        m_inputDeviceCombo->addItem(dev.description(), QVariant::fromValue(dev));
    form->addRow("Microphone:", m_inputDeviceCombo);

    m_outputDeviceCombo = new QComboBox(this);
    const auto outputs = QMediaDevices::audioOutputs();
    for (const QAudioDevice &dev : outputs)
        m_outputDeviceCombo->addItem(dev.description(), QVariant::fromValue(dev));
    form->addRow("Speakers:", m_outputDeviceCombo);

    mainLayout->addLayout(form);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

QString SettingsDialog::server() const { return m_serverEdit->text().trimmed(); }
QString SettingsDialog::username() const { return m_usernameEdit->text().trimmed(); }
QString SettingsDialog::password() const { return m_passwordEdit->text(); }
int SettingsDialog::port() const { return m_portEdit->value(); }
bool SettingsDialog::echoCancelEnabled() const { return m_echoCancelCheck->isChecked(); }
int SettingsDialog::echoAggressiveness() const { return m_echoAggressivenessCombo->currentData().toInt(); }

void SettingsDialog::setServer(const QString &s) { m_serverEdit->setText(s); }
void SettingsDialog::setUsername(const QString &u) { m_usernameEdit->setText(u); }
void SettingsDialog::setPassword(const QString &p) { m_passwordEdit->setText(p); }
void SettingsDialog::setPort(int p) { m_portEdit->setValue(p); }
void SettingsDialog::setEchoCancelEnabled(bool enabled) { m_echoCancelCheck->setChecked(enabled); }
void SettingsDialog::setEchoAggressiveness(int level) {
    int idx = m_echoAggressivenessCombo->findData(level);
    if (idx >= 0) m_echoAggressivenessCombo->setCurrentIndex(idx);
}

QAudioDevice SettingsDialog::selectedInputDevice() const {
    return m_inputDeviceCombo->currentData().value<QAudioDevice>();
}
QAudioDevice SettingsDialog::selectedOutputDevice() const {
    return m_outputDeviceCombo->currentData().value<QAudioDevice>();
}
void SettingsDialog::setInputDevice(const QAudioDevice &device) {
    for (int i = 0; i < m_inputDeviceCombo->count(); ++i) {
        if (m_inputDeviceCombo->itemData(i).value<QAudioDevice>().id() == device.id()) {
            m_inputDeviceCombo->setCurrentIndex(i);
            break;
        }
    }
}
void SettingsDialog::setOutputDevice(const QAudioDevice &device) {
    for (int i = 0; i < m_outputDeviceCombo->count(); ++i) {
        if (m_outputDeviceCombo->itemData(i).value<QAudioDevice>().id() == device.id()) {
            m_outputDeviceCombo->setCurrentIndex(i);
            break;
        }
    }
}
