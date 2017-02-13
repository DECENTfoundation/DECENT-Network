/*
 *	File: decent_gui_contentdlg.hpp
 *
 *	Created on: 10 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  In this file there is a defination
 *  for the class decent::gui::ContentDlg
 *  This class is used by GUI to show details on digital content
 *
 */
#ifndef DECENT_GUI_CONTENTDLG_HPP
#define DECENT_GUI_CONTENTDLG_HPP

#include <QDialog>
#include <QHBoxLayout>
#include <QTableWidget>

namespace decent{ namespace gui{

static const char* s_cpcContentFieldNames[] = {
    "author", "price", "synopsis", "URI", "AVG_rating","details_and_ok_buttons"
};

static const int s_cnContentNumberOfFields = sizeof(s_cpcContentFieldNames) / sizeof(const char*);
static const int s_cnNumberOfLabels(s_cnContentNumberOfFields-1);

class ContentDlg : public QDialog
{
public:
    ContentDlg();
    virtual ~ContentDlg();

    QWidget* GettableWidget(int col, int row);

protected:
    QHBoxLayout                     m_main_layout;
    QTableWidget                    m_main_table;
};

}}

#endif // DECENT_GUI_CONTENTDLG_HPP
