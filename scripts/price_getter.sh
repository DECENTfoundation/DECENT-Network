#!/bin/bash

if [ $# -le 0 ]; then echo "Usage: $0 http://ip:port account_name coinmarketcat_api_key"; exit ; fi
endpoint=$1
account=$2
CMC_API_KEY=$3
max_length=17

for c in USD EUR JPY BGN CZK DKK GBP HUF PLN RON SEK CHF NOK HRK RUB TRY AUD BRL CAD CNY HKD IDR ILS INR KRW MXN MYR NZD PHP SGD THB ZAR
do
   price=`curl -s -H "X-CMC_PRO_API_KEY: $CMC_API_KEY" "https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/latest?symbol=DCT&convert=$c" | grep -i price | grep -o '[0-9]*\.\?[0-9]\+'`
   orig_price=`echo $price`
   base=`echo $price | sed 's/\(.*\)\.\(.*\)/\1\2/'`
   quote_exp=`echo $price | sed 's/\(.*\)\.\(.*\)/\20000/' |sed s/./0/g`
   id=`curl -s -X POST --data "{\"id\":1, \"method\":\"call\", \"params\":[0,\"get_asset\",[\"$c\"]]}" $endpoint | awk -F '"' '{print $8}'`

   length=`echo "$quote_exp" | awk '{print length}'`
   if (( length > max_length )); then
      diff=$((length - max_length))
      base=`echo $base | sed 's/.\{'"$diff"'\}$//'`
      quote_exp=`echo $quote_exp | sed 's/.\{'"$diff"'\}$//'`
   fi

   result=`curl -s -X POST --data "{\"id\":1, \"method\":\"call\", \"params\":[0,\"publish_asset_feed\",[\"$account\",\"$c\",{ \"core_exchange_rate\":{\"quote\":{\"amount\": 1$quote_exp, \"asset_id\": \"1.3.0\" }, \"base\":{\"amount\": \"$base\", \"asset_id\": \"$id\" } } }, true]]}" $endpoint`

   echo "$result"
   echo
   sleep 2 # cmc basic plan limits the number of requests to 30 per minute.
done
