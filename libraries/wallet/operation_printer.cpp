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

#include "operation_printer.hpp"
#include "wallet_impl.hpp"

namespace graphene { namespace wallet { namespace detail {

operation_printer::operation_printer(std::ostream& out, const wallet_api_impl& wallet, const chain::operation_result& r)
   : out(out), wallet(wallet), result(r)
{
}

void operation_printer::fee(const chain::asset& a) const
{
   out << " (Fee: " << wallet.get_asset(a.asset_id).amount_to_pretty_string(a) << ")";
}

std::string operation_printer::memo(const fc::optional<chain::memo_data>& data, const chain::account_object& from, const chain::account_object& to) const
{
   std::string memo;
   if( data )
   {
      if( wallet.is_locked() )
      {
         out << " -- Unlock wallet to see memo.";
      }
      else
      {
         try
         {
            memo = wallet.decrypt_memo( *data, from, to );
            out << " -- Memo: " << memo;
         }
         catch (const fc::exception& e)
         {
            out << " -- could not decrypt memo";
            elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
         }
      }
   }
   return memo;
}

std::string operation_printer::generic(std::string op_name, const chain::asset& fee, chain::account_id_type payer_id) const
{
   //balance_accumulator acc;
   //op.get_balance_delta( acc, result );
   auto a = wallet.get_asset(fee.asset_id);
   auto payer = wallet.get_account(payer_id);

   if( op_name.find_last_of(':') != std::string::npos )
      op_name.erase(0, op_name.find_last_of(':')+1);
   out << op_name <<" ";
   // out << "balance delta: " << fc::json::to_string(acc.balance) <<"   ";
   out << payer.name << " fee: " << a.amount_to_pretty_string(fee);
   result_visitor rprinter;
   std::string str_result = result.visit(rprinter);
   if( !str_result.empty() )
   {
      out << "   result: " << str_result;
   }
   return std::string();
}

std::string operation_printer::operator()(const chain::transfer_obsolete_operation& op) const
{
   const auto& from_account = wallet.get_account(op.from);
   const auto& to_account = wallet.get_account(op.to);
   out << "Transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
         << " from " << from_account.name << " to " << to_account.name;
   fee(op.fee);
   return memo(op.memo, from_account, to_account);
}

std::string operation_printer::operator()(const chain::transfer_operation& op) const
{
   const auto& from_account = wallet.get_account(op.from);
   chain::account_object to_account;
   std::string receiver;

   if(  op.to.is<chain::account_id_type>() )
   {
      to_account = wallet.get_account(op.to);
      receiver = to_account.name;
   }
   else
   {
      chain::content_id_type content_id = op.to.as<chain::content_id_type>();
      const chain::content_object content_obj = wallet.get_object<chain::content_object>(content_id);
      to_account = wallet.get_account(content_obj.author);
      receiver = std::string(op.to);
   }

   out << "Transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
         << " from " << from_account.name << " to " << receiver;
   fee(op.fee);
   return memo(op.memo, from_account, to_account);
}

std::string operation_printer::operator()(const chain::non_fungible_token_issue_operation& op) const
{
   const auto& from_account = wallet.get_account(op.issuer);
   const auto& to_account = wallet.get_account(op.to);
   out << "Issue non fungible token " << wallet.get_non_fungible_token(op.nft_id).symbol << " from " << from_account.name << " to " << to_account.name;
   fee(op.fee);
   return memo(op.memo, from_account, to_account);
}

std::string operation_printer::operator()(const chain::non_fungible_token_transfer_operation& op) const
{
   const auto& from_account = wallet.get_account(op.from);
   const auto& to_account = wallet.get_account(op.to);
   const auto& nft_data = wallet.get_non_fungible_token_data(op.nft_data_id);
   out << "Transfer non fungible token " << wallet.get_non_fungible_token(nft_data.nft_id).symbol <<
      " (" << std::string(db::object_id_type(nft_data.get_id())) << ") from " << from_account.name << " to " << to_account.name;
   fee(op.fee);
   return memo(op.memo, from_account, to_account);
}

std::string operation_printer::operator()(const chain::leave_rating_and_comment_operation& op) const
{
   out << wallet.get_account(op.consumer).name;
   if( op.comment.empty() )
      out << " rated " << op.URI << " -- Rating: " << op.rating;
   else if( op.rating == 0 )
      out << " commented " << op.URI << " -- Comment: " << op.comment;
   else
      out << " rated and commented " << op.URI << " -- Rating: " << op.rating << " -- Comment: " << op.comment;
   fee(op.fee);
   return std::string();
}

std::string operation_printer::operator()(const chain::account_create_operation& op) const
{
   out << "Create Account '" << op.name << "'";
   fee(op.fee);
   return std::string();
}

std::string operation_printer::operator()(const chain::account_update_operation& op) const
{
   out << "Update Account '" << wallet.get_account(op.account).name << "'";
   fee(op.fee);
   return std::string();
}

std::string operation_printer::operator()(const chain::asset_create_operation& op) const
{
   out << "Create ";
   out << "Monitored Asset ";

   out << "'" << op.symbol << "' with issuer " << wallet.get_account(op.issuer).name;
   fee(op.fee);
   return std::string();
}

std::string operation_printer::operator()(const chain::content_submit_operation& op) const
{
   out << "Submit content by " << wallet.get_account(op.author).name << " -- URI: " << op.URI;
   fee(op.fee);
   return std::string();
}

std::string operation_printer::operator()(const chain::request_to_buy_operation& op) const
{
   out << "Request to buy by " << wallet.get_account(op.consumer).name << " -- URI: " << op.URI << " -- Price: " << op.price.amount.value;
   fee(op.fee);
   return std::string();
}

std::string operation_printer::operator()(const chain::ready_to_publish_operation& op) const
{
   out << "Ready to publish -- Seeder: " << wallet.get_account(op.seeder).name << " -- space: " << op.space << " -- Price per MB: " << op.price_per_MByte;
   fee(op.fee);
   return std::string();
}

std::string operation_printer::operator()(const chain::custom_operation& op) const
{
   if (op.id == chain::custom_operation::custom_operation_subtype_messaging) {
      chain::message_payload pl;
      op.get_messaging_payload(pl);

      const auto& from_account = wallet.get_account(pl.from);
      chain::account_object to_account;
      std::string receivers;

      for (int i = 0; i < (int)pl.receivers_data.size(); i++) {
         const auto& to_account = wallet.get_account(pl.receivers_data[i].to);
         receivers += to_account.name;
         receivers += " ";
      }

      out << "Send message from " << from_account.name << " to " << receivers;

      std::string memo;
      if (wallet.is_locked())
      {
         out << " -- Unlock wallet to see memo.";
      }
      else
      {
         for (const auto& receivers_data_item : pl.receivers_data) {
            try
            {
               try {

                  if (pl.pub_from == chain::public_key_type() || receivers_data_item.pub_to == chain::public_key_type())
                  {
                     memo = receivers_data_item.get_message(chain::private_key_type(), chain::public_key_type());
                     break;
                  }

                  auto it = wallet._keys.find(receivers_data_item.pub_to);
                  if (it != wallet._keys.end()) {
                     fc::optional<chain::private_key_type> privkey = utilities::wif_to_key(it->second);
                     if (privkey)
                        memo = receivers_data_item.get_message(*privkey, pl.pub_from);
                     else
                        std::cout << "Cannot decrypt message." << std::endl;
                  }
                  else {
                     it = wallet._keys.find(pl.pub_from);
                     if (it != wallet._keys.end()) {
                        fc::optional<chain::private_key_type> privkey = utilities::wif_to_key(it->second);
                        if (privkey)
                           memo = receivers_data_item.get_message(*privkey, receivers_data_item.pub_to);
                        else
                           std::cout << "Cannot decrypt message." << std::endl;
                     }
                     else {
                        std::cout << "Cannot decrypt message." << std::endl;
                     }
                  }

               }
               catch (fc::exception& e)
               {
                  std::cout << "Cannot decrypt message." << std::endl;
                  std::cout << "Error: " << e.what() << std::endl;
                  throw;
               }
               catch (...) {
                  std::cout << "Unknown exception in decrypting message" << std::endl;
                  throw;
               }
               // memo = wallet.decrypt_memo(*op.memo, from_account, to_account);
               out << " -- Memo: " << memo;
            }
            catch (const fc::exception& e)
            {
               out << " -- could not decrypt memo";
               elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
            }
         }
      }

      fee(op.fee);
      return memo;
   }
   return std::string();
}

std::string operation_printer::result_visitor::operator()(const chain::void_result& x) const
{
   return std::string();
}

std::string operation_printer::result_visitor::operator()(const db::object_id_type& oid) const
{
   return std::string(oid);
}

} } } // namespace graphene::wallet::detail
