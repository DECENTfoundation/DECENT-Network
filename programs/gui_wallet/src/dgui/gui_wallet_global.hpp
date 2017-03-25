/*
 *	File: gui_wallet_global.hpp
 *
 *	Created on: 30 Nov, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file declares the global functions,
 *  implemented in the file gui_wallet_global.cpp
 *
 *
 */
#ifndef GUI_WALLET_GLOBAL_HPP
#define GUI_WALLET_GLOBAL_HPP

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


#include <numeric>

#define ALERT(message)                                  \
{                                                       \
QMessageBox msgBox;                                 \
msgBox.setWindowTitle("Error");                     \
msgBox.setText(QString::fromStdString(message));    \
msgBox.exec();                                      \
}                                                       \



#define ALERT_DETAILS(message, details)                                  \
{                                                       \
QMessageBox msgBox;                                 \
msgBox.setWindowTitle("Error");                     \
msgBox.setText(QString::fromStdString(message));    \
msgBox.setDetailedText(QObject::tr(details));    \
msgBox.exec();                                      \
}                                                       \





#define MESSAGE(message)                                  \
{                                                       \
QMessageBox msgBox;                                 \
msgBox.setWindowTitle("Message");                     \
msgBox.setText(QString::fromStdString(message));    \
msgBox.exec();                                      \
}                                                   \


#define DCT_VERIFY(condition) \
{ \
   bool _b_condition_ = (condition); \
   Q_ASSERT(_b_condition_); \
}

namespace gui_wallet
{
    
        std::string CalculateRemainingTime(QDateTime const& dt, QDateTime const& dtFuture);
    
    
        inline std::size_t extra_space(const std::string& s) noexcept
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
    
        inline std::string unescape_string(const std::string& s)
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
    
    
        inline std::string escape_string(const std::string& s)
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
   
   // QLabel with clicked() signal implemented
   
    class DecentSmallButton : public QLabel {
        Q_OBJECT;
    public:
        DecentSmallButton(const QString& normalImg, const QString& highlightedImg ){
            normalImage.load(normalImg);
            highlightedImage.load(highlightedImg);
            this->setPixmap(normalImg);
        }
        
        void unhighlight()
        {
            this->setPixmap(normalImage);
            this->setStyleSheet("* { background-color: rgb(255,255,255); color : black; }");
        }
        
        void highlight()
        {
            this->setPixmap(highlightedImage);
            this->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");
        }
    
    private:
        QPixmap normalImage;
        QPixmap highlightedImage;
        
    signals:
        void clicked();
        
    protected:
        void mousePressEvent(QMouseEvent* event) {
            emit clicked();
        }
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
    
   
   
   
   
   // Table with additional functionality to use in our GUI
   struct DecentColumn {
      std::string title;
      int size; // Negative value of size means absolute value of width, positive is weighted value
   };
   
   class DecentTable : public QTableWidget {
      Q_OBJECT
       
   public:
   signals:
       void MouseWasMoved();
       
   public:
      DecentTable() {
         this->horizontalHeader()->setStretchLastSection(true);
         this->setSelectionMode(QAbstractItemView::NoSelection);
         this->setStyleSheet("QTableView{border : 0px}");
         this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
         this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
         
         this->verticalHeader()->hide();
         this->setMouseTracking(true);
      }
       
       int getCurrentHighlightedRow(){return _current_highlighted_row;}
      
      
      void set_columns(const std::vector<DecentColumn>& cols) {
         _cols = cols;
         
         this->setColumnCount(cols.size());
         
         QFont font("Open Sans Bold", 14, QFont::Bold);
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
         this->horizontalHeader()->setFont(font);
         
         this->horizontalHeader()->setStyleSheet("QHeaderView::section {"
                                                          "border-right: 1px solid rgb(193,192,193);"
                                                          "border-bottom: 0px;"
                                                          "border-top: 0px;}");
      }
      
   private:
      virtual void resizeEvent(QResizeEvent * a_event) {
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

      virtual void mouseMoveEvent(QMouseEvent * event) {
         
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
             emit MouseWasMoved();
            return;
         }
         
         
         for (int i = 0; i < this->columnCount(); ++i) {
            QTableWidgetItem* cell = this->item(row, i);
            QWidget* cell_widget = this->cellWidget(row, i);

            if (cell != NULL) {
               cell->setBackgroundColor(QColor(27,176,104));
               cell->setForeground(QColor::fromRgb(255,255,255));
            }
            
            if(cell_widget != NULL)
                if(DecentSmallButton *button = qobject_cast<DecentSmallButton*>(cell_widget)) {
                    button->highlight();
                } else {
                   cell_widget->setProperty("old_style", cell_widget->styleSheet());
                   cell_widget->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");
                }
            
            
         }
          _current_highlighted_row = row;
      }
       
       

      
   private:
      int                            _current_highlighted_row = -1;
      int                            _sum_weights = 1;
      int                            _sum_absoulte = 0;
      std::vector<DecentColumn>      _cols;
   };




}

#endif // GUI_WALLET_GLOBAL_HPP
