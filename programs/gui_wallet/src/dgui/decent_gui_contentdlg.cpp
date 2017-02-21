/*
 *	File: decent_gui_contentdlg.cpp
 *
 *	Created on: 10 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  In this file there is a defination
 *  for the class decent::gui::ContentDlg
 *  This class is used by GUI to show details on digital content
 *
 */


#include "decent_gui_contentdlg.hpp"
#include <QPushButton>
#include <QHeaderView>

decent::gui::ContentDlg::ContentDlg()
    :
      m_main_table(s_cnContentNumberOfFields,2)
{

    for(int i(0); i<s_cnNumberOfLabels; ++i)
    {
        m_main_table.setItem(i,0,new QTableWidgetItem(tr(s_cpcContentFieldNames[i])));
        m_main_table.setItem(i,1,new QTableWidgetItem(tr("")));
    }
    m_main_table.setCellWidget(s_cnNumberOfLabels,0,new QPushButton(tr("show details")));
    m_main_table.setCellWidget(s_cnNumberOfLabels,1,new QPushButton(tr("OK")));


    //QPalette aPalette = m_main_table.cellWidget(s_cnNumberOfLabels,0)->palette();
    //aPalette.setColor(QPalette::Base,Qt::gray);
    //m_main_table.cellWidget(CONNECT_BUTTON_FIELD,0)->setPalette(aPalette);

    m_main_table.horizontalHeader()->hide();
    m_main_table.verticalHeader()->hide();
    m_main_table.resize(size());
    m_main_layout.addWidget(&m_main_table);
    setLayout(&m_main_layout);
}

decent::gui::ContentDlg::~ContentDlg()
{
    //
}

//ContentDlg();
//virtual ~ContentDlg()
