/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QHeaderView>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QThread>
#include <QTimer>

#include <boost/filesystem.hpp>

#include <fc/interprocess/signals.hpp>
#endif

#include "gui_wallet_global.hpp"
#include "richdialog.hpp"

//#define DECENT_WITHOUT_DAEMON

#include <signal.h>

#ifdef _MSC_VER
#include <tlhelp32.h>
#endif

int runDecentD(gui_wallet::BlockChainStartType type, fc::promise<void>::ptr& exit_promise);
QProcess* run_ipfs_daemon(QObject* parent, const QString& app_dir);

namespace gui_wallet
{
   
void ShowMessageBox(const QString& strTitle,
                    const QString& strMessage,
                    const QString& strDetailedText/* = QString()*/,
                    QWidget* parent)
{
   QMessageBox* pMessageBox = new QMessageBox(parent);
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
      return std::stoll(o.get<std::string>());
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

QString CalculateRemainingTime(const QDateTime& dt, const QDateTime& dtFuture)
{
   CalendarDuration duration = CalculateCalendarDuration(dt, dtFuture);
   if (duration.sign == CalendarDuration::sign_negative)
      return QString(QObject::tr("expired"));
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

      QString str_result;
      if (arrParts.empty())
         str_result = QString(QObject::tr("expiring in a minute"));
      else
      {
         str_result = QString::fromStdString( arrParts.front());
         
         if (arrParts.size() > 1) {
            str_result.append(" ");
            str_result.append(QString::fromStdString(arrParts[1]));
         }
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

QString convertDateToLocale(const std::string& s)
{
   QDate time = QDate::fromString(QString::fromStdString(s), "yyyy-MM-dd");
   if (!time.isValid()) {
      return QString("EEE");
   }

   return Globals::instance().locale().toString(time, QLocale::ShortFormat);
}

QDateTime convertStringToDateTime(const std::string& s)
{
   return QDateTime::fromString(QString::fromStdString(s), "yyyy-MM-ddTHH:mm:ss");
}

QString convertDateTimeToLocale(const std::string& s)
{
   QDateTime time = QDateTime::fromString(QString::fromStdString(s), "yyyy-MM-dd hh:mm:ss");
   if (!time.isValid()) {
      return QString("EEE");
   }

   return Globals::instance().locale().toString(time, QLocale::ShortFormat);
}
QString convertDateTimeToLocale2(const std::string& s)
{
   if (s.empty()) {
      return QString();
   }

   QDateTime time = convertStringToDateTime(s);
   if (!time.isValid()) {
      return QString("EEE");
   }

   return Globals::instance().locale().toString(time, QLocale::ShortFormat);
}

//
// WalletOperator
//
WalletOperator::WalletOperator(const boost::filesystem::path &wallet_file, const graphene::wallet::server_data &ws)
: m_wallet_api(wallet_file, ws)
, m_cancellation_token(false)
{
}

WalletOperator::~WalletOperator() = default;

void WalletOperator::cancel()
{
   m_cancellation_token = true;
}

void WalletOperator::slot_connect()
{
   std::string str_error;
   try
   {
      m_wallet_api.Connect(m_cancellation_token);
   }
   catch(const std::exception& ex)
   {
      str_error = ex.what();
   }

   emit signal_connected(str_error);
}

//
// Asset
//
Asset::Asset(const std::string &str_symbol, uint8_t precision, uint64_t amount)
   : m_amount(amount), m_scale(pow(10, precision)), m_str_symbol(str_symbol)
{
}

double Asset::to_value() const
{
   uint64_t amount = m_amount / m_scale;
   double tail = double(m_amount % m_scale) / m_scale;
   return amount + tail;
}

bool Asset::hasDecimals() const
{
   return double(m_amount % m_scale) > 0.0;
}

QString Asset::getString() const
{
   if (m_amount > 0) {

      QLocale& locale = Globals::instance().locale();

      if (hasDecimals())
         return locale.toString(to_value(), 'g', g_max_number_of_decimal_places) + " " + QString::fromStdString(m_str_symbol);
      else
         return locale.toString((int)to_value()) + " " + QString::fromStdString(m_str_symbol);
   }
   else {
      return QString("Free");
   }
}

QString Asset::getBalance() const
{
   QLocale& locale = Globals::instance().locale();
   return locale.toString(to_value(), 'g' , g_max_number_of_decimal_places);
}

const std::string Asset::dct_id("1.3.0");

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
Globals::Globals() : QObject()
, m_connected_state(ConnectionState::NoState)
, m_p_wallet_operator(nullptr)
, m_p_wallet_operator_thread(nullptr)
, m_p_timer(new QTimer(this))
, m_p_locale(new QLocale())
, m_p_daemon_details(nullptr)
, m_logger(fc::logger::get("gui"))
, m_str_currentUser()
, m_tp_started(std::chrono::steady_clock::now())
{

   QLocale::setDefault(*m_p_locale);

//   *m_p_locale = ((QApplication*)QApplication::instance())->inputMethod()->locale();
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

void Globals::setCommandLine(bpo::options_description &app_options, bpo::options_description &cfg_options)
{
   bpo::options_description cli, cfg;
   graphene::app::application::set_program_options(cli, cfg);
   gui_wallet::Globals::Plugins::set_program_options(cli, cfg);
   cli.add_options()
      ("generate-keys,g", "Generate brain, wif private and public keys.")
      ("wallet-file,w", bpo::value<std::string>()->default_value(
          (graphene::utilities::decent_path_finder::instance().get_decent_home() / "wallet.json").generic_string()), "Wallet to load.")
      ("log-level,l", bpo::value<char>()->default_value('I'), "Set minimum log level: (D)ebug, (I)nfo, (W)arning, (E)rror, (O)ff")
   ;

   app_options.add(cli);
   cfg_options.add(cfg);
}

void Globals::startDaemons(BlockChainStartType type, const std::string &wallet_file, const graphene::wallet::server_data &ws)
{
   if (m_p_daemon_details)
      return;

   m_p_daemon_details = new DaemonDetails();

   bool bNeedNewConnection = false;

   if (nullptr == m_p_wallet_operator)
   {
      Q_ASSERT(m_p_wallet_operator_thread == nullptr);
      m_p_wallet_operator_thread = new QThread(this);


      bNeedNewConnection = true;
      m_p_wallet_operator = new WalletOperator(wallet_file, ws);
      m_p_wallet_operator->moveToThread(m_p_wallet_operator_thread);
      QObject::connect(this, &Globals::signal_connect,
                       m_p_wallet_operator, &WalletOperator::slot_connect);
      QObject::connect(m_p_wallet_operator, &WalletOperator::signal_connected,
                       this, &Globals::slot_connected);

      m_p_wallet_operator_thread->start();
   }

   QObject::connect(this, &Globals::walletConnectionStatusChanged, this, &Globals::slot_ConnectionStatusChange);

   QProcess* daemon_process = nullptr;
   if (IsIpfsRunning() == false) {
      daemon_process = run_ipfs_daemon(qApp, qApp->applicationDirPath());
      if (daemon_process == nullptr) {
         elog("Cannot start IPFS.");
         ShowMessageBox(tr("Error"), tr("Submiting or downloading content will not be functional."), tr("Cannot start IPFS"));
      }
   }

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

#if !defined(DECENT_WITHOUT_DAEMON)
   fc::thread& thread_decentd = m_p_daemon_details->thread_decentd;

   fc::promise<void>::ptr& exit_promise = m_p_daemon_details->exit_promise;

   m_p_daemon_details->future_decentd = thread_decentd.async([type, &exit_promise]() -> int
                                                            {
                                                               return ::runDecentD(type, exit_promise);
                                                            });
   m_p_daemon_details->future_decentd.on_complete([this](int ret, const fc::exception_ptr& e) { emit signal_daemonFinished(e ? -1 : ret); });
#endif

   m_tp_started = std::chrono::steady_clock::now();

   emit walletConnectionStatusChanged(Globals::ConnectionState::NoState, Globals::ConnectionState::NoState);

   if (bNeedNewConnection)
      emit signal_connect();
}
   
void Globals::stopDaemons()
{
   bool bConnected = connected();

   auto backup_state = m_connected_state;
   m_connected_state = ConnectionState::Connecting;
   if (backup_state != m_connected_state)
      emit walletConnectionStatusChanged(backup_state, m_connected_state);

   if (m_p_wallet_operator_thread)
   {
      m_p_wallet_operator_thread->quit();
      m_p_wallet_operator_thread->wait();
      delete m_p_wallet_operator_thread;
      m_p_wallet_operator_thread = nullptr;
   }

   if (m_p_wallet_operator)
   {
      if (bConnected) {
         m_p_wallet_operator->m_wallet_api.SaveWalletFile();
      }
      else {
         m_p_wallet_operator->cancel();
      }

      delete m_p_wallet_operator;
      m_p_wallet_operator = nullptr;
   }

#if !defined(DECENT_WITHOUT_DAEMON)
   fc::promise<void>::ptr& exit_promise = m_p_daemon_details->exit_promise;
   exit_promise->set_value();

   m_p_daemon_details->future_decentd.wait();
#endif

   if (m_p_daemon_details->ipfs_process) {
      m_p_daemon_details->ipfs_process->terminate();
      if (!m_p_daemon_details->ipfs_process->waitForFinished(5000)) {
         m_p_daemon_details->ipfs_process->kill();
      }

      delete m_p_daemon_details->ipfs_process;
      m_p_daemon_details->ipfs_process = nullptr;
   }

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
      if (connected())
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

Asset Globals::asset(uint64_t amount, const std::string& assetId)
{
   auto search = m_asset_symbols_cache.find(assetId);
   if (search == m_asset_symbols_cache.end())
   {
      size_t limit = 100;
      std::string lowerbound;
      do
      {
         nlohmann::json assets = runTaskParse("list_assets \"" + lowerbound + "\" 100");
         limit = assets.size();
         if (!assets.empty())
            lowerbound = assets.back()["symbol"].get<std::string>();

         std::for_each(assets.begin(), assets.end(), [&](const nlohmann::json& asset) {
            m_asset_symbols_cache[asset["id"].get<std::string>()] = Asset(asset["symbol"].get<std::string>(), asset["precision"].get<uint8_t>());
         } );
      } while (limit == 100);

      search = m_asset_symbols_cache.find(assetId);
      if (search == m_asset_symbols_cache.end())
         return Asset();
   }

   Asset asset(search->second);
   asset.m_amount = amount;
   return asset;
}

std::string Globals::runTask(std::string const& str_command)
{
   GUI_DLOG("Running task ${c}", ("c", str_command));
   return getWallet().RunTask(str_command);
}

nlohmann::json Globals::runTaskParse(std::string const& str_command)
{
   std::string str_result = runTask(str_command);
   return nlohmann::json::parse(str_result);
}

std::vector<Publisher> Globals::getPublishers()
{
   auto publishers = runTaskParse("list_seeders_by_price 100");
   std::vector<Publisher> result;

   result.reserve(publishers.size());
   for (size_t iIndex = 0; iIndex < publishers.size(); ++iIndex)
   {
      result.push_back(Publisher());
      Publisher& publisher = result.back();

      publisher.m_str_name = publishers[iIndex]["seeder"].get<std::string>();
      uint64_t iPrice = json_to_int64(publishers[iIndex]["price"]["amount"]);
      std::string iSymbolId = publishers[iIndex]["price"]["asset_id"];
      publisher.m_price = asset(iPrice, iSymbolId);
      publisher.m_storage_size = publishers[iIndex]["free_space"].get<int>();
   }

   return result;
}

bool Globals::connected() const
{
   return m_connected_state != ConnectionState::Connecting;
}

void Globals::setWalletUnlocked()
{
   emit walletUnlocked();
}

void Globals::setWalletError(std::string const& error)
{
   emit walletConnectionError(error);
}

std::string Globals::getAccountName(std::string const& accountId)
{
   auto search = m_map_user_id_cache.find(accountId);
   if (search == m_map_user_id_cache.end())
   {
      std::string accountName("Unknown");
      nlohmann::json accountInfoParsed = runTaskParse("get_object \"" + accountId + "\"");

      if (!accountInfoParsed.empty())
         accountName = accountInfoParsed[0]["name"].get<std::string>();

      auto pair_value = m_map_user_id_cache.insert(std::make_pair(accountId, accountName));
      search = pair_value.first;
   }

   return search->second;
}

void Globals::setCurrentAccount(const QString& account_name)
{
   m_str_currentUser = account_name.toStdString();
   emit currentUserChanged(account_name);

   slot_updateAccountBalance();
}

Asset Globals::getDCoreFees(int iOperation)
{
   nlohmann::json global_prop_info = runTaskParse("get_global_properties");
   nlohmann::json param = global_prop_info["parameters"]["current_fees"]["parameters"];
   nlohmann::json curr_fee = param[iOperation];  //account_update_operation

   return asset(curr_fee[1]["fee"].get<uint64_t>() );
}

void Globals::slot_updateAccountBalance()
{
   if (!m_str_currentUser.empty())
   {
      nlohmann::json allBalances = runTaskParse("list_account_balances " + m_str_currentUser);

      QList<Asset> assets;
      if (!allBalances.empty())
      {
         for ( const auto &balance : allBalances )
         {
            const std::string& assetId = balance["asset_id"].get<std::string>();
            uint64_t amount =  balance[ "amount" ].is_number() ?
               balance[ "amount" ].get<uint64_t>() :
               static_cast<uint64_t>(std::stoll(balance[ "amount" ].get<std::string>()));

            Asset a = asset(amount, assetId);
            if (assetId == Asset::dct_id)
               assets.prepend(a);
            else
               assets.append(a);
         }
      }

      if (assets.empty())
      {
         assets << asset(0);
      }

      emit signal_updateAccountAssets(assets);
   }
}

std::string Globals::ImportAccount(const std::string& name, const std::string& key)
{
   std::string message;

   try
   {
      std::string command = "import_key ";
      command += "\"" + name + "\" ";
      command += "\"" + key + "\" ";
      auto result = Globals::instance().runTaskParse(command);
      bool tf = result.get<bool>();
      if (!tf) {
         message = "Invalid key";
      }
   }
   catch (const std::exception& ex)
   {
      message = ex.what();
   }
   catch (const fc::exception& ex)
   {
      message = ex.what();
   }

   return message;
}

std::string Globals::TransferFunds(const std::string& from, const std::string to,
                                   double amount, const std::string& asset_symbol, const std::string& memo)
{
   std::string result;
   try {
      std::string command = "transfer ";
      command += "\"" + from + "\" ";
      command += "\"" + to + "\" ";
      command += "\"" + QString::number(amount).toStdString() + "\" " + asset_symbol + " ";
      command += "\"" + memo + "\"";
      command += " true";

      std::string dummy = runTask(command);
      if (dummy.find("exception:") != std::string::npos) {
         result = dummy;
      }
   }
   catch(const std::exception& ex)
   {
      result = ex.what();
   }
   catch(const fc::exception& ex)
   {
      result = ex.what();
   }

   return result;
}

void Globals::slot_showTransferDialog(const QString& user)
{
   if(m_str_currentUser.empty())
      return;

   nlohmann::json allBalances = runTaskParse("list_account_balances " + m_str_currentUser);

   QList<QPair<QString, QString>> assets;
   if (!allBalances.empty())
   {
      for ( const auto &balance : allBalances )
      {
         const std::string& assetId = balance["asset_id"].get<std::string>();
         Asset a = asset(0, assetId);
         auto asset = qMakePair(QString::fromStdString(a.m_str_symbol), QString::fromStdString(assetId));
         if (assetId == Asset::dct_id)
            assets.prepend(asset);
         else
            assets.append(asset);
      }
   }

   if (assets.empty())
   {
      assets << qMakePair(QString::fromStdString(asset(0).m_str_symbol), QString::fromStdString(Asset::dct_id));
   }

   TransferWidget* pTransferDialog = new TransferWidget(nullptr, assets, user);
   signal_stackWidgetPush(pTransferDialog);
}

void Globals::slot_showTransferDialog()
{
   slot_showTransferDialog(QString());
}

void Globals::slot_connected(std::string const& str_error)
{
   m_connected_state = ConnectionState::SyncingUp;
   if (str_error.empty()) {
      emit walletConnectionStatusChanged(ConnectionState::Connecting, ConnectionState::SyncingUp);

      m_blockStart = getWallet().HeadBlockTime();
   }
   else
      emit walletConnectionError(str_error);
}

void Globals::slot_ConnectionStatusChange(ConnectionState from, ConnectionState to)
{
   if (ConnectionState::NoState == from) {

      m_p_timer->start(1000);
      QObject::connect(m_p_timer, &QTimer::timeout, this, &Globals::slot_timer);
   }
   else if (ConnectionState::SyncingUp == from && ConnectionState::Up == to) {
      m_p_timer->stop();
   }
}

static graphene::app::application* s_node = nullptr;

void Globals::slot_timer()
{
   auto backup_state = m_connected_state;

   if (m_exceptionMsgBoxParam1.size()) {
      QMessageBox* msgBox = new QMessageBox();
      msgBox->setWindowTitle(m_exceptionMsgBoxParam1.c_str());
      msgBox->setText(QString::fromStdString(m_exceptionMsgBoxParam3.c_str()));
      msgBox->exec();

      m_p_timer->stop();
      exit(1);
      return;
   }

   double reindexing_percent = 0;
   if (s_node && m_connected_state == ConnectionState::NoState) {
      reindexing_percent = s_node->chain_database()->get_reindexing_percent();
      if (reindexing_percent >= 0 && reindexing_percent < 100) {
         m_connected_state = ConnectionState::Reindexing;
         emit walletConnectionStatusChanged(ConnectionState::NoState, ConnectionState::Reindexing);
      }
      else
      if (reindexing_percent == 100) {
         m_connected_state = ConnectionState::Connecting;
         emit walletConnectionStatusChanged(ConnectionState::NoState, ConnectionState::Connecting);
      }
   } else
   if (s_node && ConnectionState::Reindexing == m_connected_state)
   {
      double reindexing_percent = s_node->chain_database()->get_reindexing_percent();
      if (reindexing_percent != -1) {
         if (reindexing_percent == 100) {
            m_connected_state = ConnectionState::Connecting;
            emit walletConnectionStatusChanged(ConnectionState::Reindexing, ConnectionState::Connecting);
         }
         else {
            QString showMsg = QString::number((int)reindexing_percent);
            emit progressCommonTextMessage("reindexing database...");
            emit updateProgress((int)reindexing_percent, 100);
         }
      }
   } else
   if (ConnectionState::Connecting == m_connected_state)
   {
      emit progressCommonTextMessage("connecting...");
   }
   else 
   {
      auto currBlockTime = Globals::instance().getWallet().HeadBlockTime();
      uint64_t value = std::chrono::duration_cast<std::chrono::seconds>(currBlockTime - m_blockStart).count();
      uint64_t maxValue = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - m_blockStart).count();

      emit updateProgress(value, maxValue);

      QDateTime qdt;
      qdt.setTime_t(std::chrono::system_clock::to_time_t(currBlockTime));

      CalendarDuration duration = CalculateCalendarDuration(qdt, QDateTime::currentDateTime());
      if (duration.sign == CalendarDuration::sign_negative)
         duration = CalendarDuration();

      QString str_result = CalculateRemainingTime_Behind(qdt, QDateTime::currentDateTime());
      if (!str_result.isEmpty()) {
         emit progressSyncMessage(str_result, 5000);
      }

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

void Globals::display_error_and_stop_slot_timer(std::string param1, std::string param2, std::string param3)
{
   m_exceptionMsgBoxParam1 = param1;
   m_exceptionMsgBoxParam2 = param2;
   m_exceptionMsgBoxParam3 = param3;
}

//**
// DecentTable and DecentColumn
//**
// Table with additional functionality to use in our GUI
//
// DecentColumn
//

DecentColumn::DecentColumn(const QString& title, int size, std::string const& sortid/* = std::string()*/)
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
      return std::string();

   return  (_is_ascending ? "+" : "-") + _cols[_current_sort_index].sortid;
}

void DecentTable::set_columns(const std::vector<DecentColumn>& cols)
{
   _cols = cols;

   setColumnCount(static_cast<int>(cols.size()));

   QStringList columns;
   for (size_t i = 0; i < cols.size(); ++i) {
      this->horizontalHeader()->setSectionResizeMode(static_cast<int>(i), QHeaderView::Fixed);
      columns << cols[i].title;
   }
   setHorizontalHeaderLabels(columns);

   _sum_weights = std::accumulate(_cols.begin(), _cols.end(),
                                  0, [](int sum, const DecentColumn& col) {
                                     return (col.size > 0) ? sum + col.size : sum;
                                  });

   _sum_absoulte = std::accumulate(_cols.begin(), _cols.end(),
                                   0, [](int sum, const DecentColumn& col) {
                                      return (col.size > 0) ? sum : sum - col.size;
                                   });
}

void DecentTable::setRowCount(int rows)
{
   QTableWidget::setRowCount(rows);

   for(int i = 0; i < rowCount(); ++i)
   {
      setRowHeight(i, 30 * gui_wallet::scale());
   }
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

   for(size_t i = 0; i < _cols.size(); ++i) {
      if (_cols[i].size > 0) {
         setColumnWidth(static_cast<int>(i), width * _cols[i].size / _sum_weights);
      } else {
         setColumnWidth(static_cast<int>(i), -_cols[i].size);
      }
   }
}

void DecentTable::mouseMoveEvent(QMouseEvent * event)
{
   if (_current_highlighted_row != -1) {
      for (int i = 0; i < this->columnCount(); ++i) {
         QTableWidgetItem* cell = this->item(_current_highlighted_row, i);
         QWidget* cell_widget = this->cellWidget(_current_highlighted_row, i);

         if(cell != nullptr) {
            cell->setBackgroundColor(QColor(255,255,255));
            cell->setForeground(QColor::fromRgb(0,0,0));
         }

         if (cell_widget)
            cell_widget->setEnabled(false);
      }

   }

   int row = rowAt(event->pos().y());

   if(row < 0) {
      _current_highlighted_row = -1;
      return;
   }

   for (int i = 0; i < this->columnCount(); ++i) {
      QTableWidgetItem* cell = this->item(row, i);
      QWidget* cell_widget = this->cellWidget(row, i);

      if (cell != nullptr) {
         cell->setBackgroundColor(QColor(27,176,104));
         cell->setForeground(QColor::fromRgb(255,255,255));
      }

      if (cell_widget)
         cell_widget->setEnabled(true);
   }
   _current_highlighted_row = row;
}
bool IsIpfsRunning()
{
#if defined( _MSC_VER )
   const char* processName = "ipfs.exe";
   PROCESSENTRY32 entry;
   entry.dwSize = sizeof(PROCESSENTRY32);

   HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

   if (Process32First(snapshot, &entry))
      do {
         if (!stricmp(entry.szExeFile, processName))
            return true;
      } while (Process32Next(snapshot, &entry));

   CloseHandle(snapshot);
     
   return false;
#elif defined( __GNUC__ )
   // add detection of running IPFS process
   return false;
#else
#error "Undefined compiler platform"
#endif
}



}

//
// below is decent and ipfs daemon staff, that's executed in a parallel thread
//
using namespace graphene;

bool check_for_ipfs(QObject* parent, const QString& program) {

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

QString get_ipfs_path(QObject* parent, const QString& app_dir)
{
   QString ipfs_bin = QString::fromStdString(utilities::decent_path_finder::instance().get_ipfs_bin().string());
   QString ipfs_path_bin = QString::fromStdString((utilities::decent_path_finder::instance().get_ipfs_path() / "ipfs").string());

   QString ipfs_path_next_to_exe = app_dir + QDir::separator() + "ipfs";
   QString ipfs_path_next_to_bin_exe = app_dir + QDir::separator() + ".."  + QDir::separator() + "bin"  + QDir::separator() + "ipfs";


   if (utilities::decent_path_finder::instance().get_ipfs_bin() != boost::filesystem::path()) {
      if (check_for_ipfs(parent, ipfs_bin)) {
         return ipfs_bin;
      }
   }

   if (utilities::decent_path_finder::instance().get_ipfs_path() != boost::filesystem::path()) {
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

   return QString();
}


QProcess* run_ipfs_daemon(QObject* parent, const QString& app_dir)
{
   QString program = get_ipfs_path(parent, app_dir);

   if (program.isEmpty()) {
      throw std::runtime_error("Cannot find IPFS executable. Please export IPFS_BIN or IPFS_PATH environment variables");
   }

   // Run daemon
   QProcess *daemonProcess = new QProcess(parent);
   QStringList params;
   params << "daemon" << "--init" << "--migrate";
   daemonProcess->start(program, params);
   if (daemonProcess->state() == QProcess::NotRunning) {
      delete daemonProcess;
      return nullptr;
   }
   return daemonProcess;
}

int runDecentD(gui_wallet::BlockChainStartType type, fc::promise<void>::ptr& exit_promise)
{
   bpo::options_description app_options("DECENT Daemon");
   bpo::options_description cfg_options("DECENT Daemon");
   bpo::variables_map options;

   try
   {
      gui_wallet::Globals::setCommandLine(app_options, cfg_options);

      const QStringList app_args = QCoreApplication::arguments();
      std::vector<std::string> args(std::vector<std::string>::size_type(app_args.count() - 1));
      std::transform(app_args.begin() + 1, app_args.end(), args.begin(), [](const QString &s) { return s.toStdString(); });
      if (type == gui_wallet::BlockChainStartType::Replay)
      {
         args.push_back("--replay-blockchain");
      }
      else if (type == gui_wallet::BlockChainStartType::Resync)
      {
         args.push_back("--resync-blockchain");
      }

      bpo::command_line_parser parser(args);
      bpo::store(parser.options(app_options).style(0).extra_parser(bpo::ext_parser()).run(), options);
   }
   catch (const bpo::error& e)
   {
      GUI_ELOG("Error parsing command line: ${e}", ("e", e.what()));
      return EXIT_FAILURE;
   }

   app::application* node = new app::application();
   fc::oexception unhandled_exception;
   try {
      gui_wallet::Globals::Plugins::types plugins = gui_wallet::Globals::Plugins::create(*node);

      boost::filesystem::path data_dir;
      if( options.count("data-dir") )
      {
         data_dir = options["data-dir"].as<boost::filesystem::path>();
         if( data_dir.is_relative() )
            data_dir = boost::filesystem::current_path() / data_dir;
      }

      boost::filesystem::path config_ini_path = data_dir / "config.ini";
      if( exists(config_ini_path) )
      {
         // get the basic options
         bpo::store(bpo::parse_config_file<char>(config_ini_path.make_preferred().string().c_str(), cfg_options, true), options);
      }

      bpo::notify(options);
      node->initialize(data_dir, options);
      node->initialize_plugins( options );

      gui_wallet::s_node = node;
      node->startup();
      node->startup_plugins();

      ilog("Started miner node on a chain with ${h} blocks.", ("h", node->chain_database()->head_block_num()));
      ilog("Chain ID is ${id}", ("id", node->chain_database()->get_chain_id()) );

      exit_promise->wait();
      node->shutdown_plugins();
      node->shutdown();
      delete node;
   } catch( const fc::exception& e ) {
      // deleting the node can yield, so do this outside the exception handler
      unhandled_exception = e;
   }

   if (unhandled_exception)
   {
      elog("Exiting with error:\n${e}", ("e", unhandled_exception->to_detail_string()));
      gui_wallet::Globals::instance().display_error_and_stop_slot_timer("Error", "Unhandled exception", unhandled_exception->to_detail_string());
      node->shutdown();
      delete node;
      return 1;
   }
   else
      return 0;
}

qreal gui_wallet::scale()
{
   //
   // this function in theory can
   // get scale not only based on static conditions
   //
   // this is the single place defining the scale
   // of all controls and fonts. this is supporting
   // different dpi settings
   // all controls use "em" unit that follow font sizes
   // and behave this scale
   // only exception is the shfitty qtablewidget
   //
   qreal local_scale = 1;
   return
#if defined(_WIN32)
   local_scale / 12 * 8
#elif defined (WINDOWS_HIGH_DPI)
   local_scale / 12 * 8
#elif __APPLE__
   local_scale / 12 * 12
#elif __linux__
   local_scale / 12 * 10
#else
   local_scale / 12 * 12
#endif
   ;
}

qreal FontSize(qreal size)
{
   return size * gui_wallet::scale();
}


QFont gui_wallet::PaginationFont()
{
   QFont font("Open Sans");
   font.setPointSizeF(FontSize(12));
   return font;
}

QFont gui_wallet::AccountBalanceFont()
{
   QFont font("Open Sans");
   font.setPointSizeF(FontSize(12));
   return font;
}

QFont gui_wallet::DescriptionDetailsFont()
{
   QFont font("Open Sans");
   font.setPointSizeF(FontSize(12));
   return font;
}

QFont gui_wallet::PopupButtonRegularFont()
{
   QFont font("Open Sans");
   font.setPointSizeF(FontSize(12));
   return font;
}

QFont gui_wallet::PopupButtonBigFont()
{
   QFont font("Open Sans");
   font.setPointSizeF(FontSize(12));
   return font;
}

QFont gui_wallet::TabButtonFont()
{
   QFont font("Open Sans");
   font.setPointSizeF(FontSize(12));
   return font;
}

QFont gui_wallet::ProgressInfoFont()
{
   QFont font("Open Sans");
   font.setPointSizeF(FontSize(10));
   return font;
}

QFont gui_wallet::MainFont()
{
   QFont font("Open Sans");
   font.setPointSizeF(FontSize(12));
   return font;
}
