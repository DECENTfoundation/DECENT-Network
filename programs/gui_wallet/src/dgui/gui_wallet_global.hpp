#pragma once

#include <QObject>
#include <QDateTime>
#include <QDate>
#include <QEvent>
#include <QTime>
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QMouseEvent>
#include <QHeaderView>
#include <iostream>
#include "gui_design.hpp"

#include <numeric>
#if defined( _MSC_VER )
#include <iso646.h>
#endif


#define ALERT(message)                                  \
{                                                       \
QMessageBox* msgBox = new QMessageBox();                \
msgBox->setWindowTitle("Error");                     \
msgBox->setText(QString::fromStdString(message));    \
msgBox->exec();                                      \
msgBox->close();                                      \
delete msgBox;                                      \
}



#define ALERT_DETAILS(message, details)                                  \
{                                                       \
QMessageBox* msgBox = new QMessageBox();                \
msgBox->setWindowTitle("Error");                     \
msgBox->setText(QString::fromStdString(message));    \
msgBox->setDetailedText(QObject::tr(details));    \
msgBox->exec();                                      \
msgBox->close();                                      \
delete msgBox;                                      \
}                                                       \





#define MESSAGE(message)                                  \
{                                                       \
QMessageBox* msgBox = new QMessageBox();                     \
msgBox->setWindowTitle("Message");                     \
msgBox->setText(QString::fromStdString(message));    \
msgBox->exec();                                      \
msgBox->close();                                      \
delete msgBox;                                      \
}                                                   \


#define DCT_VERIFY(condition) \
{ \
   bool _b_condition_ = (condition); \
   Q_ASSERT(_b_condition_); \
}

namespace gui_wallet
{
   std::string CalculateRemainingTime(QDateTime const& dt, QDateTime const& dtFuture);
   void ShowMessageBox(QString const& strTitle,
                       QString const& strMessage,
                       QString const& strDetailedText = QString());
    
    
   std::size_t extra_space(const std::string& s) noexcept;
   std::string unescape_string(const std::string& s);
   std::string escape_string(const std::string& s);

   //
   // GlobalEvents
   //
   class GlobalEvents : public QObject {
      Q_OBJECT
   private:
      
      std::string 	_currentUser;
      
   private:
      GlobalEvents() { }
      GlobalEvents(const GlobalEvents& other) { }
      
   public:
      static GlobalEvents& instance() {
         static GlobalEvents theOne;
         return theOne;
      }

      std::string getCurrentUser() const { return _currentUser; }
      void setCurrentUser(const std::string& user) { _currentUser = user; emit currentUserChanged(_currentUser); }
      
      void setWalletUnlocked() { emit walletUnlocked(); }
      
      void setWalletConnected(bool isNew) { emit walletConnected(isNew); }
      void setWalletError(std::string error) { emit walletConnectionError(error); }
      
   signals:
      void currentUserChanged(std::string user);
      void walletUnlocked();
      
      void walletConnectionError(std::string message);
      void walletConnected(bool isNew);
   };

   
   //
   // EventPassthtough
   //
   // This is helper class that allows to passthrough mousemove events to parent widget.
   // Useful when highlighting rows in tableview
   template<class QTType>
   class EventPassthrough : public QTType {
      
   public:
      template<class... Args>
      EventPassthrough(const Args&... args) : QTType(args...) {
         this->setMouseTracking(true);
      }
      
      bool event(QEvent *event){
         if (event->type() == QEvent::MouseMove)
            return false;
         else
            return QWidget::event(event);
      }
   };

   //
   // DecentSmallButton
   //
   // QLabel with clicked() signal implemented
   
   class DecentSmallButton : public QLabel
   {
      Q_OBJECT;
   public:
      DecentSmallButton(const QString& normalImg, const QString& highlightedImg );

      void unhighlight();
      void highlight();

   signals:
      void clicked();

   protected:
      void mousePressEvent(QMouseEvent* event);
   private:
      QPixmap normalImage;
      QPixmap highlightedImage;
   };

   class ClickableLabel : public QLabel {
      Q_OBJECT;
   public:
      template<class... Args>
      ClickableLabel(const Args&... args) : QLabel(args...) {}


   signals:
      void clicked();

   protected:
      void mousePressEvent(QMouseEvent* event) {
         emit clicked();
      }
   };

   //**
   // DecentTable and DecentColumn
   //**
   // Table with additional functionality to use in our GUI
   //
   // DecentColumn
   //

   struct DecentColumn
   {
      DecentColumn(std::string title, int size, std::string const& sortid = std::string());
      
      std::string title;
      int size; // Negative value of size means absolute value of width, positive is weighted value
      std::string sortid; // "+author" means sort by author ascending "-author" descending
   };

   // DecentTable
   //
   class DecentTable : public QTableWidget
   {
      Q_OBJECT
      
   public:
      DecentTable(QWidget* pParent);
      int getCurrentHighlightedRow() const;
      std::string getSortedColumn() const;
      void set_columns(const std::vector<DecentColumn>& cols);
      
   private slots:
      void sectionClicked(int index);
      
   private:
      virtual void resizeEvent(QResizeEvent * a_event);
      virtual void mouseMoveEvent(QMouseEvent * event);
     
   private:
      int                            _current_highlighted_row = -1;
      int                            _sum_weights = 1;
      int                            _sum_absoulte = 0;
      std::vector<DecentColumn>      _cols;
      int                            _current_sort_index = -1;
      bool                           _is_ascending = true;
   };




}

