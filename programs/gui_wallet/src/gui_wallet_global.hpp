/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#ifndef STDAFX_H
#include <QEvent>
#include <QTableWidget>
#include <chrono>
#include <vector>
#include <atomic>

#include <nlohmann/json.hpp>

#include <graphene/wallet/wallet_utility.hpp>
#include <graphene/miner/miner.hpp>
#include <graphene/seeding/seeding.hpp>
#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/transaction_history/transaction_history_plugin.hpp>
#include <graphene/utilities/dirhelper.hpp>

class QTimer;
#endif

#include <numeric>
#if defined( _MSC_VER )
#include <iso646.h>
#endif

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

namespace bpo = boost::program_options;

namespace gui_wallet
{
   const int g_max_number_of_decimal_places = 8;   //number of decimal places showed in asset price

   class StackLayerWidget;
   QString CalculateRemainingTime(const QDateTime& dt, const QDateTime& dtFuture);
   QString CalculateRemainingTime_Behind(QDateTime const& dt, QDateTime const& dtFuture);

   void ShowMessageBox(QString const& strTitle,
                        QString const& strMessage,
                        QString const& strDetailedText = QString(),
                        QWidget* parent = nullptr);

   uint64_t json_to_int64(nlohmann::json const& o);

   std::size_t extra_space(const std::string& s) noexcept;
   std::string unescape_string(const std::string& s);
   std::string escape_string(const std::string& s);
   QDateTime convertStringToDateTime(const std::string& s);
   QString convertDateToLocale(const std::string& s);
   QString convertDateTimeToLocale(const std::string& s);
   QString convertDateTimeToLocale2(const std::string& s);

   class WalletOperator : public QObject
   {
      Q_OBJECT
   public:
      WalletOperator();
      ~WalletOperator();

      void cancel();
      void connect(const boost::filesystem::path &wallet_file, const graphene::wallet::server_data &ws);

   signals:
      void signal_connected(std::string const& str_error);
   public:
      graphene::wallet::WalletAPI m_wallet_api;
   private:
      std::atomic_bool m_cancellation_token;
   };

   // Asset
   //
   // use Globals.asset to get a valid one
   class Asset
   {
   public:
      Asset() = default;
      Asset(const std::string &str_symbol, uint8_t precision, uint64_t amount = 0u);
      double to_value() const;
      bool hasDecimals() const;

      QString getString() const;
      QString getBalance() const;

      uint64_t m_amount = 0;
      uint64_t m_scale = 1;
      std::string m_str_symbol;

      static const std::string dct_id;
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
      enum class ConnectionState { NoState, Reindexing, Connecting, SyncingUp, Up };
      static Globals& instance();

      using Plugins = graphene::app::plugin_set<
         graphene::miner_plugin::miner_plugin,
         graphene::account_history::account_history_plugin,
         decent::seeding::seeding_plugin,
         graphene::transaction_history::transaction_history_plugin
      >;

      static void setCommandLine(bpo::options_description &app_options, bpo::options_description &cfg_options);

      void startDaemons(BlockChainStartType type, const boost::filesystem::path &wallet_file, const graphene::wallet::server_data &ws);
      void stopDaemons(const boost::filesystem::path &wallet_file);
      fc::logger& guiLogger() { return m_logger; }
      std::string getCurrentUser() const;
      graphene::wallet::WalletAPI& getWallet() const;
      void clear();
      Asset asset(uint64_t amount, const std::string& assetId = Asset::dct_id);
      std::string runTask(std::string const& str_command);
      nlohmann::json runTaskParse(std::string const& str_command);
      std::vector<Publisher> getPublishers();
      QLocale& locale() { return *m_p_locale; }
      bool connected() const;
      void display_error_and_stop_slot_timer(std::string param1, std::string param2, std::string param3);
      void set_reindexing_percent(uint8_t progress) { m_reindexing_percent = progress; }

      //functions
      std::string ImportAccount(const std::string& name, const std::string& key);
      std::string TransferFunds(const std::string& from, const std::string to,
                                double amount, const std::string& asset_symbol, const std::string& memo);

   public slots:
      void slot_updateAccountBalance();
      void slot_showTransferDialog(const QString& user);
      void slot_showTransferDialog();

   signals:
      void signal_stackWidgetPush(gui_wallet::StackLayerWidget*);
      void signal_showPurchasedTab();
      void signal_showTransactionsTab(std::string const&);
      void signal_updateAccountAssets(const QList<Asset>& assets);
      void signal_keyImported();
      void signal_daemonFinished(int ret);

   public:
      void setWalletUnlocked();
      void setWalletError(std::string const& error);
      std::string getAccountName(const std::string& accountId);
      void setCurrentAccount(const QString& account_name);

      Asset getDCoreFees(int iOperation);

   signals:
      void signal_connect();  // for internal use
   private slots:
      void slot_connected(std::string const& str_error);
      void slot_timer();
      void slot_ConnectionStatusChange(ConnectionState from, ConnectionState to);

   signals:
      //void connectingProgress(const QString& str_progress);
      void currentUserChanged(const QString& user);
      void progressSyncMessage(const QString& str_message, int timeout = 0);
      void progressCommonTextMessage(const QString& str_message);
      void updateProgress(int value, int maxVal);
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
      fc::logger m_logger;
      std::string m_str_currentUser;
      std::chrono::steady_clock::time_point m_tp_started;
      std::chrono::system_clock::time_point m_blockStart;

      std::map<std::string, Asset> m_asset_symbols_cache;
      std::map<std::string, std::string> m_map_user_id_cache;

      std::string m_exceptionMsgBoxParam1;
      std::string m_exceptionMsgBoxParam2;
      std::string m_exceptionMsgBoxParam3;

      uint8_t m_reindexing_percent = 0;
   };

#define GUI_LOGGER gui_wallet::Globals::instance().guiLogger()
#define GUI_DLOG(FORMAT, ...) fc_dlog(GUI_LOGGER, FORMAT, __VA_ARGS__)
#define GUI_ILOG(FORMAT, ...) fc_ilog(GUI_LOGGER, FORMAT, __VA_ARGS__)
#define GUI_WLOG(FORMAT, ...) fc_wlog(GUI_LOGGER, FORMAT, __VA_ARGS__)
#define GUI_ELOG(FORMAT, ...) fc_elog(GUI_LOGGER, FORMAT, __VA_ARGS__)

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
      DecentColumn(const QString& title, int size, std::string const& sortid = std::string());

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

   bool IsIpfsRunning();

}
