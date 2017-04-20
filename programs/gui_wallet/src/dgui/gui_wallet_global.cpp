#ifdef _MSC_VER
#include "stdafx.h"
#endif

#include "gui_wallet_global.hpp"
#ifndef _MSC_VER
#include <QMessageBox>
#include <QThread>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QTimer>
#include <QHeaderView>

#include <iostream>
#endif

namespace gui_wallet
{
   
void ShowMessageBox(QString const& strTitle,
                    QString const& strMessage,
                    QString const& strDetailedText/* = QString()*/)
{
   QMessageBox* pMessageBox = new QMessageBox();
   pMessageBox->setWindowTitle(strTitle);
   pMessageBox->setText(strMessage);
   pMessageBox->setDetailedText(strDetailedText);
   pMessageBox->setAttribute(Qt::WA_DeleteOnClose);
   pMessageBox->open();
   // alternatively can connect to delete later as below
   //pMessageBox->open(pMessageBox, SLOT(deleteLater()));
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

std::string CalculateRemainingTime_Behind(QDateTime const& dt, QDateTime const& dtFuture)
{
   CalendarDuration duration = CalculateCalendarDuration(dt, dtFuture);
   if (duration.sign == CalendarDuration::sign_negative)
      return std::string();
   else
   {
      std::string str_result;
      std::vector<std::string> arrParts;

      if (duration.years > 1)
         arrParts.push_back(std::to_string(duration.years) + " years");
      else if (duration.years)
         arrParts.push_back("a year");
      if (duration.months > 1)
         arrParts.push_back(std::to_string(duration.months) + " months");
      else if (duration.months)
         arrParts.push_back("a month");
      if (duration.days > 1)
         arrParts.push_back(std::to_string(duration.days) + " days");
      else if (duration.days)
         arrParts.push_back("a day");
      if (duration.hours > 1)
         arrParts.push_back(std::to_string(duration.hours) + " hours");
      else if (duration.hours)
         arrParts.push_back("an hour");
      if (duration.minutes > 1)
         arrParts.push_back(std::to_string(duration.minutes) + " minutes");
      else if (duration.minutes)
         arrParts.push_back("a minute");
      if (duration.seconds > 1)
         arrParts.push_back(std::to_string(duration.seconds) + " seconds");
      else if (duration.seconds)
         arrParts.push_back("a second");

      if (arrParts.empty())
         return std::string();
      else if (arrParts.size() == 1 &&
               duration.seconds < 30)
         return std::string();
      else
      {
         str_result += "syncing up with blockchain: ";
         str_result += arrParts[0];

         if (arrParts.size() > 1)
            str_result += " and " + arrParts[1];

         str_result += " to go";
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
   emit signal_connected(str_error);
}
//
// Globals
//
Globals::Globals()
: m_connected(false)
, m_p_wallet_operator(new WalletOperator())
, m_p_wallet_operator_thread(new QThread(this))
, m_p_timer(new QTimer())
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
}

void Globals::setCurrentUser(std::string const& user)
{
   m_str_currentUser = user;
   emit currentUserChanged(m_str_currentUser);
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
         emit connectingProgress("still connecting");
      else if (duration > std::chrono::seconds(30))
         emit connectingProgress("verifying the local database, probably the local database is corrupted");
      else if (duration > std::chrono::seconds(20))
         emit connectingProgress("verifying the local databse");
      else
         emit connectingProgress("connecting");

      m_p_timer->start(1000);
   }
   else
      emit connectingProgress(std::string());
}
//
// DecentSmallButton
//

DecentSmallButton::DecentSmallButton(const QString& normalImg, const QString& highlightedImg, QWidget* pParent/* = nullptr*/)
: QLabel(pParent)
{
   normalImage.load(normalImg);
   highlightedImage.load(highlightedImg);
   this->setPixmap(normalImg);
}

void DecentSmallButton::unhighlight()
{
   this->setPixmap(normalImage);
   this->setStyleSheet("* { background-color: rgb(255,255,255); color : black; }");
}

void DecentSmallButton::highlight()
{
   this->setPixmap(highlightedImage);
   this->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");
}

void DecentSmallButton::mousePressEvent(QMouseEvent* event)
{
   emit clicked();
}

//**
// DecentTable and DecentColumn
//**
// Table with additional functionality to use in our GUI
//
// DecentColumn
//

DecentColumn::DecentColumn(std::string title, int size, std::string const& sortid/* = std::string()*/)
: title(title)
, size(size)
, sortid(sortid) {}

// DecentTable
//
DecentTable::DecentTable(QWidget* pParent)
   : QTableWidget(pParent)
{
   this->horizontalHeader()->setStretchLastSection(true);
   this->setSelectionMode(QAbstractItemView::NoSelection);
   this->setStyleSheet("QTableView{border : 0px}");
   this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

   this->verticalHeader()->hide();
   this->setMouseTracking(true);

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
   for (const DecentColumn& col: cols) {
      columns << QString::fromStdString(col.title);
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


   this->horizontalHeader()->setFixedHeight(35);
   this->horizontalHeader()->setFont(TableHeaderFont());

   this->horizontalHeader()->setStyleSheet("QHeaderView::section {"
                                           "border-right: 1px solid rgb(193,192,193);"
                                           "border-bottom: 0px;"
                                           "border-top: 0px;}");
}

void DecentTable::sectionClicked(int index)
{
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

         if(cell_widget != NULL) {
            if(DecentSmallButton *button = qobject_cast<DecentSmallButton*>(cell_widget)) {
               button->unhighlight();
            } else {
               QString old_style = cell_widget->property("old_style").toString();

               if (old_style.isEmpty())
                  cell_widget->setStyleSheet("* { background-color: rgb(255,255,255); color : black; }");
               else
                  cell_widget->setStyleSheet(old_style);
            }
         }
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

      if(cell_widget != NULL) {
         if(DecentSmallButton *button = qobject_cast<DecentSmallButton*>(cell_widget)) {
            button->highlight();
         } else {
            cell_widget->setProperty("old_style", cell_widget->styleSheet());
            cell_widget->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");
         }
      }

   }
   _current_highlighted_row = row;
}

}
