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
#endif

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
         str_result += QObject::tr("syncing up with blockchain: ");
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
// Globals
//
Globals::Globals()
: m_connected(false)
, m_p_wallet_operator(new WalletOperator())
, m_p_wallet_operator_thread(new QThread(this))
, m_p_timer(new QTimer(this))
, m_p_locale(new QLocale())
, m_str_currentUser()
, m_tp_started(std::chrono::steady_clock::now())
{
   m_p_wallet_operator->moveToThread(m_p_wallet_operator_thread);
   m_p_wallet_operator_thread->start();

   QObject::connect(this, &Globals::signal_connect,
                    m_p_wallet_operator, &WalletOperator::slot_connect);
   QObject::connect(m_p_wallet_operator, &WalletOperator::signal_connected,
                    this, &Globals::slot_connected);

   emit signal_connect();

   m_p_timer->setSingleShot(true);
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

std::string Globals::getCurrentUser() const
{
   return m_str_currentUser;
}

bool Globals::isConnected() const
{
   return m_connected;
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
}

Asset Globals::asset(uint64_t amount)
{
   Asset ast_amount;
   uint8_t precision = 0;
   getWallet().LoadAssetInfo(ast_amount.m_str_symbol, precision);
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

void Globals::setCurrentUser(std::string const& user)
{
   m_str_currentUser = user;
   emit currentUserChanged(m_str_currentUser.c_str());
}

void Globals::setWalletUnlocked()
{
   emit walletUnlocked();
}

void Globals::setWalletConnected()
{
   m_connected = true;
   emit walletConnected();
}

void Globals::setWalletError(std::string const& error)
{
   emit walletConnectionError(error);
}

void Globals::showTransferDialog(std::string const& user)
{
   if(getCurrentUser().empty())
      return;
   
   TransferDialog* pTransferDialog = new TransferDialog(nullptr , QString::fromStdString(user));
   pTransferDialog->setAttribute(Qt::WA_DeleteOnClose);
   pTransferDialog->open();
}
   
void Globals::slot_displayWalletContent()
{
   emit signal_keyImported(false);
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
   m_connected = true;
   if (str_error.empty())
      emit walletConnected();
   else
      emit walletConnectionError(str_error);
}

void Globals::slot_timer()
{
   auto duration = std::chrono::steady_clock::now() - m_tp_started;

   if (false == m_connected)
   {
      if (duration > std::chrono::seconds(40))
         emit connectingProgress(tr("still connecting").toStdString());
      else if (duration > std::chrono::seconds(30))
         emit connectingProgress(tr("verifying the local database, probably the local database is corrupted").toStdString());
      else if (duration > std::chrono::seconds(20))
         emit connectingProgress(tr("verifying the local database").toStdString());
      else
         emit connectingProgress(tr("connecting").toStdString());

      m_p_timer->start(1000);
   }
   else
      emit connectingProgress(std::string());
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
