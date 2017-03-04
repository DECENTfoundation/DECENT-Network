/*
 *	File: browse_content_tab.hpp
 *
 *	Created on: 11 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef BROWSE_CONTENT_TAB_H
#define BROWSE_CONTENT_TAB_H


#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <vector>
#include <string>
#include "qt_commonheader.hpp"
#include <QLineEdit>
#include <QHBoxLayout>
#include <QComboBox>
#include <QTimer>
#include "gui_wallet_tabcontentmanager.hpp"



namespace gui_wallet
{

    // DCF stands for Digital Contex Fields
namespace DCF {enum DIG_CONT_FIELDS{SYNOPSIS, RATING, SIZE, PRICE, TIME, LEFT, NUM_OF_DIG_CONT_FIELDS};}

// ST stands for search type
namespace ST{
    enum STtype{URI_start,author,content};
    static const char* s_vcpcSearchTypeStrs[] = {"URI_start","author","content"};
}



class BTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    BTableWidget(int a , int b) : QTableWidget(a,b)
    {
         this->setMouseTracking(true);
    };

    virtual void mouseMoveEvent(QMouseEvent * event);
public:
signals:
    void mouseMoveEventDid();
};







class Browse_content_tab : public TabContentManager
{
    friend class CentralWigdet;
    Q_OBJECT
public:
    Browse_content_tab();
    virtual ~Browse_content_tab();

    void ShowDigitalContentsGUI(std::vector<SDigitalContent>& contents);
    void Connects();

public:
    
    virtual void content_activated() {}
    virtual void content_deactivated() {}

    
public:
signals:
    void ShowDetailsOnDigContentSig(SDigitalContent get_cont_str);

public slots:
    void onTextChanged(const QString& text);
    void doRowColor();
    void updateContents();
    void maybeUpdateContent();

protected:
    void DigContCallback(_NEEDED_ARGS2_);
    void PrepareTableWidgetHeaderGUI();
    virtual void resizeEvent ( QResizeEvent * a_event );
    void ArrangeSize();
    
private:
    bool FilterContent(const SDigitalContent& content);

protected:
    QVBoxLayout     m_main_layout;
    QHBoxLayout     m_search_layout;

    BTableWidget*    m_pTableWidget;
    
    QLineEdit       m_filterLineEdit;
    QComboBox       m_searchTypeCombo;
    
    std::vector<SDigitalContent> m_dContents;
    bool m_doUpdate = true;
    int green_row;
    QTimer  m_contentUpdateTimer;
};


}

#endif // BROWSE_CONTENT_TAB_H
