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

namespace gui_wallet
{

	void makeWarningImediatly(const char* waringTitle, const char* waringText, const char* details, void* parent );


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
	    void setCurrentUser(const std::string& user) { _currentUser = user; emit }
	
	signals:
	    void currentUserChanged(std::string user);

	};



}

#endif // GUI_WALLET_GLOBAL_HPP
