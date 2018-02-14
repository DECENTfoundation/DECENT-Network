#ifndef DECENT_WALLET_MESSAGING_H
#define DECENT_WALLET_MESSAGING_H

/**
 * @brief Send text message
 * @param from
 * @param to
 * @param text
 */
void send_message(const std::string& from, std::vector<string> to, string text);

/**
* @brief Receives message objects by sender and/or receiver
* @param sender Name of message sender. If you dont want to filter by sender then let it empty.
* @param receiver Name of message receiver. If you dont want to filter by receiver then let it empty.
* @param max_count Maximal number of last messages to be displayed
* @return vector of message objects
*/
vector<message_object> get_message_objects(const std::string& sender, const std::string& receiver, uint32_t max_count) const;

/**
* @brief Receives messages by receiver
* @param receiver Name of message receiver which must be imported to caller's wallet
* @param max_count Maximal number of last messages to be displayed
* @return vector of message objects
*/
vector<text_message> get_messages(const std::string& receiver, uint32_t max_count) const;

/**
* @brief Receives sent messages by sender
* @param sender Name of message sender which must be imported to caller's wallet
* @param max_count Maximal number of last messages to be displayed
* @return vector of message objects
*/
vector<text_message> get_sent_messages(const std::string& sender, uint32_t max_count) const;

#endif //DECENT_WALLET_MESSAGING_H
