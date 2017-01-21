/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/iostreams/copy.hpp>



#include <graphene/package/package.hpp>

#include <fc/exception/exception.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <iostream>
#include <atomic>

#include <decent/encrypt/encryptionutils.hpp>


#include <libtorrent/alert_types.hpp>


#include "torrent_transfer.hpp"


using namespace graphene::package;
using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;


torrent_transfer::torrent_transfer() {
	_my_thread = new fc::thread("torrent_thread");
}

torrent_transfer::~torrent_transfer() {
	delete _my_thread;
}


void torrent_transfer::upload_package(transfer_id id, const package_object& package, transfer_listener* listener) {
	_id = id;
	_listener = listener;
	_url = "magnet:?xt=blahblah";

	listener->on_upload_started(id, _url);
	listener->on_upload_progress(id, transfer_progress(1000, 200, 100));
	listener->on_upload_progress(id, transfer_progress(1000, 800, 120));
	listener->on_upload_finished(id);

}

void torrent_transfer::download_package(transfer_id id, const std::string& url, transfer_listener* listener) {
	_id = id;
	_listener = listener;
	_url = url;

	libtorrent::add_torrent_params atp;
	atp.url = url.c_str();

	path temp_dir = temp_directory_path();

	atp.save_path = temp_dir.string();
	cout << "Save path is: " << atp.save_path << endl;

	_torrent_handle = _session.add_torrent(atp);

	_session.set_alert_notify([this]() {

		_my_thread->async([this] () {
        	this->handle_torrent_alerts();           
        });

    });


//	listener->on_download_started(id);
//	listener->on_download_progress(id, transfer_progress(1000, 200, 100));
//	listener->on_download_progress(id, transfer_progress(1000, 800, 120));
	
//	listener->on_download_finished(id, package_manager::instance().get_package_object(fc::ripemd160("22ad84efeca3776a4e37b738eab728abcedc92d8")));
}

void torrent_transfer::update_torrent_status() {
	libtorrent::torrent_status st = _torrent_handle.status();

	bool is_finished = st.state == libtorrent::torrent_status::finished || st.state == libtorrent::torrent_status::seeding;
	bool is_error = st.errc != 0;

	if (!is_finished && !is_error) {
		_my_thread->schedule([this] () {
        	this->update_torrent_status();
        }, fc::time_point::now() + fc::seconds(5));
	}
	if (is_error) {
		_listener->on_error(_id, st.errc.message());
	} else {
		_listener->on_download_progress(_id, transfer_progress(st.total_wanted, st.total_wanted_done, st.download_rate));
	}
}


void torrent_transfer::handle_torrent_alerts() {
	
	std::vector<libtorrent::alert*> alerts;
	_session.pop_alerts(&alerts);

	for (int i = 0; i < alerts.size(); ++i) {
		libtorrent::alert* alert = alerts[i];

		switch (alert->type()) {
			case libtorrent::torrent_added_alert::alert_type:
				_listener->on_download_started(_id);

				_my_thread->schedule([this] () {
		        	this->update_torrent_status();           
		        }, fc::time_point::now() + fc::seconds(5));
				break;

			case libtorrent::torrent_finished_alert::alert_type:
            	_listener->on_download_finished(_id, package_manager::instance().get_package_object(fc::ripemd160("22ad84efeca3776a4e37b738eab728abcedc92d8")));
				break;

			//case libtorrent::tracker_error_alert::alert_type:
			//case libtorrent::peer_error_alert::alert_type:
			case libtorrent::file_error_alert::alert_type:
			//case libtorrent::udp_error_alert::alert_type:
			//case libtorrent::portmap_error_alert::alert_type:
			case libtorrent::torrent_error_alert::alert_type:
			//case libtorrent::dht_error_alert::alert_type:
			//case libtorrent::lsd_error_alert::alert_type:
            	_listener->on_error(_id, alert->message()); 
            	break;

			
		}
		
		//cout << "Torrent alert: " << alert->message() << endl;
	}
}



std::string torrent_transfer::get_transfer_url(transfer_id id) {
	return _url;
}

torrent_transfer::transfer_progress torrent_transfer::get_transfer_progress(transfer_id id) {
	return transfer_progress(1000, 800, 120);
}



