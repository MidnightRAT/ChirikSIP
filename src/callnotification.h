#ifndef CALLNOTIFICATION_H
#define CALLNOTIFICATION_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>

class SipClient;
class ScrollHelper;

class CallNotification : public QDialog
{
    Q_OBJECT
public:
    explicit CallNotification(SipClient *client, QWidget *parent = nullptr);

    void showNotification(const QString &number, const QString &name);
    void hideNotification();

signals:
    void accepted();
    void rejected();

private slots:
    void onAnswer();
    void onReject();

private:
    SipClient *m_client;
    QLabel *m_numberLabel;
    QLabel *m_nameLabel;
    QPushButton *m_answerBtn;
    QPushButton *m_rejectBtn;
    ScrollHelper *m_scrollHelper;
};

#endif // CALLNOTIFICATION_H
