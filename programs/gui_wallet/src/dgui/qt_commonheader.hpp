#pragma once

#include <QEvent>
#include <QObject>
#include <stdio.h>
#include <QMouseEvent>
#include "ui_wallet_functions_base.hpp"
#include <QTime>




// DCT stands for Digital Contex Actions
namespace DCT {
    enum DIG_CONT_TYPES {GENERAL, BOUGHT, WAITING_DELIVERY};
}


namespace gui_wallet {
    
    
struct SDigitalContent{
    DCT::DIG_CONT_TYPES  type = DCT::GENERAL;
    std::string          author;
    struct {
        double      amount;
        std::string asset_id;
        std::string symbol;
        std::string precision;
    } price;
   
    std::string   synopsis;
    std::string   URI;
    double        AVG_rating;
    std::string   created;
    std::string   expiration;
    int           size;
    int           times_bougth;
};



}
