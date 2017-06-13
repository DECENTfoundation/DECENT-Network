#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "richdialog.hpp"

#ifdef SEPARATE_DECENT_DAEMON
#include <thread>
#include <chrono>
#endif

#ifndef _MSC_VER
#include <QMessageBox>
#include <QThread>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QTimer>
#include <QHeaderView>
#include <QObject>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLocale>
#include <QApplication>

// used for running daemons
//
#include <graphene/witness/witness.hpp>
#include <graphene/seeding/seeding.hpp>
#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/utilities/dirhelper.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/filesystem.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/thread/thread.hpp>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <QProcess>
#include <QDir>
//
// //
#endif

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include <signal.h>

int runDecentD(bool replay_blockchain, fc::promise<void>::ptr& exit_promise);
QProcess* run_ipfs_daemon(QObject* parent, QString app_dir);

using  std::string;

namespace gui_wallet
{
   
void ShowMessageBox(QString const& strTitle,
                    QString const& strMessage,
                    QString const& strDetailedText/* = QString()*/)
{
   QMessageBox* pMessageBox = new QMessageBox(nullptr);
   pMessageBox->setWindowTitle(strTitle); // funny MacOS ignores the title
   pMessageBox->setText(strMessage);
   pMessageBox->setDetailedText(strDetailedText);
   pMessageBox->setAttribute(Qt::WA_DeleteOnClose);
   pMessageBox->open();
   // alternatively can connect to delete later as below
   //pMessageBox->open(pMessageBox, SLOT(deleteLater()));
#ifdef _MSC_VER
   int height = pMessageBox->style()->pixelMetric(QStyle::PM_TitleBarHeight);
   pMessageBox->setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
      : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

uint64_t json_to_int64(nlohmann::json const& o)
{
   if (o.is_number())
      return o.get<uint64_t>();
   else
      return std::stoll(o.get<string>());
}

struct CalendarDuration
{
   enum esign {sign_positive, sign_negative} sign = sign_positive;
   uint32_t seconds = 0;
   uint32_t minutes = 0;
   uint32_t hours = 0;
   uint32_t days = 0;
   uint32_t months = 0;
   uint32_t years = 0;
};

static CalendarDuration CalculateCalendarDuration(QDateTime const& dt, QDateTime const& dtFuture)
{
   if (dtFuture == dt)
      return CalendarDuration();
   else if (dtFuture < dt)
   {
      CalendarDuration duration = CalculateCalendarDuration(dtFuture, dt);
      duration.sign = CalendarDuration::sign_negative;
      return duration;
   }
   else
   {
      QDateTime dt_scan = dt;

      int iMonthsDiff = 0, iMonthsStep = 12*12;
      while (true)
      {
         QDateTime dt_temp = dt_scan.addMonths(iMonthsStep);
         if (dt_temp > dtFuture &&
             1 == iMonthsStep)
            break;
         else if (dt_temp > dtFuture)
            iMonthsStep /= 12;
         else
         {
            dt_scan = dt_temp;
            iMonthsDiff += iMonthsStep;
         }
      }

      int iYearsDiff = iMonthsDiff / 12;
      iMonthsDiff %= 12;

      // 60*24 minutes (a day) = 12*5*12*2 minutes = 12*12*10
      // so the step here is almost a day
      int iMinutesDiff = 0, iMinutesStep = 12 * 12 * 12;
      while (true)
      {
         QDateTime dt_temp = dt_scan.addSecs(60 * iMinutesStep);
         if (dt_temp > dtFuture &&
             1 == iMinutesStep)
            break;
         else if (dt_temp > dtFuture)
            iMinutesStep /= 12;
         else
         {
            dt_scan = dt_temp;
            iMinutesDiff += iMinutesStep;
         }
      }

      int iHoursDiff = iMinutesDiff / 60;
      iMinutesDiff %= 60;

      int iDaysDiff = iHoursDiff / 24;
      iHoursDiff %= 24;

      CalendarDuration result;
      result.years = iYearsDiff;
      result.months = iMonthsDiff;
      result.days = iDaysDiff;
      result.hours = iHoursDiff;
      result.minutes = iMinutesDiff;
      result.seconds = dt_scan.secsTo(dtFuture);

      return result;
   }
}

std::string CalculateRemainingTime(QDateTime const& dt, QDateTime const& dtFuture)
{
   CalendarDuration duration = CalculateCalendarDuration(dt, dtFuture);
   if (duration.sign == CalendarDuration::sign_negative)
      return "expired";
   else
   {
      std::vector<std::string> arrParts;

      if (duration.years)
         arrParts.push_back(std::to_string(duration.years) + " y");
      if (duration.months)
         arrParts.push_back(std::to_string(duration.months) + " m");
      if (duration.days)
         arrParts.push_back(std::to_string(duration.days) + " d");
      if (duration.hours)
         arrParts.push_back(std::to_string(duration.hours) + " h");
      if (duration.minutes)
         arrParts.push_back(std::to_string(duration.minutes) + " min");

      std::string str_result;
      if (arrParts.empty())
         str_result = "expiring in a minute";
      else
      {
         str_result = arrParts.front();
         
         if (arrParts.size() > 1)
            str_result += " " + arrParts[1];
      }
      
      return str_result;
   }
}

QString CalculateRemainingTime_Behind(QDateTime const& dt, QDateTime const& dtFuture)
{
   CalendarDuration duration = CalculateCalendarDuration(dt, dtFuture);
   if (duration.sign == CalendarDuration::sign_negative)
      return QString();
   else
   {
      QString str_result;
      std::vector<QString> arrParts;

      if(duration.years)
         arrParts.push_back(QObject::tr("%n year(s)", "", duration.years));
      if(duration.months)
         arrParts.push_back(QObject::tr("%n month(s)", "", duration.months));
      if(duration.days)
         arrParts.push_back(QObject::tr("%n day(s)", "", duration.days));
      if(duration.hours)
         arrParts.push_back(QObject::tr("%n hour(s)", "", duration.hours));
      if(duration.minutes)
         arrParts.push_back(QObject::tr("%n minute(s)", "", duration.minutes));
      if(duration.seconds)
         arrParts.push_back(QObject::tr("%n second(s)", "", duration.seconds));

      if (arrParts.empty())
         return QString();
      else if (arrParts.size() == 1 &&
               duration.seconds < 30)
         return QString();
      else
      {
         //str_result += QObject::tr("syncing up with blockchain: ");
         str_result += arrParts[0];

         if (arrParts.size() > 1)
            str_result += " " + QObject::tr("and") + " " + arrParts[1];

         str_result += " " + QObject::tr("to go");
      }

      return str_result;
   }
}

std::size_t extra_space(const std::string& s) noexcept
{
   return std::accumulate(s.begin(), s.end(), size_t{},
                          [](size_t res, typename std::string::value_type c)
                          {
                             switch (c)
                             {
                                case '"':
                                case '\\':
                                case '\b':
                                case '\f':
                                case '\n':
                                case '\r':
                                case '\t':
                                {
                                   // from c (1 byte) to \x (2 bytes)
                                   return res + 1;
                                }

                                default:
                                {
                                   if (c >= 0x00 and c <= 0x1f)
                                   {
                                      // from c (1 byte) to \uxxxx (6 bytes)
                                      return res + 5;
                                   }

                                   return res;
                                }
                             }
                          });
}
std::string unescape_string(const std::string& s)
{
   std::string result;
   bool is_escape = false;

   for (const auto& c : s)
   {
      if (!is_escape && (c != '\\')) {
         result += c;
         continue;
      }

      if (!is_escape && (c == '\\')) {
         is_escape = true;
         continue;
      }

      is_escape = false;

      switch (c)
      {
         case '"':
         {
            result += '"';
            break;
         }

         case '\\':
         {
            result += '\\';
            break;
         }

            // backspace (0x08)
         case 'b':
         {
            result += '\b';
            break;
         }

            // formfeed (0x0c)
         case 'f':
         {
            result += '\f';
            break;
         }

            // newline (0x0a)
         case 'n':
         {
            result += '\n';
            break;
         }

            // carriage return (0x0d)
         case 'r':
         {
            result += '\r';
            break;
         }

            // horizontal tab (0x09)
         case 't':
         {
            result += '\t';
            break;
         }

      }
   }

   return result;

}

std::string escape_string(const std::string& s)
{
   const auto space = extra_space(s);
   if (space == 0)
   {
      return s;
   }

   // create a result string of necessary size
   std::string result(s.size() + space, '\\');
   std::size_t pos = 0;

   for (const auto& c : s)
   {
      switch (c)
      {
            // quotation mark (0x22)
         case '"':
         {
            result[pos + 1] = '"';
            pos += 2;
            break;
         }

            // reverse solidus (0x5c)
         case '\\':
         {
            // nothing to change
            pos += 2;
            break;
         }

            // backspace (0x08)
         case '\b':
         {
            result[pos + 1] = 'b';
            pos += 2;
            break;
         }

            // formfeed (0x0c)
         case '\f':
         {
            result[pos + 1] = 'f';
            pos += 2;
            break;
         }

            // newline (0x0a)
         case '\n':
         {
            result[pos + 1] = 'n';
            pos += 2;
            break;
         }

            // carriage return (0x0d)
         case '\r':
         {
            result[pos + 1] = 'r';
            pos += 2;
            break;
         }

            // horizontal tab (0x09)
         case '\t':
         {
            result[pos + 1] = 't';
            pos += 2;
            break;
         }

         default:
         {
            if (c >= 0x00 and c <= 0x1f)
            {
               // convert a number 0..15 to its hex representation
               // (0..f)
               static const char hexify[16] =
               {
                  '0', '1', '2', '3', '4', '5', '6', '7',
                  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
               };

               // print character c as \uxxxx
               for (const char m :
                    { 'u', '0', '0', hexify[c >> 4], hexify[c & 0x0f]
                    })
               {
                  result[++pos] = m;
               }

               ++pos;
            }
            else
            {
               // all other characters are added as-is
               result[pos++] = c;
            }
            break;
         }
      }
   }

   return result;
}

//
// WalletOperator
//
WalletOperator::WalletOperator()
: QObject(nullptr)
, m_wallet_api()
{

}

WalletOperator::~WalletOperator()
{

}

void WalletOperator::slot_connect()
{
   std::string str_error;
   try
   {
      m_wallet_api.Connent();
   }
   catch(std::exception const& ex)
   {
      str_error = ex.what();
   }
#ifdef SEPARATE_DECENT_DAEMON
   std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
   emit signal_connected(str_error);
}
//
// Asset
//
Asset::operator double() const
{
   uint64_t amount = m_amount / m_scale;
   double tail = double(m_amount % m_scale) / m_scale;
   return amount + tail;
}

Asset::operator string() const
{
   return std::to_string(double(*this));
}

string Asset::getString() const
{
   if (m_amount)
      return QString::number(double(*this), 'f' , 4).toStdString() + " " + m_str_symbol;
   else
      return "Free";
}
//
// DaemonDetails
//
class DaemonDetails
{
public:
   DaemonDetails()
   : ipfs_process(nullptr)
   , exit_promise(new fc::promise<void>("Decent Daemon Exit Promise"))
   , thread_decentd("decentd service")
   , future_decentd()
   {}
   ~DaemonDetails()
   {
      if (ipfs_process)
         delete ipfs_process;
   }

   QProcess* ipfs_process;
   fc::promise<void>::ptr exit_promise;
   fc::thread thread_decentd;
   fc::future<int> future_decentd;
};
//
// Globals
//
Globals::Globals()
: m_connected_state(ConnectionState::Connecting)
, m_p_wallet_operator(nullptr)
, m_p_wallet_operator_thread(new QThread(this))
, m_p_timer(new QTimer(this))
, m_p_locale(new QLocale())
, m_p_daemon_details(nullptr)
, m_str_currentUser()
, m_tp_started(std::chrono::steady_clock::now())
{
   m_p_wallet_operator_thread->start();

   m_p_timer->start(1000);
   QObject::connect(m_p_timer, &QTimer::timeout,
                    this, &Globals::slot_timer);

   *m_p_locale = ((QApplication*)QApplication::instance())->inputMethod()->locale();
}

Globals::~Globals()
{
   // make sure the cleanup happens before the destructor to avoid segfault
   clear();
}

Globals& Globals::instance()
{
   static Globals theOne;
   return theOne;
}

void Globals::startDaemons(bool replay_blockchain)
{
   if (m_p_daemon_details ||
       m_p_wallet_operator)
      return;

   m_p_daemon_details = new DaemonDetails();

   m_p_wallet_operator = new WalletOperator();
   m_p_wallet_operator->moveToThread(m_p_wallet_operator_thread);
   QObject::connect(this, &Globals::signal_connect,
                    m_p_wallet_operator, &WalletOperator::slot_connect);
   QObject::connect(m_p_wallet_operator, &WalletOperator::signal_connected,
                    this, &Globals::slot_connected);

   fc::thread& thread_decentd = m_p_daemon_details->thread_decentd;

   fc::promise<void>::ptr& exit_promise = m_p_daemon_details->exit_promise;

   QProcess* daemon_process = nullptr;
   daemon_process = run_ipfs_daemon(qApp, qApp->applicationDirPath());
   m_p_daemon_details->ipfs_process = daemon_process;

   fc::set_signal_handler([this](int /*signal*/) {
      elog( "Caught SIGINT attempting to exit cleanly" );
      stopDaemons();
   }, SIGINT);

   fc::set_signal_handler([this](int /*signal*/) {
      elog( "Caught SIGTERM attempting to exit cleanly" );
      stopDaemons();
   }, SIGTERM);


#if !defined( _MSC_VER )

   fc::set_signal_handler([this](int /*signal*/) {
      elog( "Caught SIGHUP attempting to exit cleanly" );
      stopDaemons();
   }, SIGHUP);

#endif

#ifndef SEPARATE_DECENT_DAEMON
   m_p_daemon_details->future_decentd = thread_decentd.async([replay_blockchain, &exit_promise]() -> int
                                                            {
                                                               return ::runDecentD(replay_blockchain, exit_promise);
                                                            });
#endif

   m_tp_started = std::chrono::steady_clock::now();
   emit signal_connect();
}
   
void Globals::stopDaemons()
{
   auto backup_state = m_connected_state;
   m_connected_state = ConnectionState::Connecting;
   emit walletConnectionStatusChanged(backup_state, m_connected_state);

   if (m_p_wallet_operator)
   {
      m_p_wallet_operator->m_wallet_api.SaveWalletFile();
      delete m_p_wallet_operator;
      m_p_wallet_operator = nullptr;
   }

   fc::promise<void>::ptr& exit_promise = m_p_daemon_details->exit_promise;
   exit_promise->set_value();

#ifndef SEPARATE_DECENT_DAEMON
   m_p_daemon_details->future_decentd.wait();
#endif

   if (m_p_daemon_details->ipfs_process)
      m_p_daemon_details->ipfs_process->terminate();
   delete m_p_daemon_details->ipfs_process;
   m_p_daemon_details->ipfs_process = nullptr;

   if (m_p_daemon_details)
   {
      delete m_p_daemon_details;
      m_p_daemon_details = nullptr;
   }
}

std::string Globals::getCurrentUser() const
{
   return m_str_currentUser;
}

WalletAPI& Globals::getWallet() const
{
   return m_p_wallet_operator->m_wallet_api;
}

void Globals::clear()
{
   if (m_p_wallet_operator_thread)
   {
      m_p_wallet_operator_thread->quit();
      m_p_wallet_operator_thread->wait();
      delete m_p_wallet_operator_thread;
      m_p_wallet_operator_thread = nullptr;
   }

   if (m_p_wallet_operator)
   {
      m_p_wallet_operator->m_wallet_api.SaveWalletFile();
      delete m_p_wallet_operator;
      m_p_wallet_operator = nullptr;
   }

   if (m_p_locale)
   {
      delete m_p_locale;
      m_p_locale = nullptr;
   }

   if (m_p_daemon_details)
   {
      delete m_p_daemon_details;
      m_p_daemon_details = nullptr;
   }
}

Asset Globals::asset(uint64_t amount, const std::string& symbol_id )
{
   Asset ast_amount;
   uint8_t precision = 0;

   graphene::chain::asset_id_type asset_id;
   fc::from_variant( symbol_id, asset_id );

   getWallet().LoadAssetInfo(ast_amount.m_str_symbol, precision, asset_id);
   ast_amount.m_scale = pow(10, precision);
   ast_amount.m_amount = amount;

   return ast_amount;
}

void Globals::updateAccountBalance()
{
   if (false == m_str_currentUser.empty())
   {
      nlohmann::json allBalances = runTaskParse("list_account_balances " + m_str_currentUser);

      if (allBalances.empty())
         emit signal_updateAccountBalance(asset(0));
      else if (allBalances.size() != 1)
         throw std::runtime_error("an account cannot have more than one balance");
      else
      {
         auto const& json_balance = allBalances[0]["amount"];

         Asset ast_balance = asset(0);

         if (json_balance.is_number())
            ast_balance.m_amount = json_balance.get<uint64_t>();
         else
            ast_balance.m_amount = std::stoll(json_balance.get<string>());

         emit signal_updateAccountBalance(ast_balance);
      }
   }
}

string Globals::runTask(string const& str_command)
{
   return getWallet().RunTask(str_command);
}

nlohmann::json Globals::runTaskParse(string const& str_command)
{
   string str_result = runTask(str_command);
   return nlohmann::json::parse(str_result);
}

std::vector<Publisher> Globals::getPublishers()
{
   auto publishers = runTaskParse("list_publishers_by_price 100");
   std::vector<Publisher> result;

   for (int iIndex = 0; iIndex < publishers.size(); ++iIndex)
   {
      result.push_back(Publisher());
      Publisher& publisher = result.back();

      publisher.m_str_name = publishers[iIndex]["seeder"].get<std::string>();
      uint64_t iPrice = json_to_int64(publishers[iIndex]["price"]["amount"]);
      publisher.m_price = asset(iPrice);
      publisher.m_storage_size = publishers[iIndex]["free_space"].get<int>();
   }

   return result;
}

QLocale& Globals::locale()
{
   return *m_p_locale;
}

bool Globals::connected() const
{
   return m_connected_state != ConnectionState::Connecting;
}

void Globals::setCurrentUser(std::string const& user)
{
   m_str_currentUser = user;
   emit currentUserChanged(m_str_currentUser.c_str());
}

void Globals::setWalletUnlocked()
{
   emit walletUnlocked();
}

void Globals::setWalletError(std::string const& error)
{
   emit walletConnectionError(error);
}

void Globals::showTransferDialog(std::string const& user)
{
   if(getCurrentUser().empty())
      return;
   
   TransferWidget* pTransferDialog = new TransferWidget(nullptr , QString::fromStdString(user));
   signal_stackWidgetPush(pTransferDialog);
}

string Globals::getAccountName(string const& accountId)
{
   auto search = m_map_user_id_cache.find(accountId);
   if (search == m_map_user_id_cache.end())
   {
      string accountName = "Unknown";
      nlohmann::json accountInfoParsed = runTaskParse("get_object \"" + accountId + "\"");

      if (false == accountInfoParsed.empty())
         accountName = accountInfoParsed[0]["name"].get<std::string>();

      auto pair_value = m_map_user_id_cache.insert(std::make_pair(accountId, accountName));
      search = pair_value.first;
   }

   return search->second;
}

void Globals::slot_connected(std::string const& str_error)
{
   m_connected_state = ConnectionState::SyncingUp;
   if (str_error.empty())
      emit walletConnectionStatusChanged(ConnectionState::Connecting, ConnectionState::SyncingUp);
   else
      emit walletConnectionError(str_error);
}

void Globals::slot_timer()
{
   auto duration = std::chrono::steady_clock::now() - m_tp_started;

   auto backup_state = m_connected_state;

   if (ConnectionState::Connecting == m_connected_state)
   {
      if (duration > std::chrono::seconds(40))
         emit connectingProgress(tr("still connecting").toStdString());
      else if (duration > std::chrono::seconds(20))
         emit connectingProgress(tr("verifying the local database").toStdString());
      else
         emit connectingProgress(tr("connecting").toStdString());
   }
   else
   {
      QDateTime qdt;
      qdt.setTime_t(std::chrono::system_clock::to_time_t(Globals::instance().getWallet().HeadBlockTime()));

      CalendarDuration duration = CalculateCalendarDuration(qdt, QDateTime::currentDateTime());
      if (duration.sign == CalendarDuration::sign_negative)
         duration = CalendarDuration();

      QString str_result = CalculateRemainingTime_Behind(qdt, QDateTime::currentDateTime());
      std::string result = str_result.toStdString();
      if (false == result.empty())
         emit statusShowMessage(result.c_str(), 5000);

      bool justbehind = false, farbehind = false;
      if (duration.years == 0 &&
          duration.months == 0 &&
          duration.days == 0 &&
          duration.hours == 0 &&
          duration.minutes == 0 &&
          duration.seconds < 30)
         justbehind = true;

      if (duration.minutes > 1 ||
          duration.hours > 0 ||
          duration.days > 0 ||
          duration.months > 0 ||
          duration.years > 0)
         farbehind = true;

      if (farbehind &&
          m_connected_state == ConnectionState::Up)
      {
         m_connected_state = ConnectionState::SyncingUp;
         emit walletConnectionStatusChanged(backup_state, m_connected_state);
      }
      else if (justbehind &&
               m_connected_state == ConnectionState::SyncingUp)
      {
         m_connected_state = ConnectionState::Up;
         emit walletConnectionStatusChanged(backup_state, m_connected_state);
      }
   }
}


//**
// DecentTable and DecentColumn
//**
// Table with additional functionality to use in our GUI
//
// DecentColumn
//

DecentColumn::DecentColumn(QString title, int size, std::string const& sortid/* = std::string()*/)
: title(title)
, size(size)
, sortid(sortid) {}

// DecentTable
//
DecentTable::DecentTable(QWidget* pParent)
   : QTableWidget(pParent)
{
   //horizontalHeader()->setStretchLastSection(true);
   setSelectionMode(QAbstractItemView::NoSelection);
   setStyleSheet("QTableView{border : 0px}");
   setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

   verticalHeader()->hide();
   setMouseTracking(true);

   connect(this->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sectionClicked(int)));
}

int DecentTable::getCurrentHighlightedRow() const
{
   return _current_highlighted_row;
}

std::string DecentTable::getSortedColumn() const
{
   if (_current_sort_index < 0)
      return "";

   return  (_is_ascending ? "+" : "-") + _cols[_current_sort_index].sortid;
}

void DecentTable::set_columns(const std::vector<DecentColumn>& cols)
{
   _cols = cols;

   this->setColumnCount(cols.size());

   this->horizontalHeader()->setDefaultSectionSize(300);
   this->setRowHeight(0,35);

   QStringList columns;
   for (int i = 0; i < cols.size(); ++i) {
      this->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
      columns << cols[i].title;
   }
   this->setHorizontalHeaderLabels(columns);

   _sum_weights = std::accumulate(_cols.begin(), _cols.end(),
                                  0, [](int sum, const DecentColumn& col) {
                                     return (col.size > 0) ? sum + col.size : sum;
                                  });

   _sum_absoulte = std::accumulate(_cols.begin(), _cols.end(),
                                   0, [](int sum, const DecentColumn& col) {
                                      return (col.size > 0) ? sum : sum - col.size;
                                   });

#ifdef WINDOWS_HIGH_DPI
   this->horizontalHeader()->setFixedHeight(45);
#else
   this->horizontalHeader()->setFixedHeight(35);
#endif
   this->horizontalHeader()->setFont(TableHeaderFont());

   this->horizontalHeader()->setStyleSheet("QHeaderView::section {"
                                           "border-right: 1px solid rgb(193,192,193);"
                                           "border-bottom: 0px;"
                                           "border-top: 0px;}");
}

void DecentTable::sectionClicked(int index)
{
   emit signal_SortingChanged(index);
   if (_cols[index].sortid.empty()) {
      return;
   }

   if (_current_sort_index == index) {
      _is_ascending = !_is_ascending;
   } else {
      _current_sort_index = index;
      _is_ascending = true;
   }
}

void DecentTable::resizeEvent(QResizeEvent * a_event)
{
   QSize tableSize = this->size();
   int width = tableSize.width() - _sum_absoulte;

   for(int i = 0; i < _cols.size(); ++i) {
      if (_cols[i].size > 0) {
         this->setColumnWidth(i, width * _cols[i].size / _sum_weights);
      } else {
         this->setColumnWidth(i, -_cols[i].size);
      }
   }
}

void DecentTable::mouseMoveEvent(QMouseEvent * event)
{
   if (_current_highlighted_row != -1) {
      for (int i = 0; i < this->columnCount(); ++i) {
         QTableWidgetItem* cell = this->item(_current_highlighted_row, i);
         QWidget* cell_widget = this->cellWidget(_current_highlighted_row, i);

         if(cell != NULL) {
            cell->setBackgroundColor(QColor(255,255,255));
            cell->setForeground(QColor::fromRgb(0,0,0));
         }

         if (cell_widget)
            cell_widget->setEnabled(false);
      }

   }

   int row = this->rowAt(event->pos().y());

   if(row < 0) {
      _current_highlighted_row = -1;
      return;
   }

   for (int i = 0; i < this->columnCount(); ++i) {
      QTableWidgetItem* cell = this->item(row, i);
      QWidget* cell_widget = this->cellWidget(row, i);

      if (cell != NULL) {
         cell->setBackgroundColor(QColor(27,176,104));
         cell->setForeground(QColor::fromRgb(255,255,255));
      }

      if (cell_widget)
         cell_widget->setEnabled(true);
   }
   _current_highlighted_row = row;
}

}

//
// below is decent and ipfs daemon staff, that's executed in a parallel thread
//
using namespace graphene;
namespace bpo = boost::program_options;


void write_default_logging_config_to_stream(std::ostream& out);
fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path& config_ini_filename);


bool check_for_ipfs(QObject* parent, QString program) {

   QProcess *checkProcess = new QProcess(parent);
   checkProcess->start(program, QStringList("version"));

   if (!checkProcess->waitForStarted(2000)) {
      return false;
   }


   if (!checkProcess->waitForFinished(2000)) {
      return false;
   }

   return true;
}

QString get_ipfs_path(QObject* parent, QString app_dir)
{
   QString ipfs_bin = QString::fromStdString(utilities::decent_path_finder::instance().get_ipfs_bin().string());
   QString ipfs_path_bin = QString::fromStdString((utilities::decent_path_finder::instance().get_ipfs_path() / "ipfs").string());

   QString ipfs_path_next_to_exe = app_dir + QDir::separator() + "ipfs";
   QString ipfs_path_next_to_bin_exe = app_dir + QDir::separator() + ".."  + QDir::separator() + "bin"  + QDir::separator() + "ipfs";


   if (utilities::decent_path_finder::instance().get_ipfs_bin() != fc::path()) {
      if (check_for_ipfs(parent, ipfs_bin)) {
         return ipfs_bin;
      }
   }

   if (utilities::decent_path_finder::instance().get_ipfs_path() != fc::path()) {
      if (check_for_ipfs(parent, ipfs_path_bin)) {
         return ipfs_path_bin;
      }
   }

   if (check_for_ipfs(parent, ipfs_path_next_to_exe)) {
      return ipfs_path_next_to_exe;
   }

   if (check_for_ipfs(parent, ipfs_path_next_to_bin_exe)) {
      return ipfs_path_next_to_bin_exe;
   }


   if (check_for_ipfs(parent, "ipfs")) {
      return "ipfs";
   }


   return "";
}


QProcess* run_ipfs_daemon(QObject* parent, QString app_dir)
{
   QString program = get_ipfs_path(parent, app_dir);

   if (program.isEmpty()) {
      throw std::runtime_error("Cannot find IPFS executable. Please export IPFS_BIN or IPFS_PATH environment variables");
   }

   QProcess *initProcess = new QProcess(parent);
   initProcess->start(program, QStringList("init"));

   // If init timeout throw something
   if (!initProcess->waitForFinished(2000)) {
      throw std::runtime_error("Timeout while initializing ipfs");
   }

   // Run daemon
   QProcess *daemonProcess = new QProcess(parent);
   daemonProcess->start(program, QStringList("daemon"));
   return daemonProcess;
}


int runDecentD(bool replay_blockchain, fc::promise<void>::ptr& exit_promise)
{
   app::application* node = new app::application();
   fc::oexception unhandled_exception;
   try {
      bpo::options_description app_options("DECENT Daemon");
      bpo::options_description cfg_options("DECENT Daemon");
      app_options.add_options()
      ("help,h", "Print this help message and exit.")
      ("data-dir,d", bpo::value<boost::filesystem::path>()->default_value( utilities::decent_path_finder::instance().get_decent_data() / "decentd"), "Directory containing databases, configuration file, etc.");


      bpo::variables_map options;

      auto witness_plug = node->register_plugin<witness_plugin::witness_plugin>();
      auto history_plug = node->register_plugin<account_history::account_history_plugin>();
      auto seeding_plug = node->register_plugin<decent::seeding::seeding_plugin>();

      try
      {
         bpo::options_description cli, cfg;
         node->set_program_options(cli, cfg);
         app_options.add(cli);
         cfg_options.add(cfg);

         int argc = 1;
         char str_dummy[] = "dummy";
         char str_replay[] = "--replay-blockchain";

         char* argvEmpty[] = {str_dummy};
         char* argvRestart[] = {str_dummy, str_replay};
         char** argv = argvEmpty;

         if (replay_blockchain)
         {
            argc = 2;
            argv = argvRestart;
         }

         bpo::store(bpo::parse_command_line(argc, argv, app_options), options);
      }
      catch (const boost::program_options::error& e)
      {
         std::cerr << "Error parsing command line: " << e.what() << "\n";
         return 1;
      }

      if( options.count("help") )
      {
         std::cout << app_options << "\n";
         return 0;
      }

      fc::path data_dir;
      if( options.count("data-dir") )
      {
         data_dir = options["data-dir"].as<boost::filesystem::path>();
         if( data_dir.is_relative() )
            data_dir = fc::current_path() / data_dir;
      }

      fc::path config_ini_path = data_dir / "config.ini";
      if( fc::exists(config_ini_path) )
      {
         // get the basic options
         bpo::store(bpo::parse_config_file<char>(config_ini_path.preferred_string().c_str(), cfg_options, true), options);

         // try to get logging options from the config file.
         try
         {
            fc::optional<fc::logging_config> logging_config = load_logging_config_from_ini_file(config_ini_path);
            if (logging_config)
               fc::configure_logging(*logging_config);
         }
         catch (const fc::exception&)
         {
            wlog("Error parsing logging config from config file ${config}, using default config", ("config", config_ini_path.preferred_string()));
         }
      }
      else
      {
         ilog("Writing new config file at ${path}", ("path", config_ini_path));
         if( !fc::exists(data_dir) )
            fc::create_directories(data_dir);

         std::ofstream out_cfg(config_ini_path.preferred_string());
         for( const boost::shared_ptr<bpo::option_description> od : cfg_options.options() )
         {
            if( !od->description().empty() )
               out_cfg << "# " << od->description() << "\n";
            boost::any store;
            if( !od->semantic()->apply_default(store) )
               out_cfg << "# " << od->long_name() << " = \n";
            else {
               auto example = od->format_parameter();
               if( example.empty() )
                  // This is a boolean switch
                  out_cfg << od->long_name() << " = " << "false\n";
               else {
                  // The string is formatted "arg (=<interesting part>)"
                  example.erase(0, 6);
                  example.erase(example.length()-1);
                  out_cfg << od->long_name() << " = " << example << "\n";
               }
            }
            out_cfg << "\n";
         }
         write_default_logging_config_to_stream(out_cfg);
         out_cfg.close();
         // read the default logging config we just wrote out to the file and start using it
         fc::optional<fc::logging_config> logging_config = load_logging_config_from_ini_file(config_ini_path);
         if (logging_config)
            fc::configure_logging(*logging_config);
      }
      bpo::notify(options);
      node->initialize(data_dir, options);
      node->initialize_plugins( options );

      node->startup();
      node->startup_plugins();

      ilog("Started witness node on a chain with ${h} blocks.", ("h", node->chain_database()->head_block_num()));
      ilog("Chain ID is ${id}", ("id", node->chain_database()->get_chain_id()) );

      if( !seeding_plug->my )
      {
         decent::seeding::seeding_promise = new fc::promise<decent::seeding::seeding_plugin_startup_options>("Seeding Promise");

         while( !decent::seeding::seeding_promise->ready() && !exit_promise->ready() ){
            fc::usleep(fc::microseconds(1000000));
         }

         if( decent::seeding::seeding_promise->ready() && !exit_promise->ready() ){
            seeding_plug->plugin_pre_startup(decent::seeding::seeding_promise->wait(fc::microseconds(1)));
         }
      }

      exit_promise->wait();
      node->shutdown_plugins();
      node->shutdown();
      delete node;
      return 0;
   } catch( const fc::exception& e ) {
      // deleting the node can yield, so do this outside the exception handler
      unhandled_exception = e;
   }

   if (unhandled_exception)
   {
      elog("Exiting with error:\n${e}", ("e", unhandled_exception->to_detail_string()));
      node->shutdown();
      delete node;
      return 1;
   }
}

void write_default_logging_config_to_stream(std::ostream& out)
{
   out << "# declare an appender named \"stderr\" that writes messages to the console\n"
   "[log.console_appender.stderr]\n"
   "stream=std_error\n\n"
   "# declare an appender named \"p2p\" that writes messages to p2p.log\n"
   "[log.file_appender.p2p]\n"
   "filename=logs/p2p/p2p.log\n"
   "# filename can be absolute or relative to this config file\n\n"
   "# route any messages logged to the default logger to the \"stderr\" logger we\n"
   "# declared above, if they are info level are higher\n"
   "[logger.default]\n"
   "level=info\n"
   "appenders=stderr\n\n"
   "# route messages sent to the \"p2p\" logger to the p2p appender declared above\n"
   "[logger.p2p]\n"
   "level=debug\n"
   "appenders=p2p\n\n";
}

fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path& config_ini_filename)
{
   try
   {
      fc::logging_config logging_config;
      bool found_logging_config = false;

      boost::property_tree::ptree config_ini_tree;
      boost::property_tree::ini_parser::read_ini(config_ini_filename.preferred_string().c_str(), config_ini_tree);
      for (const auto& section : config_ini_tree)
      {
         const string& section_name = section.first;
         const boost::property_tree::ptree& section_tree = section.second;

         const string console_appender_section_prefix = "log.console_appender.";
         const string file_appender_section_prefix = "log.file_appender.";
         const string logger_section_prefix = "logger.";

         if (boost::starts_with(section_name, console_appender_section_prefix))
         {
            string console_appender_name = section_name.substr(console_appender_section_prefix.length());
            string stream_name = section_tree.get<string>("stream");

            // construct a default console appender config here
            // stdout/stderr will be taken from ini file, everything else hard-coded here
            fc::console_appender::config console_appender_config;
            console_appender_config.level_colors.emplace_back(
                                                              fc::console_appender::level_color(fc::log_level::debug,
                                                                                                fc::console_appender::color::green));
            console_appender_config.level_colors.emplace_back(
                                                              fc::console_appender::level_color(fc::log_level::warn,
                                                                                                fc::console_appender::color::brown));
            console_appender_config.level_colors.emplace_back(
                                                              fc::console_appender::level_color(fc::log_level::error,
                                                                                                fc::console_appender::color::cyan));
            console_appender_config.stream = fc::variant(stream_name).as<fc::console_appender::stream::type>();
            logging_config.appenders.push_back(fc::appender_config(console_appender_name, "console", fc::variant(console_appender_config)));
            found_logging_config = true;
         }
         else if (boost::starts_with(section_name, file_appender_section_prefix))
         {
            string file_appender_name = section_name.substr(file_appender_section_prefix.length());
            fc::path file_name = section_tree.get<string>("filename");
            if (file_name.is_relative())
               file_name = fc::absolute(config_ini_filename).parent_path() / file_name;


            // construct a default file appender config here
            // filename will be taken from ini file, everything else hard-coded here
            fc::file_appender::config file_appender_config;
            file_appender_config.filename = file_name;
            file_appender_config.flush = true;
            file_appender_config.rotate = true;
            file_appender_config.rotation_interval = fc::hours(1);
            file_appender_config.rotation_limit = fc::days(1);
            logging_config.appenders.push_back(fc::appender_config(file_appender_name, "file", fc::variant(file_appender_config)));
            found_logging_config = true;
         }
         else if (boost::starts_with(section_name, logger_section_prefix))
         {
            string logger_name = section_name.substr(logger_section_prefix.length());
            string level_string = section_tree.get<string>("level");
            string appenders_string = section_tree.get<string>("appenders");
            fc::logger_config logger_config(logger_name);
            logger_config.level = fc::variant(level_string).as<fc::log_level>();
            boost::split(logger_config.appenders, appenders_string,
                         boost::is_any_of(" ,"),
                         boost::token_compress_on);
            logging_config.loggers.push_back(logger_config);
            found_logging_config = true;
         }
      }
      if (found_logging_config)
         return logging_config;
      else
         return fc::optional<fc::logging_config>();
   }
   FC_RETHROW_EXCEPTIONS(warn, "")
}
