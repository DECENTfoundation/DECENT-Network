
#include "torrent_transfer.hpp"

#include <decent/encrypt/encryptionutils.hpp>
#include <graphene/package/package.hpp>
#include <graphene/utilities/dirhelper.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/variant.hpp>

#include <libtorrent/alert_types.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_info.hpp>
//#include <libtorrent/extensions/metadata_transfer.hpp>
#include <libtorrent/extensions/ut_metadata.hpp>
#include <libtorrent/extensions/ut_pex.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <atomic>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;
using namespace libtorrent;
using namespace graphene::package;
using namespace graphene::utilities;


namespace {

    int load_file(const fc::path& filename, std::vector<char>& v, libtorrent::error_code& ec, int limit = 8000000) {
		ec.clear();
		FILE* f = std::fopen(filename.string().c_str(), "rb");
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

    int save_file(const fc::path& filename, std::vector<char>& v) {
		FILE* f = std::fopen(filename.string().c_str(), "wb");
		if (f == nullptr)
			return -1;

		int w = int(std::fwrite(&v[0], 1, v.size(), f));
		std::fclose(f);

		if (w < 0) return -1;
		if (w != int(v.size())) return -3;
		return 0;
	}

    void to_settings_pack(const std::map<std::string, fc::variant>& settings_map, libtorrent::settings_pack& settings_pack) {

#define WRITE_SETTING_VAL(sp, sm, name, tp, vtp) if (sm.count(#name)) sp.set_##tp(libtorrent::settings_pack::name, sm.at(#name).as_##vtp());

#define WRITE_SETTING_STR(name)   WRITE_SETTING_VAL(settings_pack, settings_map, name, str, string)
#define WRITE_SETTING_BOOL(name)  WRITE_SETTING_VAL(settings_pack, settings_map, name, bool, bool)
#define WRITE_SETTING_INT(name)   WRITE_SETTING_VAL(settings_pack, settings_map, name, int, int64)

        WRITE_SETTING_STR(user_agent)
        WRITE_SETTING_STR(announce_ip)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_STR(mmap_cache)
#else
        WRITE_SETTING_STR(deprecated21)
#endif
        WRITE_SETTING_STR(handshake_client_version)
        WRITE_SETTING_STR(outgoing_interfaces)
        WRITE_SETTING_STR(listen_interfaces)
        WRITE_SETTING_STR(proxy_hostname)
        WRITE_SETTING_STR(proxy_username)
        WRITE_SETTING_STR(proxy_password)
        WRITE_SETTING_STR(i2p_hostname)
        WRITE_SETTING_STR(peer_fingerprint)
        WRITE_SETTING_STR(dht_bootstrap_nodes)

        WRITE_SETTING_BOOL(allow_multiple_connections_per_ip)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(ignore_limits_on_local_network)
#else
        WRITE_SETTING_BOOL(deprecated1)
#endif
        WRITE_SETTING_BOOL(send_redundant_have)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(lazy_bitfields)
#else
        WRITE_SETTING_BOOL(deprecated12)
#endif
        WRITE_SETTING_BOOL(use_dht_as_fallback)
        WRITE_SETTING_BOOL(upnp_ignore_nonrouters)
        WRITE_SETTING_BOOL(use_parole_mode)
        WRITE_SETTING_BOOL(use_read_cache)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(use_write_cache)
        WRITE_SETTING_BOOL(dont_flush_write_cache)
#else
        WRITE_SETTING_BOOL(deprecated11)
        WRITE_SETTING_BOOL(deprecated22)
#endif
        WRITE_SETTING_BOOL(coalesce_reads)
        WRITE_SETTING_BOOL(coalesce_writes)
        WRITE_SETTING_BOOL(auto_manage_prefer_seeds)
        WRITE_SETTING_BOOL(dont_count_slow_torrents)
        WRITE_SETTING_BOOL(close_redundant_connections)
        WRITE_SETTING_BOOL(prioritize_partial_pieces)
        WRITE_SETTING_BOOL(rate_limit_ip_overhead)
        WRITE_SETTING_BOOL(announce_to_all_tiers)
        WRITE_SETTING_BOOL(announce_to_all_trackers)
        WRITE_SETTING_BOOL(prefer_udp_trackers)
        WRITE_SETTING_BOOL(strict_super_seeding)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(lock_disk_cache)
#else
        WRITE_SETTING_BOOL(deprecated10)
#endif
        WRITE_SETTING_BOOL(disable_hash_checks)
        WRITE_SETTING_BOOL(allow_i2p_mixed)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(low_prio_disk)
#else
        WRITE_SETTING_BOOL(deprecated17)
#endif
        WRITE_SETTING_BOOL(volatile_read_cache)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(guided_read_cache)
#else
        WRITE_SETTING_BOOL(deprecated13)
#endif
        WRITE_SETTING_BOOL(no_atime_storage)
        WRITE_SETTING_BOOL(incoming_starts_queued_torrents)
        WRITE_SETTING_BOOL(report_true_downloaded)
        WRITE_SETTING_BOOL(strict_end_game_mode)
        WRITE_SETTING_BOOL(broadcast_lsd)
        WRITE_SETTING_BOOL(enable_outgoing_utp)
        WRITE_SETTING_BOOL(enable_incoming_utp)
        WRITE_SETTING_BOOL(enable_outgoing_tcp)
        WRITE_SETTING_BOOL(enable_incoming_tcp)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(ignore_resume_timestamps)
#else
        WRITE_SETTING_BOOL(deprecated8)
#endif
        WRITE_SETTING_BOOL(no_recheck_incomplete_resume)
        WRITE_SETTING_BOOL(anonymous_mode)
        WRITE_SETTING_BOOL(report_web_seed_downloads)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(rate_limit_utp)
#else
        WRITE_SETTING_BOOL(deprecated2)
#endif
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(announce_double_nat)
#else
        WRITE_SETTING_BOOL(deprecated18)
#endif
        WRITE_SETTING_BOOL(seeding_outgoing_connections)
        WRITE_SETTING_BOOL(no_connect_privileged_ports)
        WRITE_SETTING_BOOL(smooth_connects)
        WRITE_SETTING_BOOL(always_send_user_agent)
        WRITE_SETTING_BOOL(apply_ip_filter_to_trackers)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(use_disk_read_ahead)
#else
        WRITE_SETTING_BOOL(deprecated19)
#endif
        WRITE_SETTING_BOOL(lock_files)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_BOOL(contiguous_recv_buffer)
#else
        WRITE_SETTING_BOOL(deprecated15)
#endif
        WRITE_SETTING_BOOL(ban_web_seeds)
        WRITE_SETTING_BOOL(allow_partial_disk_writes)
        WRITE_SETTING_BOOL(force_proxy)
        WRITE_SETTING_BOOL(support_share_mode)
        WRITE_SETTING_BOOL(support_merkle_torrents)
        WRITE_SETTING_BOOL(report_redundant_bytes)
        WRITE_SETTING_BOOL(listen_system_port_fallback)
        WRITE_SETTING_BOOL(use_disk_cache_pool)
        WRITE_SETTING_BOOL(announce_crypto_support)
        WRITE_SETTING_BOOL(enable_upnp)
        WRITE_SETTING_BOOL(enable_natpmp)
        WRITE_SETTING_BOOL(enable_lsd)
        WRITE_SETTING_BOOL(enable_dht)
        WRITE_SETTING_BOOL(prefer_rc4)
        WRITE_SETTING_BOOL(proxy_hostnames)
        WRITE_SETTING_BOOL(proxy_peer_connections)
        WRITE_SETTING_BOOL(auto_sequential)
        WRITE_SETTING_BOOL(proxy_tracker_connections)

        WRITE_SETTING_INT(tracker_completion_timeout)
        WRITE_SETTING_INT(tracker_receive_timeout)
        WRITE_SETTING_INT(stop_tracker_timeout)
        WRITE_SETTING_INT(tracker_maximum_response_length)
        WRITE_SETTING_INT(piece_timeout)
        WRITE_SETTING_INT(request_timeout)
        WRITE_SETTING_INT(request_queue_time)
        WRITE_SETTING_INT(max_allowed_in_request_queue)
        WRITE_SETTING_INT(max_out_request_queue)
        WRITE_SETTING_INT(whole_pieces_threshold)
        WRITE_SETTING_INT(peer_timeout)
        WRITE_SETTING_INT(urlseed_timeout)
        WRITE_SETTING_INT(urlseed_pipeline_size)
        WRITE_SETTING_INT(urlseed_max_request_bytes)
        WRITE_SETTING_INT(urlseed_wait_retry)
        WRITE_SETTING_INT(file_pool_size)
        WRITE_SETTING_INT(max_failcount)
        WRITE_SETTING_INT(min_reconnect_time)
        WRITE_SETTING_INT(peer_connect_timeout)
        WRITE_SETTING_INT(connection_speed)
        WRITE_SETTING_INT(inactivity_timeout)
        WRITE_SETTING_INT(unchoke_interval)
        WRITE_SETTING_INT(optimistic_unchoke_interval)
        WRITE_SETTING_INT(num_want)
        WRITE_SETTING_INT(initial_picker_threshold)
        WRITE_SETTING_INT(allowed_fast_set_size)
        WRITE_SETTING_INT(suggest_mode)
        WRITE_SETTING_INT(max_queued_disk_bytes)
        WRITE_SETTING_INT(handshake_timeout)
        WRITE_SETTING_INT(send_buffer_low_watermark)
        WRITE_SETTING_INT(send_buffer_watermark)
        WRITE_SETTING_INT(send_buffer_watermark_factor)
        WRITE_SETTING_INT(choking_algorithm)
        WRITE_SETTING_INT(seed_choking_algorithm)
        WRITE_SETTING_INT(cache_size)
        WRITE_SETTING_INT(cache_buffer_chunk_size)
        WRITE_SETTING_INT(cache_expiry)
        WRITE_SETTING_INT(disk_io_write_mode)
        WRITE_SETTING_INT(disk_io_read_mode)
        WRITE_SETTING_INT(outgoing_port)
        WRITE_SETTING_INT(num_outgoing_ports)
        WRITE_SETTING_INT(peer_tos)
        WRITE_SETTING_INT(active_downloads)
        WRITE_SETTING_INT(active_seeds)
        WRITE_SETTING_INT(active_checking)
        WRITE_SETTING_INT(active_dht_limit)
        WRITE_SETTING_INT(active_tracker_limit)
        WRITE_SETTING_INT(active_lsd_limit)
        WRITE_SETTING_INT(active_limit)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_INT(active_loaded_limit)
#else
        WRITE_SETTING_INT(deprecated20)
#endif
        WRITE_SETTING_INT(auto_manage_interval)
        WRITE_SETTING_INT(seed_time_limit)
        WRITE_SETTING_INT(auto_scrape_interval)
        WRITE_SETTING_INT(auto_scrape_min_interval)
        WRITE_SETTING_INT(max_peerlist_size)
        WRITE_SETTING_INT(max_paused_peerlist_size)
        WRITE_SETTING_INT(min_announce_interval)
        WRITE_SETTING_INT(auto_manage_startup)
        WRITE_SETTING_INT(seeding_piece_quota)
        WRITE_SETTING_INT(max_rejects)
        WRITE_SETTING_INT(recv_socket_buffer_size)
        WRITE_SETTING_INT(send_socket_buffer_size)
        WRITE_SETTING_INT(max_peer_recv_buffer_size)
        WRITE_SETTING_INT(file_checks_delay_per_block)
        WRITE_SETTING_INT(read_cache_line_size)
        WRITE_SETTING_INT(write_cache_line_size)
        WRITE_SETTING_INT(optimistic_disk_retry)
        WRITE_SETTING_INT(max_suggest_pieces)
        WRITE_SETTING_INT(local_service_announce_interval)
        WRITE_SETTING_INT(dht_announce_interval)
        WRITE_SETTING_INT(udp_tracker_token_expiry)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_INT(default_cache_min_age)
#else
        WRITE_SETTING_INT(deprecated16)
#endif
        WRITE_SETTING_INT(num_optimistic_unchoke_slots)
        WRITE_SETTING_INT(default_est_reciprocation_rate)
        WRITE_SETTING_INT(increase_est_reciprocation_rate)
        WRITE_SETTING_INT(decrease_est_reciprocation_rate)
        WRITE_SETTING_INT(max_pex_peers)
        WRITE_SETTING_INT(tick_interval)
        WRITE_SETTING_INT(share_mode_target)
        WRITE_SETTING_INT(upload_rate_limit)
        WRITE_SETTING_INT(download_rate_limit)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_INT(local_upload_rate_limit)
        WRITE_SETTING_INT(local_download_rate_limit)
#else
        WRITE_SETTING_INT(deprecated3)
        WRITE_SETTING_INT(deprecated4)
#endif
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_INT(dht_upload_rate_limit)
#else
        WRITE_SETTING_INT(deprecated7)
#endif
        WRITE_SETTING_INT(unchoke_slots_limit)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_INT(half_open_limit)
#else
        WRITE_SETTING_INT(deprecated5)
#endif
        WRITE_SETTING_INT(connections_limit)
        WRITE_SETTING_INT(connections_slack)
        WRITE_SETTING_INT(utp_target_delay)
        WRITE_SETTING_INT(utp_gain_factor)
        WRITE_SETTING_INT(utp_min_timeout)
        WRITE_SETTING_INT(utp_syn_resends)
        WRITE_SETTING_INT(utp_fin_resends)
        WRITE_SETTING_INT(utp_num_resends)
        WRITE_SETTING_INT(utp_connect_timeout)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_INT(utp_delayed_ack)
#else
        WRITE_SETTING_INT(deprecated6)
#endif
        WRITE_SETTING_INT(utp_loss_multiplier)
        WRITE_SETTING_INT(mixed_mode_algorithm)
        WRITE_SETTING_INT(listen_queue_size)
        WRITE_SETTING_INT(torrent_connect_boost)
        WRITE_SETTING_INT(alert_queue_size)
        WRITE_SETTING_INT(max_metadata_size)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_INT(hashing_threads)
#else
        WRITE_SETTING_INT(deprecated14)
#endif
        WRITE_SETTING_INT(checking_mem_usage)
        WRITE_SETTING_INT(predictive_piece_announce)
        WRITE_SETTING_INT(aio_threads)
        WRITE_SETTING_INT(aio_max)
        WRITE_SETTING_INT(network_threads)
#ifndef TORRENT_NO_DEPRECATE
        WRITE_SETTING_INT(ssl_listen)
#else
        WRITE_SETTING_INT(deprecated9)
#endif
        WRITE_SETTING_INT(tracker_backoff)
        WRITE_SETTING_INT(share_ratio_limit)
        WRITE_SETTING_INT(seed_time_ratio_limit)
        WRITE_SETTING_INT(peer_turnover)
        WRITE_SETTING_INT(peer_turnover_cutoff)
        WRITE_SETTING_INT(peer_turnover_interval)
        WRITE_SETTING_INT(connect_seed_every_n_download)
        WRITE_SETTING_INT(max_http_recv_buffer_size)
        WRITE_SETTING_INT(max_retry_port_bind)
        WRITE_SETTING_INT(alert_mask)
        WRITE_SETTING_INT(out_enc_policy)
        WRITE_SETTING_INT(in_enc_policy)
        WRITE_SETTING_INT(allowed_enc_level)
        WRITE_SETTING_INT(inactive_down_rate)
        WRITE_SETTING_INT(inactive_up_rate)
        WRITE_SETTING_INT(proxy_type)
        WRITE_SETTING_INT(proxy_port)
        WRITE_SETTING_INT(i2p_port)
        WRITE_SETTING_INT(cache_size_volatile)

#undef WRITE_SETTING_INT
#undef WRITE_SETTING_BOOL
#undef WRITE_SETTING_STR
#undef WRITE_SETTING_VAL

    }

    void from_settings_pack(const libtorrent::settings_pack& settings_pack, std::map<std::string, fc::variant>& settings_map) {

#define READ_SETTING_VAL(sp, sm, name, tp) if (sp.has_val(libtorrent::settings_pack::name)) sm[#name] = sp.get_##tp(libtorrent::settings_pack::name);

#define READ_SETTING_STR(name)   READ_SETTING_VAL(settings_pack, settings_map, name, str)
#define READ_SETTING_BOOL(name)  READ_SETTING_VAL(settings_pack, settings_map, name, bool)
#define READ_SETTING_INT(name)   READ_SETTING_VAL(settings_pack, settings_map, name, int)

        READ_SETTING_STR(user_agent)
        READ_SETTING_STR(announce_ip)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_STR(mmap_cache)
#else
        READ_SETTING_STR(deprecated21)
#endif
        READ_SETTING_STR(handshake_client_version)
        READ_SETTING_STR(outgoing_interfaces)
        READ_SETTING_STR(listen_interfaces)
        READ_SETTING_STR(proxy_hostname)
        READ_SETTING_STR(proxy_username)
        READ_SETTING_STR(proxy_password)
        READ_SETTING_STR(i2p_hostname)
        READ_SETTING_STR(peer_fingerprint)
        READ_SETTING_STR(dht_bootstrap_nodes)

        READ_SETTING_BOOL(allow_multiple_connections_per_ip)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(ignore_limits_on_local_network)
#else
        READ_SETTING_BOOL(deprecated1)
#endif
        READ_SETTING_BOOL(send_redundant_have)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(lazy_bitfields)
#else
        READ_SETTING_BOOL(deprecated12)
#endif
        READ_SETTING_BOOL(use_dht_as_fallback)
        READ_SETTING_BOOL(upnp_ignore_nonrouters)
        READ_SETTING_BOOL(use_parole_mode)
        READ_SETTING_BOOL(use_read_cache)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(use_write_cache)
        READ_SETTING_BOOL(dont_flush_write_cache)
#else
        READ_SETTING_BOOL(deprecated11)
        READ_SETTING_BOOL(deprecated22)
#endif
        READ_SETTING_BOOL(coalesce_reads)
        READ_SETTING_BOOL(coalesce_writes)
        READ_SETTING_BOOL(auto_manage_prefer_seeds)
        READ_SETTING_BOOL(dont_count_slow_torrents)
        READ_SETTING_BOOL(close_redundant_connections)
        READ_SETTING_BOOL(prioritize_partial_pieces)
        READ_SETTING_BOOL(rate_limit_ip_overhead)
        READ_SETTING_BOOL(announce_to_all_tiers)
        READ_SETTING_BOOL(announce_to_all_trackers)
        READ_SETTING_BOOL(prefer_udp_trackers)
        READ_SETTING_BOOL(strict_super_seeding)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(lock_disk_cache)
#else
        READ_SETTING_BOOL(deprecated10)
#endif
        READ_SETTING_BOOL(disable_hash_checks)
        READ_SETTING_BOOL(allow_i2p_mixed)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(low_prio_disk)
#else
        READ_SETTING_BOOL(deprecated17)
#endif
        READ_SETTING_BOOL(volatile_read_cache)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(guided_read_cache)
#else
        READ_SETTING_BOOL(deprecated13)
#endif
        READ_SETTING_BOOL(no_atime_storage)
        READ_SETTING_BOOL(incoming_starts_queued_torrents)
        READ_SETTING_BOOL(report_true_downloaded)
        READ_SETTING_BOOL(strict_end_game_mode)
        READ_SETTING_BOOL(broadcast_lsd)
        READ_SETTING_BOOL(enable_outgoing_utp)
        READ_SETTING_BOOL(enable_incoming_utp)
        READ_SETTING_BOOL(enable_outgoing_tcp)
        READ_SETTING_BOOL(enable_incoming_tcp)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(ignore_resume_timestamps)
#else
        READ_SETTING_BOOL(deprecated8)
#endif
        READ_SETTING_BOOL(no_recheck_incomplete_resume)
        READ_SETTING_BOOL(anonymous_mode)
        READ_SETTING_BOOL(report_web_seed_downloads)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(rate_limit_utp)
#else
        READ_SETTING_BOOL(deprecated2)
#endif
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(announce_double_nat)
#else
        READ_SETTING_BOOL(deprecated18)
#endif
        READ_SETTING_BOOL(seeding_outgoing_connections)
        READ_SETTING_BOOL(no_connect_privileged_ports)
        READ_SETTING_BOOL(smooth_connects)
        READ_SETTING_BOOL(always_send_user_agent)
        READ_SETTING_BOOL(apply_ip_filter_to_trackers)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(use_disk_read_ahead)
#else
        READ_SETTING_BOOL(deprecated19)
#endif
        READ_SETTING_BOOL(lock_files)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_BOOL(contiguous_recv_buffer)
#else
        READ_SETTING_BOOL(deprecated15)
#endif
        READ_SETTING_BOOL(ban_web_seeds)
        READ_SETTING_BOOL(allow_partial_disk_writes)
        READ_SETTING_BOOL(force_proxy)
        READ_SETTING_BOOL(support_share_mode)
        READ_SETTING_BOOL(support_merkle_torrents)
        READ_SETTING_BOOL(report_redundant_bytes)
        READ_SETTING_BOOL(listen_system_port_fallback)
        READ_SETTING_BOOL(use_disk_cache_pool)
        READ_SETTING_BOOL(announce_crypto_support)
        READ_SETTING_BOOL(enable_upnp)
        READ_SETTING_BOOL(enable_natpmp)
        READ_SETTING_BOOL(enable_lsd)
        READ_SETTING_BOOL(enable_dht)
        READ_SETTING_BOOL(prefer_rc4)
        READ_SETTING_BOOL(proxy_hostnames)
        READ_SETTING_BOOL(proxy_peer_connections)
        READ_SETTING_BOOL(auto_sequential)
        READ_SETTING_BOOL(proxy_tracker_connections)

        READ_SETTING_INT(tracker_completion_timeout)
        READ_SETTING_INT(tracker_receive_timeout)
        READ_SETTING_INT(stop_tracker_timeout)
        READ_SETTING_INT(tracker_maximum_response_length)
        READ_SETTING_INT(piece_timeout)
        READ_SETTING_INT(request_timeout)
        READ_SETTING_INT(request_queue_time)
        READ_SETTING_INT(max_allowed_in_request_queue)
        READ_SETTING_INT(max_out_request_queue)
        READ_SETTING_INT(whole_pieces_threshold)
        READ_SETTING_INT(peer_timeout)
        READ_SETTING_INT(urlseed_timeout)
        READ_SETTING_INT(urlseed_pipeline_size)
        READ_SETTING_INT(urlseed_max_request_bytes)
        READ_SETTING_INT(urlseed_wait_retry)
        READ_SETTING_INT(file_pool_size)
        READ_SETTING_INT(max_failcount)
        READ_SETTING_INT(min_reconnect_time)
        READ_SETTING_INT(peer_connect_timeout)
        READ_SETTING_INT(connection_speed)
        READ_SETTING_INT(inactivity_timeout)
        READ_SETTING_INT(unchoke_interval)
        READ_SETTING_INT(optimistic_unchoke_interval)
        READ_SETTING_INT(num_want)
        READ_SETTING_INT(initial_picker_threshold)
        READ_SETTING_INT(allowed_fast_set_size)
        READ_SETTING_INT(suggest_mode)
        READ_SETTING_INT(max_queued_disk_bytes)
        READ_SETTING_INT(handshake_timeout)
        READ_SETTING_INT(send_buffer_low_watermark)
        READ_SETTING_INT(send_buffer_watermark)
        READ_SETTING_INT(send_buffer_watermark_factor)
        READ_SETTING_INT(choking_algorithm)
        READ_SETTING_INT(seed_choking_algorithm)
        READ_SETTING_INT(cache_size)
        READ_SETTING_INT(cache_buffer_chunk_size)
        READ_SETTING_INT(cache_expiry)
        READ_SETTING_INT(disk_io_write_mode)
        READ_SETTING_INT(disk_io_read_mode)
        READ_SETTING_INT(outgoing_port)
        READ_SETTING_INT(num_outgoing_ports)
        READ_SETTING_INT(peer_tos)
        READ_SETTING_INT(active_downloads)
        READ_SETTING_INT(active_seeds)
        READ_SETTING_INT(active_checking)
        READ_SETTING_INT(active_dht_limit)
        READ_SETTING_INT(active_tracker_limit)
        READ_SETTING_INT(active_lsd_limit)
        READ_SETTING_INT(active_limit)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_INT(active_loaded_limit)
#else
        READ_SETTING_INT(deprecated20)
#endif
        READ_SETTING_INT(auto_manage_interval)
        READ_SETTING_INT(seed_time_limit)
        READ_SETTING_INT(auto_scrape_interval)
        READ_SETTING_INT(auto_scrape_min_interval)
        READ_SETTING_INT(max_peerlist_size)
        READ_SETTING_INT(max_paused_peerlist_size)
        READ_SETTING_INT(min_announce_interval)
        READ_SETTING_INT(auto_manage_startup)
        READ_SETTING_INT(seeding_piece_quota)
        READ_SETTING_INT(max_rejects)
        READ_SETTING_INT(recv_socket_buffer_size)
        READ_SETTING_INT(send_socket_buffer_size)
        READ_SETTING_INT(max_peer_recv_buffer_size)
        READ_SETTING_INT(file_checks_delay_per_block)
        READ_SETTING_INT(read_cache_line_size)
        READ_SETTING_INT(write_cache_line_size)
        READ_SETTING_INT(optimistic_disk_retry)
        READ_SETTING_INT(max_suggest_pieces)
        READ_SETTING_INT(local_service_announce_interval)
        READ_SETTING_INT(dht_announce_interval)
        READ_SETTING_INT(udp_tracker_token_expiry)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_INT(default_cache_min_age)
#else
        READ_SETTING_INT(deprecated16)
#endif
        READ_SETTING_INT(num_optimistic_unchoke_slots)
        READ_SETTING_INT(default_est_reciprocation_rate)
        READ_SETTING_INT(increase_est_reciprocation_rate)
        READ_SETTING_INT(decrease_est_reciprocation_rate)
        READ_SETTING_INT(max_pex_peers)
        READ_SETTING_INT(tick_interval)
        READ_SETTING_INT(share_mode_target)
        READ_SETTING_INT(upload_rate_limit)
        READ_SETTING_INT(download_rate_limit)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_INT(local_upload_rate_limit)
        READ_SETTING_INT(local_download_rate_limit)
#else
        READ_SETTING_INT(deprecated3)
        READ_SETTING_INT(deprecated4)
#endif
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_INT(dht_upload_rate_limit)
#else
        READ_SETTING_INT(deprecated7)
#endif
        READ_SETTING_INT(unchoke_slots_limit)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_INT(half_open_limit)
#else
        READ_SETTING_INT(deprecated5)
#endif
        READ_SETTING_INT(connections_limit)
        READ_SETTING_INT(connections_slack)
        READ_SETTING_INT(utp_target_delay)
        READ_SETTING_INT(utp_gain_factor)
        READ_SETTING_INT(utp_min_timeout)
        READ_SETTING_INT(utp_syn_resends)
        READ_SETTING_INT(utp_fin_resends)
        READ_SETTING_INT(utp_num_resends)
        READ_SETTING_INT(utp_connect_timeout)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_INT(utp_delayed_ack)
#else
        READ_SETTING_INT(deprecated6)
#endif
        READ_SETTING_INT(utp_loss_multiplier)
        READ_SETTING_INT(mixed_mode_algorithm)
        READ_SETTING_INT(listen_queue_size)
        READ_SETTING_INT(torrent_connect_boost)
        READ_SETTING_INT(alert_queue_size)
        READ_SETTING_INT(max_metadata_size)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_INT(hashing_threads)
#else
        READ_SETTING_INT(deprecated14)
#endif
        READ_SETTING_INT(checking_mem_usage)
        READ_SETTING_INT(predictive_piece_announce)
        READ_SETTING_INT(aio_threads)
        READ_SETTING_INT(aio_max)
        READ_SETTING_INT(network_threads)
#ifndef TORRENT_NO_DEPRECATE
        READ_SETTING_INT(ssl_listen)
#else
        READ_SETTING_INT(deprecated9)
#endif
        READ_SETTING_INT(tracker_backoff)
        READ_SETTING_INT(share_ratio_limit)
        READ_SETTING_INT(seed_time_ratio_limit)
        READ_SETTING_INT(peer_turnover)
        READ_SETTING_INT(peer_turnover_cutoff)
        READ_SETTING_INT(peer_turnover_interval)
        READ_SETTING_INT(connect_seed_every_n_download)
        READ_SETTING_INT(max_http_recv_buffer_size)
        READ_SETTING_INT(max_retry_port_bind)
        READ_SETTING_INT(alert_mask)
        READ_SETTING_INT(out_enc_policy)
        READ_SETTING_INT(in_enc_policy)
        READ_SETTING_INT(allowed_enc_level)
        READ_SETTING_INT(inactive_down_rate)
        READ_SETTING_INT(inactive_up_rate)
        READ_SETTING_INT(proxy_type)
        READ_SETTING_INT(proxy_port)
        READ_SETTING_INT(i2p_port)
        READ_SETTING_INT(cache_size_volatile)

#undef READ_SETTING_INT
#undef READ_SETTING_BOOL
#undef READ_SETTING_STR
#undef READ_SETTING_VAL

    }


}


namespace graphene { namespace package { namespace detail {


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
    libtorrent_config_data() {
        libtorrent::session_params p;

        // The default config:

        p.settings.set_str(libtorrent::settings_pack::user_agent,                          "Decent/0.1.0");
        p.settings.set_str(libtorrent::settings_pack::peer_fingerprint,                    "-dst010-");
        p.settings.set_bool(libtorrent::settings_pack::allow_multiple_connections_per_ip,  true);
        p.settings.set_bool(libtorrent::settings_pack::announce_to_all_tiers,              true);
        p.settings.set_bool(libtorrent::settings_pack::announce_to_all_trackers,           true);
        p.settings.set_bool(libtorrent::settings_pack::prefer_udp_trackers,                false);
        p.settings.set_bool(libtorrent::settings_pack::incoming_starts_queued_torrents,    true);
        p.settings.set_bool(libtorrent::settings_pack::lock_files,                         true);
        p.settings.set_bool(libtorrent::settings_pack::enable_upnp,                        true);
        p.settings.set_bool(libtorrent::settings_pack::enable_natpmp,                      true);
        p.settings.set_bool(libtorrent::settings_pack::enable_lsd,                         true);
        p.settings.set_bool(libtorrent::settings_pack::enable_dht,                         true);
        p.settings.set_int(libtorrent::settings_pack::dht_announce_interval,               15);
        p.settings.set_int(libtorrent::settings_pack::dht_upload_rate_limit,               10000);
        p.settings.set_int(libtorrent::settings_pack::active_limit,                        100);
        p.settings.set_int(libtorrent::settings_pack::active_seeds,                        90);
        p.settings.set_int(libtorrent::settings_pack::active_downloads,                    10);

        std::vector<std::pair<std::string, int>> default_dht_nodes{
            {"dht.transmissionbt.com", 6881},
            {"router.utorrent.com",    6881},
            {"router.bittorrent.com",  6881},
            {"router.bitcomet.com",    6881},
            {"dht.aelitis.com",        6881},
            {"dht.libtorrent.org",     25401}
        };

        std::string orig_dht_bootstrap_nodes = p.settings.get_str(libtorrent::settings_pack::dht_bootstrap_nodes);
        std::string dht_bootstrap_nodes;
        for (auto dht_node : default_dht_nodes) {
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

        dht_settings = p.dht_settings;
        from_settings_pack(p.settings, settings);
    }

    std::map<std::string, fc::variant> settings;
    libtorrent::dht_settings dht_settings;
    upload_torrent_data upload_torrent;
    download_torrent_data download_torrent;
};


} } } // graphene::package::detail


FC_REFLECT( graphene::package::detail::upload_torrent_data,
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

FC_REFLECT( graphene::package::detail::download_torrent_data,
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

FC_REFLECT( graphene::package::detail::libtorrent_config_data,
            (settings)
            (dht_settings)
            (upload_torrent)
            (download_torrent)
          )


torrent_transfer::torrent_transfer(const torrent_transfer& orig)
    : _thread(orig._thread)
    , _session(orig._session)
    , _config_data(orig._config_data)
    , _lifetime_info_mutex(std::make_shared<fc::mutex>())
    , _instance_exists(std::make_shared<std::atomic<bool>>(true))
    , _transfer_logger(orig._transfer_logger)
{
    if (!_thread)       FC_THROW("Thread instance is not available");
    if (!_session)      FC_THROW("Session instance is not available");
    if (!_config_data)  FC_THROW("Config data is not available");
}

torrent_transfer::torrent_transfer()
    : _thread(std::make_shared<fc::thread>("torrent_thread"))
    , _session(nullptr)
    , _config_data(std::make_shared<detail::libtorrent_config_data>())
    , _lifetime_info_mutex(std::make_shared<fc::mutex>())
    , _instance_exists(std::make_shared<std::atomic<bool>>(true))
    , _transfer_logger(fc::logger::get("transfer"))
{
    libtorrent::session_params p;
    to_settings_pack(_config_data->settings, p.settings);
    p.dht_settings = _config_data->dht_settings;

    _session = std::make_shared<libtorrent::session>(p);

//  _session->add_extension(&libtorrent::create_metadata_plugin);
    _session->add_extension(&libtorrent::create_ut_metadata_plugin);
    _session->add_extension(&libtorrent::create_ut_pex_plugin);

    libtorrent::error_code ec;

    const path session_state_file = decent_path_finder::instance().get_decent_home() / ".ses_state";
    std::vector<char> in;
    if (load_file(session_state_file, in, ec) == 0) {
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

        const path session_state_file = decent_path_finder::instance().get_decent_home() / ".ses_state";
        std::vector<char> out;
        bencode(std::back_inserter(out), session_state);
        save_file(session_state_file, out);
    }

//  _lifetime_info_mutex->unlock();
//  _lifetime_info_mutex->lock();

    *_instance_exists = false;
}

void torrent_transfer::reconfigure(const boost::filesystem::path& config_file) {
    if (!config_file.empty() && !boost::filesystem::exists(config_file)) {
        FC_THROW("Unable to read libtorrent config file ${fn}: file does not exists", ("fn", config_file.string()) );
    }
    *_config_data = (config_file.empty() ? detail::libtorrent_config_data() : fc::json::from_file(config_file).as<detail::libtorrent_config_data>());

    libtorrent::settings_pack sp;
    to_settings_pack(_config_data->settings, sp);

    _session->pause();
    _session->apply_settings(sp);
    _session->set_dht_settings(_config_data->dht_settings);
    _session->resume();
}

void torrent_transfer::dump_config(const boost::filesystem::path& config_file) {
    if (config_file.empty()) {
        FC_THROW("Unable to save current libtorrent config to file: file name is not specified");
    }

    ilog("saving current libtorrent config to file ${fn}", ("fn", config_file.string()) );
    std::string data = fc::json::to_pretty_string(*_config_data);
    fc::ofstream outfile{config_file};
    outfile.write(data.c_str(), data.length());
}

package_transfer_interface::transfer_progress torrent_transfer::get_progress() {
    libtorrent::torrent_status st = _torrent_handle.status();
    
    std::string str_status = "";
    switch (st.state) {
        case torrent_status::checking_files:
            str_status = "Checking Files";
            break;
        case torrent_status::downloading_metadata:
            str_status = "Downloading Metadata";
            break;
        case torrent_status::downloading:
            str_status = "Downloading";
            break;
        case torrent_status::finished:
            str_status = "Finished";
            break;
        case torrent_status::seeding:
            str_status = "Seeding";
            break;
        case torrent_status::allocating:
            str_status = "Allocating";
            break;
        case torrent_status::checking_resume_data:
            str_status = "Checking Resume Data";
            break;
        case torrent_status::queued_for_checking:
            str_status = "Queued For Checking";
            break;
    }

    
    return transfer_progress(st.total_wanted, st.total_wanted_done, st.download_rate, str_status);
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


fc::ripemd160 torrent_transfer::hash_from_url(const std::string& url) {
   
}

void torrent_transfer::upload_package(transfer_id id, const package_object& package, transfer_listener* listener) {
    _id = id;
    _listener = listener;
    _is_upload = true;

    fc_ilog(_transfer_logger, "torrent upload started for package: %{hash}", ("hash", package.get_hash().str()) );

    file_storage fs;
    libtorrent::error_code ec;

    // recursively adds files in directories
    add_files(fs, package.get_path().string());

    auto tp = _config_data->upload_torrent;

    create_torrent t(fs, tp.piece_size);

    t.set_creator(tp.creator.c_str());
    t.set_priv(tp.priv);

    // reads the files and calculates the hashes
    set_piece_hashes(t, package.get_path().parent_path().string(), ec);

    if (ec) {
        listener->on_error(_id, ec.message());
        return;
    }

    path temp_file = temp_directory_path() / (package.get_hash().str() + ".torrent");
    path temp_dir = temp_directory_path();


    std::ofstream out(temp_file.string(), std::ios_base::binary);
    bencode(std::ostream_iterator<char>(out), t.generate());
    out.close();

    ilog("torrent file created: {fn}", ("fn", temp_file.string()) );

    libtorrent::add_torrent_params atp;

    atp.ti = std::make_shared<libtorrent::torrent_info>(temp_file.string(), 0);
    atp.save_path = package.get_path().parent_path().string();

    atp.flags |= libtorrent::add_torrent_params::flag_seed_mode;
    atp.flags |= libtorrent::add_torrent_params::flag_merge_resume_http_seeds;
    atp.flags |= libtorrent::add_torrent_params::flag_merge_resume_trackers;

    (tp.upload_mode         ? atp.flags |= libtorrent::add_torrent_params::flag_upload_mode    : atp.flags &= ~libtorrent::add_torrent_params::flag_upload_mode);
    (tp.super_seeding_mode  ? atp.flags |= libtorrent::add_torrent_params::flag_super_seeding  : atp.flags &= ~libtorrent::add_torrent_params::flag_super_seeding);
    (tp.share_mode          ? atp.flags |= libtorrent::add_torrent_params::flag_share_mode     : atp.flags &= ~libtorrent::add_torrent_params::flag_share_mode);
    (tp.auto_managed        ? atp.flags |= libtorrent::add_torrent_params::flag_auto_managed   : atp.flags &= ~libtorrent::add_torrent_params::flag_auto_managed);

    atp.flags &= ~libtorrent::add_torrent_params::flag_duplicate_is_error;

    atp.dht_nodes = tp.dht_nodes;
    atp.trackers = tp.trackers;

    for (auto dht_node : atp.dht_nodes) {
        t.add_node(dht_node);
    }

    for (auto tracker : atp.trackers) {
        t.add_tracker(tracker);
    }

    _torrent_handle = _session->add_torrent(atp);

    _url = make_magnet_uri(_torrent_handle);

    _torrent_handle.set_max_uploads(tp.max_uploads);
    _torrent_handle.set_max_connections(tp.max_connections);
    _torrent_handle.set_upload_limit(tp.upload_limit);
    _torrent_handle.set_download_limit(tp.download_limit);

    for(auto url_seed : tp.url_seeds) {
        _torrent_handle.add_url_seed(url_seed);
    }

    for(auto http_seed : tp.http_seeds) {
        _torrent_handle.add_http_seed(http_seed);
    }

    _session->resume();
    _torrent_handle.resume();

    if (tp.announce_on_add) {
        _torrent_handle.force_reannounce();
        _torrent_handle.force_dht_announce();
    }

    if (tp.scrape_on_add) {
        _torrent_handle.scrape_tracker();
    }

    auto lifetime_info_mutex = _lifetime_info_mutex;
    auto instance_exists = _instance_exists;
    _thread->async([this, lifetime_info_mutex, instance_exists] () {

        fc::scoped_lock<fc::mutex> guard(*lifetime_info_mutex);
        if (*instance_exists) {
            this->update_torrent_status();
            _listener->on_upload_started(_id, _url);
        }

    });
}

void torrent_transfer::download_package(transfer_id id, const std::string& url, transfer_listener* listener, report_stats_listener_base& stats_listener) {
    _id = id;
    _listener = listener;
    _url = url;
    _is_upload = false;

    fc_ilog(_transfer_logger, "torrent download started from url: %{url}", ("url", url) );

    auto tp = _config_data->download_torrent;

    libtorrent::add_torrent_params atp;

    atp.url = url;
    atp.save_path = temp_directory_path().string();

    ilog("downloading package to ${fn}", ("fn", atp.save_path) );

    atp.flags |= libtorrent::add_torrent_params::flag_merge_resume_http_seeds;
    atp.flags |= libtorrent::add_torrent_params::flag_merge_resume_trackers;

    (tp.upload_mode         ? atp.flags |= libtorrent::add_torrent_params::flag_upload_mode    : atp.flags &= ~libtorrent::add_torrent_params::flag_upload_mode);
    (tp.share_mode          ? atp.flags |= libtorrent::add_torrent_params::flag_share_mode     : atp.flags &= ~libtorrent::add_torrent_params::flag_share_mode);
    (tp.auto_managed        ? atp.flags |= libtorrent::add_torrent_params::flag_auto_managed   : atp.flags &= ~libtorrent::add_torrent_params::flag_auto_managed);

    atp.flags &= ~libtorrent::add_torrent_params::flag_duplicate_is_error;

    atp.dht_nodes = tp.dht_nodes;
    atp.trackers = tp.trackers;

    _torrent_handle = _session->add_torrent(atp);

    _torrent_handle.set_max_uploads(tp.max_uploads);
    _torrent_handle.set_max_connections(tp.max_connections);
    _torrent_handle.set_upload_limit(tp.upload_limit);
    _torrent_handle.set_download_limit(tp.download_limit);

    for(auto url_seed : tp.url_seeds) {
        _torrent_handle.add_url_seed(url_seed);
    }

    for(auto http_seed : tp.http_seeds) {
        _torrent_handle.add_http_seed(http_seed);
    }

    _session->resume();
    _torrent_handle.resume();

    if (tp.announce_on_add) {
        _torrent_handle.force_reannounce();
        _torrent_handle.force_dht_announce();
    }

    if (tp.scrape_on_add) {
        _torrent_handle.scrape_tracker();
    }

    auto lifetime_info_mutex = _lifetime_info_mutex;
    auto instance_exists = _instance_exists;
    _thread->async([this, lifetime_info_mutex, instance_exists] () {

        fc::scoped_lock<fc::mutex> guard(*lifetime_info_mutex);
        if (*instance_exists) {
            this->update_torrent_status();
            _listener->on_download_started(_id);
        }

    });
}

void torrent_transfer::update_torrent_status() {
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
				_listener->on_download_progress(_id, transfer_progress(st.total_wanted, st.total_wanted_done, st.download_rate, "Downloading..."));
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
    std::vector<libtorrent::alert*> alerts;
	_session->pop_alerts(&alerts);

	for (int i = 0; i < alerts.size(); ++i) {
		libtorrent::alert* alert = alerts[i];
        const int cat = alert->category();

        fc_ilog(_transfer_logger, "[${ERROR}${PEER}${PORT_MAP}${STORAGE}${TRACKER}${DEBUG}${STATUS}${PROGRESS}${IPBLOCK}${PERFORMANCE}${DHT}${STATS}${SESSION}${TORRENT}${PEERL}${INREQ}${DHT_L}${DHT_OP}${PORM_MAPL}${PICKERL} ] ~> [ ${what} ]: ${message}",
                ("ERROR", (cat & libtorrent::alert::error_notification ? " ERROR" : ""))
                ("PEER", (cat & libtorrent::alert::peer_notification ? " PEER" : ""))
                ("PORT_MAP", (cat & libtorrent::alert::port_mapping_notification ? " PORT_MAP" : ""))
                ("STORAGE", (cat & libtorrent::alert::storage_notification ? " STORAGE" : ""))
                ("TRACKER", (cat & libtorrent::alert::tracker_notification ? " TRACKER" : ""))
                ("DEBUG", (cat & libtorrent::alert::debug_notification ? " DEBUG" : ""))
                ("STATUS", (cat & libtorrent::alert::status_notification ? " STATUS" : ""))
                ("PROGRESS", (cat & libtorrent::alert::progress_notification ? " PROGRESS" : ""))
                ("IPBLOCK", (cat & libtorrent::alert::ip_block_notification ? " IPBLOCK" : ""))
                ("PERFORMANCE", (cat & libtorrent::alert::performance_warning ? " PERFORMANCE" : ""))
                ("DHT", (cat & libtorrent::alert::dht_notification ? " DHT" : ""))
                ("STATS", (cat & libtorrent::alert::stats_notification ? " STATS" : ""))
                ("SESSION", (cat & libtorrent::alert::session_log_notification ? " SESSION" : ""))
                ("TORRENT", (cat & libtorrent::alert::torrent_log_notification ? " TORRENT" : ""))
                ("PEERL", (cat & libtorrent::alert::peer_log_notification ? " PEERL" : ""))
                ("INREQ", (cat & libtorrent::alert::incoming_request_notification ? " INREQ" : ""))
                ("DHT_L", (cat & libtorrent::alert::dht_log_notification ? " DHT_L" : ""))
                ("DHT_OP", (cat & libtorrent::alert::dht_operation_notification ? " DHT_OP" : ""))
                ("PORM_MAPL", (cat & libtorrent::alert::port_mapping_log_notification ? " PORM_MAPL" : ""))
                ("PICKERL", (cat & libtorrent::alert::picker_log_notification ? " PICKERL" : ""))
                ("what", alert->what())
                ("message", alert->message())
        );
	}
}

std::string torrent_transfer::get_transfer_url() {
	return _url;
}
