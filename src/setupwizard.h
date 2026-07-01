#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H

#include <QDialog>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

class SetupWizard : public QDialog
{
    Q_OBJECT
public:
    explicit SetupWizard(QWidget *parent = nullptr);

    QString server() const;
    QString username() const;
    QString password() const;
    int port() const;

    static bool isFirstRun();

private slots:
    void onNext();
    void onBack();

private:
    void setupPages();

    QStackedWidget *m_stack;
    QPushButton *m_nextBtn;
    QPushButton *m_backBtn;

    QLineEdit *m_serverEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QSpinBox *m_portEdit;

    int m_currentPage = 0;
    int m_pageCount = 0;
};

#endif // SETUPWIZARD_H
