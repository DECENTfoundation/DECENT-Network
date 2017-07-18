/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <QObject>
#include <QMouseEvent>
#include <QEvent>
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <chrono>
#include <vector>

#include <decent/wallet_utility/wallet_utility.hpp>

#include "json.hpp"

#include <numeric>
#if defined( _MSC_VER )
#include <iso646.h>
#endif

//#define UPDATE_MANAGER

#define ALERT(message)                                  \
{                                                       \
QMessageBox* msgBox = new QMessageBox();                \
QIcon i = msgBox->style()->standardIcon(QStyle::SP_MessageBoxCritical);\
msgBox->setWindowIcon(i);\
msgBox->setWindowTitle("Error");                     \
msgBox->setText(QString::fromStdString(message));    \
msgBox->exec();                                      \
msgBox->close();                                      \
delete msgBox;                                      \
}


#define ALERT_DETAILS(message, details)                                  \
{                                                       \
QMessageBox* msgBox = new QMessageBox();                \
QIcon i = msgBox->style()->standardIcon(QStyle::SP_MessageBoxCritical);\
msgBox->setWindowIcon(i);\
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
QIcon i = msgBox->style()->standardIcon(QStyle::SP_MessageBoxInformation);\
msgBox->setWindowIcon(i);\
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

class QEvent;
class QTimer;

namespace gui_wallet
{
   class StackLayerWidget;
   std::string CalculateRemainingTime(QDateTime const& dt, QDateTime const& dtFuture);
   QString CalculateRemainingTime_Behind(QDateTime const& dt, QDateTime const& dtFuture);
   
   void ShowMessageBox(QString const& strTitle,
                        QString const& strMessage,
                        QString const& strDetailedText = QString());

   uint64_t json_to_int64(nlohmann::json const& o);
    
   std::size_t extra_space(const std::string& s) noexcept;
   std::string unescape_string(const std::string& s);
   std::string escape_string(const std::string& s);

   using WalletAPI = decent::wallet_utility::WalletAPI;

   class WalletOperator : public QObject
   {
      Q_OBJECT
   public:
      WalletOperator();
      ~WalletOperator();

   public slots:
      void slot_connect();
   signals:
      void signal_connected(std::string const& str_error);
   public:
      WalletAPI m_wallet_api;
   };   
   // Asset
   //
   // use Globals.asset to get a valid one
   class Asset
   {
   public:
      operator double() const;
      operator std::string() const;
      std::string getString() const;
      std::string getStringBalance() const;
      uint64_t m_amount = 0;
      uint64_t m_scale = 1;
      std::string m_str_symbol;
   };
   //
   // Publisher
   //
   class Publisher
   {
   public:
      double m_storage_size = 0;
      std::string m_str_name;
      Asset m_price;
   };
   //
   enum class BlockChainStartType { Simple, Replay, Resync };
   //
   // Globals
   //
   class Globals : public QObject
   {
      Q_OBJECT

   private:
      Globals();
      Globals(Globals const&) = delete;
      Globals(Globals&&) = delete;
      ~Globals();
      
   public:
      enum class ConnectionState { Connecting, SyncingUp, Up };
      static Globals& instance();

      void startDaemons(BlockChainStartType type);
      void stopDaemons();
      std::string getCurrentUser() const;
      WalletAPI& getWallet() const;
      void clear();
      Asset asset(uint64_t amount, const std::string& symbol = std::string());
      std::string runTask(std::string const& str_command);
      nlohmann::json runTaskParse(std::string const& str_command);
      std::vector<Publisher> getPublishers();
      QLocale& locale();
      bool connected() const;

   public slots:
      void slot_updateAccountBalance();
      void slot_setCurrentUser(QString const& user);
      void slot_showTransferDialog(QString const& user);
      void slot_showTransferDialog();

   signals:
      void signal_stackWidgetPush(gui_wallet::StackLayerWidget*);
      void signal_showPurchasedTab();
      void signal_showTransactionsTab(std::string const&);
      void signal_updateAccountBalance(Asset const&);
      void signal_keyImported();

   public:
      void setWalletUnlocked();
      void setWalletError(std::string const& error);

      std::string getAccountName(std::string const& accountId);
   signals:
      void signal_connect();  // for internal use
   private slots:
      void slot_connected(std::string const& str_error);
      void slot_timer();

   signals:
      void connectingProgress(std::string const& str_progress);
      void currentUserChanged(QString const& user);
      void statusShowMessage(QString const& str_message, int timeout = 0);
      void statusClearMessage();
      void walletUnlocked();
      
      void walletConnectionError(std::string const& message);
      void walletConnectionStatusChanged(ConnectionState from, ConnectionState to);

   private:
      ConnectionState m_connected_state;
      WalletOperator* m_p_wallet_operator;
      QThread* m_p_wallet_operator_thread;
      QTimer* m_p_timer;
      QLocale* m_p_locale;
      class DaemonDetails* m_p_daemon_details;
      std::string m_str_currentUser;
      std::chrono::steady_clock::time_point m_tp_started;
      std::map<std::string, std::string> m_map_user_id_cache;
   };

   
   //
   // EventPassthrough
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
      
      virtual bool event(QEvent *event) override{
         if (event->type() == QEvent::MouseMove)
            return false;
         else
            return QWidget::event(event);
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
      DecentColumn(QString title, int size, std::string const& sortid = std::string());
      
      QString title;
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
      void setRowCount(int rows);

   signals:
      void signal_SortingChanged(int);
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



   // DCT stands for Digital Contex Actions
   namespace DCT {
      enum DIG_CONT_TYPES {GENERAL, BOUGHT, WAITING_DELIVERY};
   }


   struct SDigitalContent
   {
      DCT::DIG_CONT_TYPES  type = DCT::GENERAL;
      std::string          author;

      Asset price;

      std::string   synopsis;
      std::string   URI;
      double        AVG_rating;
      std::string   created;
      std::string   purchased_time;
      std::string   expiration;
      std::string   id;
      std::string   hash;
      std::string   status;
      int           size;
      int           times_bought;
   };

   qreal scale();

   QFont AccountBalanceFont();
   QFont DescriptionDetailsFont();
   QFont PopupButtonRegularFont();
   QFont PopupButtonBigFont();
   QFont TabButtonFont();
   QFont PaginationFont();
   QFont ProgressInfoFont();
   QFont MainFont();

}

