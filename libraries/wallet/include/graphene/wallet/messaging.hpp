/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
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
