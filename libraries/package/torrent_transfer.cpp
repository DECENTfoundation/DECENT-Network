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

#include "torrent_transfer.hpp"

#include <decent/encrypt/encryptionutils.hpp>
#include <graphene/package/package.hpp>

#include <fc/exception/exception.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>

#include <libtorrent/alert_types.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>
//#include <libtorrent/extensions/metadata_transfer.hpp>
#include <libtorrent/extensions/ut_metadata.hpp>
#include <libtorrent/extensions/ut_pex.hpp>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <atomic>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;
using namespace libtorrent;
using namespace graphene::package;


namespace {

	int load_file(std::string const& filename, std::vector<char>& v, libtorrent::error_code& ec, int limit = 8000000) {
		ec.clear();
		FILE* f = std::fopen(filename.c_str(), "rb");
		if (f == nullptr) {
			ec.assign(errno, boost::system::system_category());
			return -1;
		}

		int r = fseek(f, 0, SEEK_END);
		if (r != 0) {
			ec.assign(errno, boost::system::system_category());
			std::fclose(f);
			return -1;
		}

		long s = ftell(f);
		if (s < 0) {
			ec.assign(errno, boost::system::system_category());
			std::fclose(f);
			return -1;
		}

		if (s > limit) {
			std::fclose(f);
			return -2;
		}

		r = fseek(f, 0, SEEK_SET);
		if (r != 0) {
			ec.assign(errno, boost::system::system_category());
			std::fclose(f);
			return -1;
		}

		v.resize(s);
		if (s == 0) {
			std::fclose(f);
			return 0;
		}

		r = int(std::fread(&v[0], 1, v.size(), f));
		if (r < 0) {
			ec.assign(errno, boost::system::system_category());
			std::fclose(f);
			return -1;
		}

		std::fclose(f);

		if (r != s) return -3;
		return 0;
	}

	int save_file(std::string const& filename, std::vector<char>& v) {
		FILE* f = std::fopen(filename.c_str(), "wb");
		if (f == nullptr)
			return -1;

		int w = int(std::fwrite(&v[0], 1, v.size(), f));
		std::fclose(f);

		if (w < 0) return -1;
		if (w != int(v.size())) return -3;
		return 0;
	}
}

torrent_transfer::torrent_transfer(const torrent_transfer& orig)
    : _thread(orig._thread)
    , _session_mutex(orig._session_mutex)
    , _session(orig._session)
    , _lifetime_info_mutex(std::make_shared<fc::mutex>())
    , _instance_exists(std::make_shared<std::atomic<bool>>(true))
    , _default_dht_nodes(orig._default_dht_nodes)
    , _default_trackers(orig._default_trackers)
{
    if (!_thread)         FC_THROW("Thread instance is not available");
    if (!_session_mutex)  FC_THROW("Session mutex instance is not available");
    if (!_session)        FC_THROW("Session instance is not available");
}

torrent_transfer::torrent_transfer()
    : _lifetime_info_mutex(std::make_shared<fc::mutex>())
    , _instance_exists(std::make_shared<std::atomic<bool>>(true))
{
    _default_dht_nodes.push_back(std::make_pair("dht.transmissionbt.com", 6881));
    _default_dht_nodes.push_back(std::make_pair("router.utorrent.com", 6881));
    _default_dht_nodes.push_back(std::make_pair("router.bittorrent.com", 6881));
    _default_dht_nodes.push_back(std::make_pair("router.bitcomet.com", 6881));
    _default_dht_nodes.push_back(std::make_pair("dht.aelitis.com", 6881));
    _default_dht_nodes.push_back(std::make_pair("dht.libtorrent.org", 25401));
    _default_dht_nodes.push_back(std::make_pair("bootstrap.ring.cx", 4222));

//  _default_trackers.push_back("http://9.rarbg.com:2710/announce");
//  _default_trackers.push_back("udp://tracker.opentrackr.org:1337");
//  _default_trackers.push_back("udp://tracker.coppersurfer.tk:6969");
//  _default_trackers.push_back("udp://tracker.leechers-paradise.org:6969");
//  _default_trackers.push_back("udp://zer0day.ch:1337");
//  _default_trackers.push_back("udp://explodie.org:6969");

    _thread = std::make_shared<fc::thread>("torrent_thread");
    _session_mutex = std::make_shared<fc::mutex>();

    libtorrent::session_params p;

    p.settings.set_str(libtorrent::settings_pack::user_agent, "Decent/0.1.0");
    p.settings.set_str(libtorrent::settings_pack::peer_fingerprint, "-dst010-");
    p.settings.set_bool(libtorrent::settings_pack::allow_multiple_connections_per_ip, true);
    p.settings.set_bool(libtorrent::settings_pack::announce_to_all_tiers, true);
    p.settings.set_bool(libtorrent::settings_pack::announce_to_all_trackers, true);
    p.settings.set_bool(libtorrent::settings_pack::prefer_udp_trackers, false);
    p.settings.set_bool(libtorrent::settings_pack::incoming_starts_queued_torrents, false);
    p.settings.set_bool(libtorrent::settings_pack::lock_files, true);
    p.settings.set_bool(libtorrent::settings_pack::enable_upnp, true);
    p.settings.set_bool(libtorrent::settings_pack::enable_natpmp, true);
    p.settings.set_bool(libtorrent::settings_pack::enable_lsd, true);
    p.settings.set_bool(libtorrent::settings_pack::enable_dht, true);
    p.settings.set_int(libtorrent::settings_pack::dht_announce_interval, 15);
    p.settings.set_int(libtorrent::settings_pack::dht_upload_rate_limit, 10000);
    p.settings.set_int(libtorrent::settings_pack::active_limit, 100);
    p.settings.set_int(libtorrent::settings_pack::active_seeds, 90);
    p.settings.set_int(libtorrent::settings_pack::active_downloads, 10);

    std::string orig_dht_bootstrap_nodes = p.settings.get_str(libtorrent::settings_pack::dht_bootstrap_nodes);
    std::string dht_bootstrap_nodes;
    for (auto dht_node : _default_dht_nodes) {
        if (!dht_bootstrap_nodes.empty())
            dht_bootstrap_nodes += ',';
        dht_bootstrap_nodes += dht_node.first;
        dht_bootstrap_nodes += ':';
        dht_bootstrap_nodes += boost::lexical_cast<std::string>(dht_node.second);
    }
    if (!dht_bootstrap_nodes.empty())
        dht_bootstrap_nodes += ',';
    dht_bootstrap_nodes += orig_dht_bootstrap_nodes;
    p.settings.set_str(libtorrent::settings_pack::dht_bootstrap_nodes, dht_bootstrap_nodes);

    p.dht_settings.search_branching = 10;
    p.dht_settings.max_torrent_search_reply = 100;
    p.dht_settings.restrict_routing_ips = false;
    p.dht_settings.restrict_search_ips = false;
    p.dht_settings.ignore_dark_internet = false;

    fc::scoped_lock<fc::mutex> guard(*_session_mutex);

    _session = std::make_shared<libtorrent::session>(p);

//  _session->add_extension(&libtorrent::create_metadata_plugin);
    _session->add_extension(&libtorrent::create_ut_metadata_plugin);
    _session->add_extension(&libtorrent::create_ut_pex_plugin);

    libtorrent::error_code ec;

    std::vector<char> in;
    if (load_file(".ses_state", in, ec) == 0) {
        bdecode_node e;
        if (bdecode(&in[0], &in[0] + in.size(), e, ec) == 0)
            _session->load_state(e, session::save_dht_state);
    }

    auto lifetime_info_mutex = _lifetime_info_mutex;
    auto instance_exists = _instance_exists;
    _session->set_alert_notify([this, lifetime_info_mutex, instance_exists]() {

        _thread->async([this, lifetime_info_mutex, instance_exists] () {

            fc::scoped_lock<fc::mutex> guard(*lifetime_info_mutex);
            if (*instance_exists) {
                this->handle_torrent_alerts();
            }

        });

    });
}

torrent_transfer::~torrent_transfer() {
    fc::scoped_lock<fc::mutex> guard(*_lifetime_info_mutex);

    if (_session.use_count() == 1)
    {
        entry session_state;
        _session->save_state(session_state, session::save_dht_state);

        std::vector<char> out;
        bencode(std::back_inserter(out), session_state);
        save_file(".ses_state", out);
    }

//  _lifetime_info_mutex->unlock();
//  _lifetime_info_mutex->lock();

    *_instance_exists = false;
}



struct upload_torrent_data {
    std::string creator     = "Decent";
    int piece_size          = 0;
    bool priv               = false;
    bool upload_mode        = true;
    bool super_seeding_mode = false;
    bool share_mode         = false;
    bool auto_managed       = true;
    bool announce_on_add    = true;
    bool scrape_on_add      = true;
    int max_uploads         = -1;
    int max_connections     = -1;
    int upload_limit        = -1;
    int download_limit      = -1;
    std::vector<std::string>                 url_seeds;
    std::vector<std::string>                 http_seeds;
    std::vector<std::pair<std::string, int>> dht_nodes{
        {"dht.transmissionbt.com", 6881},
        {"router.utorrent.com",    6881},
        {"router.bittorrent.com",  6881},
        {"router.bitcomet.com",    6881},
        {"dht.aelitis.com",        6881},
        {"dht.libtorrent.org",     25401}
    };
    std::vector<std::string>                 trackers{
//      "udp://tracker.opentrackr.org:1337"
    };
};

struct download_torrent_data {
    bool upload_mode        = true;
    bool share_mode         = false;
    bool auto_managed       = true;
    bool announce_on_add    = true;
    bool scrape_on_add      = true;
    int max_uploads         = -1;
    int max_connections     = -1;
    int upload_limit        = -1;
    int download_limit      = -1;
    std::vector<std::string>                 url_seeds;
    std::vector<std::string>                 http_seeds;
    std::vector<std::pair<std::string, int>> dht_nodes{
        {"dht.transmissionbt.com", 6881},
        {"router.utorrent.com",    6881},
        {"router.bittorrent.com",  6881},
        {"router.bitcomet.com",    6881},
        {"dht.aelitis.com",        6881},
        {"dht.libtorrent.org",     25401}
    };
    std::vector<std::string>                 trackers{
//      "udp://tracker.opentrackr.org:1337"
    };
};

struct libtorrent_config_data
{
    std::map<std::string, std::string> settings;
    libtorrent::dht_settings dht_settings;
    upload_torrent_data upload_torrent;
    download_torrent_data download_torrent;
};

FC_REFLECT( upload_torrent_data,
            (creator)
            (piece_size)
            (priv)
            (upload_mode)
            (super_seeding_mode)
            (share_mode)
            (auto_managed)
            (announce_on_add)
            (scrape_on_add)
            (max_uploads)
            (max_connections)
            (upload_limit)
            (download_limit)
            (url_seeds)
            (http_seeds)
            (dht_nodes)
            (trackers)
          )

FC_REFLECT( download_torrent_data,
            (upload_mode)
            (share_mode)
            (auto_managed)
            (announce_on_add)
            (scrape_on_add)
            (max_uploads)
            (max_connections)
            (upload_limit)
            (download_limit)
            (url_seeds)
            (http_seeds)
            (dht_nodes)
            (trackers)
          )

FC_REFLECT( libtorrent::dht_settings,
            (max_peers_reply)
            (search_branching)
            (max_fail_count)
            (max_torrents)
            (max_dht_items)
            (max_peers)
            (max_torrent_search_reply)
            (restrict_routing_ips)
            (restrict_search_ips)
            (extended_routing_table)
            (aggressive_lookups)
            (privacy_lookups)
            (enforce_node_id)
            (ignore_dark_internet)
            (block_timeout)
            (block_ratelimit)
            (read_only)
            (item_lifetime)
            (upload_rate_limit)
          )

FC_REFLECT( libtorrent_config_data,
            (settings)
            (dht_settings)
            (upload_torrent)
            (download_torrent)
          )


void torrent_transfer::reconfigure(const boost::filesystem::path& config_file) {
    if (!boost::filesystem::exists(config_file)) {
        FC_THROW("Unable to read libtorrent config file ${file}: file does not exists", ("file", config_file.string()) );
    }



    libtorrent_config_data lcdata = fc::json::from_file(config_file, fc::json::strict_parser).as<libtorrent_config_data>();



    // TODO: actual reconfigure
}

void torrent_transfer::dump_config(const boost::filesystem::path& config_file) {
    libtorrent_config_data lcdata;

    wlog("saving current libtorrent config to file ${fn}", ("fn", config_file.string()) );

    std::string data = fc::json::to_pretty_string(lcdata);
    fc::ofstream outfile{config_file};
    outfile.write(data.c_str(), data.length());
}

void torrent_transfer::print_status() {
	libtorrent::torrent_status st = _torrent_handle.status();
	cout << "Error Message/String/File: " << st.errc.message() << " / " << st.error << " / " << st.error_file << endl;
	cout << "Save path/Name/Current Tracker: " << st.save_path << " / " << st.name << " / " << st.current_tracker << endl;
	cout << "Total download/upload/payload download/payload upload: " << st.total_download << " / " << st.total_upload << " / " << st.total_payload_download << " / " << st.total_payload_upload << endl;
	cout << "Download rate/Upload rate/Seeds/Peers: " << st.download_rate << " / " << st.upload_rate << " / " << st.num_seeds << " / " << st.num_peers << endl;
	cout << "Distributed Full Copies/Distributed Fraction/Distributed Copies: " << st.distributed_full_copies << " / " << st.distributed_fraction << " / " << st.distributed_copies << endl;
	cout << "Active Time/Finished Time/Seeding Time: " << st.active_time  << " / " << st.finished_time << " / " << st.seeding_time << endl;
	cout << "Num Uploads/Num Connections/Uploads Limit/Connections Limit: " << st.num_uploads << " / " << st.num_connections << " / " << st.uploads_limit << " / " << st.connections_limit << endl;
	cout << "Next announce/Progress/State: " << st.next_announce.count() << " / " << st.progress << "/";

	switch (st.state) {
		case torrent_status::checking_files:
			cout << "checking_files" << endl;
			break;
		case torrent_status::downloading_metadata:
			cout << "downloading_metadata" << endl;
			break;
		case torrent_status::downloading:
			cout << "downloading" << endl;
			break;
		case torrent_status::finished:
			cout << "finished" << endl;
			break;
		case torrent_status::seeding:
			cout << "seeding" << endl;
			break;
		case torrent_status::allocating:
			cout << "allocating" << endl;
			break;
		case torrent_status::checking_resume_data:
			cout << "checking_resume_data" << endl;
			break;
		case torrent_status::queued_for_checking:
			cout << "queued_for_checking" << endl;
			break;
	}

	cout << "Upload Mode/Share Mode/Super Seeding/Auto Managed: " << st.upload_mode << " / " << st.share_mode << " / " << st.super_seeding << " / " << st.auto_managed << endl;
	cout << "Paused/Seeding?/Finished?/Loaded?: " << st.paused << " / " << st.is_seeding << " / " << st.is_finished << " / " << st.is_loaded << endl;
	cout << "Has Metadata/Has Incoming/Stop When Ready: " << st.has_metadata << " / " << st.has_incoming << " / " << st.stop_when_ready << endl;
	cout << "Announcing To Trackers/LSD/DHT: " << st.announcing_to_trackers << " / " << st.announcing_to_lsd << " / " << st.announcing_to_dht << endl;
	cout << "Seed Mode/Seed Rank: " << st.seed_mode << " / " << st.seed_rank << endl;
	cout << "Block Size/Num Pieces: " << st.block_size << " / " << st.num_pieces<< endl;
}

void torrent_transfer::upload_package(transfer_id id, const package_object& package, transfer_listener* listener) {
    _id = id;
    _listener = listener;
    _is_upload = true;

    auto lifetime_info_mutex = _lifetime_info_mutex;
    auto instance_exists = _instance_exists;
    _thread->async([this, package, lifetime_info_mutex, instance_exists] () {

        fc::scoped_lock<fc::mutex> guard(*lifetime_info_mutex);
        if (*instance_exists) {
            path log_path = package_manager::instance().get_packages_path() / "transfer.log";
            this->_transfer_log.open(log_path.string(), std::ios::out | std::ios::app);
            this->_transfer_log << "***** Torrent upload started for package: " << package.get_hash().str() << endl;
        }

    });

    file_storage fs;
    libtorrent::error_code ec;

    // recursively adds files in directories
    add_files(fs, package.get_path().string());

    create_torrent t(fs, 5 * 64 * 16 * 1024); // 5MB pieces
    t.set_creator("Decent");
    t.set_priv(false);

    // reads the files and calculates the hashes
    set_piece_hashes(t, package.get_path().parent_path().string(), ec);

    if (ec) {
        listener->on_error(_id, ec.message());
        return;
    }

    path temp_file = temp_directory_path() / (package.get_hash().str() + ".torrent");
    path temp_dir = temp_directory_path();

    cout << "Torrent file created: " << temp_file.string() << endl;

    std::ofstream out(temp_file.string(), std::ios_base::binary);
    bencode(std::ostream_iterator<char>(out), t.generate());
    out.close();

    libtorrent::add_torrent_params atp;

    atp.ti = std::make_shared<libtorrent::torrent_info>(temp_file.string(), 0);
    atp.flags = libtorrent::add_torrent_params::flag_seed_mode
              | libtorrent::add_torrent_params::flag_upload_mode
//            | libtorrent::add_torrent_params::flag_share_mode
//            | libtorrent::add_torrent_params::flag_super_seeding
              | libtorrent::add_torrent_params::flag_merge_resume_http_seeds
              | libtorrent::add_torrent_params::flag_merge_resume_trackers
              ;
    atp.save_path = package.get_path().parent_path().string();

    for (auto dht_node : _default_dht_nodes) {
        atp.dht_nodes.push_back(dht_node);
        t.add_node(dht_node);
    }

    for (auto tracker : _default_trackers) {
        atp.trackers.push_back(tracker);
        t.add_tracker(tracker);
    }

//  int num_pieces = atp.ti->num_pieces();

    {
        fc::scoped_lock<fc::mutex> guard(*_session_mutex);
        _torrent_handle = _session->add_torrent(atp);
    }

    _torrent_handle.set_upload_mode(true);
    _torrent_handle.super_seeding(false);
    _torrent_handle.force_dht_announce();
    _url = make_magnet_uri(_torrent_handle);

    _thread->async([this, lifetime_info_mutex, instance_exists] () {

        fc::scoped_lock<fc::mutex> guard(*lifetime_info_mutex);
        if (*instance_exists) {
            this->update_torrent_status();
            _listener->on_upload_started(_id, _url);
        }

    });
}

void torrent_transfer::download_package(transfer_id id, const std::string& url, transfer_listener* listener) {
    _id = id;
    _listener = listener;
    _url = url;
    _is_upload = false;

    auto lifetime_info_mutex = _lifetime_info_mutex;
    auto instance_exists = _instance_exists;
    _thread->async([this, url, lifetime_info_mutex, instance_exists] () {

        fc::scoped_lock<fc::mutex> guard(*lifetime_info_mutex);
        if (*instance_exists) {
            path log_path = package_manager::instance().get_packages_path() / "transfer.log";
            this->_transfer_log.open(log_path.string(), std::ios::out | std::ios::app);
            this->_transfer_log << "***** Torrent download started from url: " << url << endl;
        }

    });

    libtorrent::add_torrent_params atp;

    atp.url = url;
    atp.flags = libtorrent::add_torrent_params::flag_merge_resume_http_seeds
              | libtorrent::add_torrent_params::flag_merge_resume_trackers
    ;
    atp.save_path = temp_directory_path().string();

    for (auto dht_node : _default_dht_nodes) {
        atp.dht_nodes.push_back(dht_node);
    }

    for (auto tracker : _default_trackers) {
        atp.trackers.push_back(tracker);
    }

    cout << "Save path is: " << atp.save_path << endl;

    {
        fc::scoped_lock<fc::mutex> guard(*_session_mutex);
        _torrent_handle = _session->add_torrent(atp);
    }

    _thread->async([this, lifetime_info_mutex, instance_exists] () {

        fc::scoped_lock<fc::mutex> guard(*lifetime_info_mutex);
        if (*instance_exists) {
            this->update_torrent_status();
            _listener->on_download_started(_id);
        }

    });
}

void torrent_transfer::update_torrent_status() {
    fc::scoped_lock<fc::mutex> guard(*_session_mutex);

	_session->post_torrent_updates();
	_session->post_session_stats();
	_session->post_dht_stats();
	_session->post_torrent_updates();

	libtorrent::torrent_status st = _torrent_handle.status();

	bool is_finished = st.state == libtorrent::torrent_status::finished || st.state == libtorrent::torrent_status::seeding;
	bool is_error = st.errc != 0;


	if (is_error) {
		_listener->on_error(_id, st.errc.message());
	} else {
		if (!_is_upload && st.total_wanted != 0) {
            
			if (st.total_wanted_done < st.total_wanted) {
				_listener->on_download_progress(_id, transfer_progress(st.total_wanted, st.total_wanted_done, st.download_rate));
			} else {
				package::package_object obj = check_and_install_package();
				if (obj.verify_hash()) {
					_listener->on_download_finished(_id, obj);
				} else {
					_listener->on_error(_id, "Unable to verify package hash");
				}
			}

		}
	}
    
    if (!is_error) {
        _thread->schedule([this] () {
            this->update_torrent_status();
        }, fc::time_point::now() + fc::seconds(5));
    }
}


package_object torrent_transfer::check_and_install_package() {
	torrent_status st = _torrent_handle.status(torrent_handle::query_save_path | torrent_handle::query_name);
    package::package_object obj(path(st.save_path) / st.name);

    if (!obj.verify_hash()) {
    	FC_THROW("Unable to verify downloaded package");    
    }

    rename(path(st.save_path) / st.name, package_manager::instance().get_packages_path() / st.name);
    return package::package_object(package_manager::instance().get_packages_path() / st.name);
}


void torrent_transfer::handle_torrent_alerts() {
	fc::scoped_lock<fc::mutex> guard(*_session_mutex);

	std::vector<libtorrent::alert*> alerts;
	_session->pop_alerts(&alerts);


	path log_path = package_manager::instance().get_packages_path() / "transfer.log";
	_transfer_log.open(log_path.string(), std::ios::out | std::ios::app);

	for (int i = 0; i < alerts.size(); ++i) {
		libtorrent::alert* alert = alerts[i];
		
		_transfer_log << "[";

		int cat = alert->category();

		if (cat & libtorrent::alert::error_notification) _transfer_log << " ERROR";
		if (cat & libtorrent::alert::peer_notification) _transfer_log << " PEER";
		if (cat & libtorrent::alert::port_mapping_notification) _transfer_log << " PORT_MAP";
		if (cat & libtorrent::alert::storage_notification) _transfer_log << " STORAGE";
		if (cat & libtorrent::alert::tracker_notification) _transfer_log << " TRACKER";
		if (cat & libtorrent::alert::debug_notification) _transfer_log << " DEBUG";
		if (cat & libtorrent::alert::status_notification) _transfer_log << " STATUS";
		if (cat & libtorrent::alert::progress_notification) _transfer_log << " PROGRESS";
		if (cat & libtorrent::alert::ip_block_notification) _transfer_log << " IPBLOCK";
		if (cat & libtorrent::alert::performance_warning) _transfer_log << " PERFORMANCE";
		if (cat & libtorrent::alert::dht_notification) _transfer_log << " DHT";
		if (cat & libtorrent::alert::stats_notification) _transfer_log << " STATS";
		if (cat & libtorrent::alert::session_log_notification) _transfer_log << " SESSION";
		if (cat & libtorrent::alert::torrent_log_notification) _transfer_log << " TORRENT";
		if (cat & libtorrent::alert::peer_log_notification) _transfer_log << " PEERL";
		if (cat & libtorrent::alert::incoming_request_notification) _transfer_log << " INREQ";
		if (cat & libtorrent::alert::dht_log_notification) _transfer_log << " DHT_L";
		if (cat & libtorrent::alert::dht_operation_notification) _transfer_log << " DHT_OP";
		if (cat & libtorrent::alert::port_mapping_log_notification) _transfer_log << " PORM_MAPL";
		if (cat & libtorrent::alert::picker_log_notification) _transfer_log << " PICKERL";

		_transfer_log << "] ~> [" << alert->what() << "]: " << alert->message() << endl;
	}
	_transfer_log.close();
}

std::string torrent_transfer::get_transfer_url(transfer_id id) {
	return _url;
}
