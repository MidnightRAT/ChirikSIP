#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <QAudioDevice>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    QString server() const;
    QString username() const;
    QString password() const;
    int port() const;
    bool echoCancelEnabled() const;
    int echoAggressiveness() const;
    QAudioDevice selectedInputDevice() const;
    QAudioDevice selectedOutputDevice() const;

    void setServer(const QString &s);
    void setUsername(const QString &u);
    void setPassword(const QString &p);
    void setPort(int p);
    void setEchoCancelEnabled(bool enabled);
    void setEchoAggressiveness(int level);
    void setInputDevice(const QAudioDevice &device);
    void setOutputDevice(const QAudioDevice &device);

private:
    QLineEdit *m_serverEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QSpinBox *m_portEdit;
    QCheckBox *m_echoCancelCheck;
    QComboBox *m_echoAggressivenessCombo;
    QComboBox *m_inputDeviceCombo;
    QComboBox *m_outputDeviceCombo;
};

#endif // SETTINGSDIALOG_H
