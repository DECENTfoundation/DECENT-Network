#!/bin/bash

if [ $# -le 0 ]; then echo "Usage: $0 http://ip:port account_name"; exit ; fi
endpoint=$1
account=$2


for c in AUD BRL CAD CHF CNY EUR GBP HKD IDR INR JPY KRW MXN RUB USD;
do
   price=`curl -s "https://api.coinmarketcap.com/v1/ticker/decent/?convert=$c&limit=10"|grep -i price_$c |awk -F '"' '{print $4}'`
   echo "$c : $price"
   base=`echo $price | sed 's/\(.*\)\.\(.*\)/\1\2/'`
   quote_exp=`echo $price | sed 's/\(.*\)\.\(.*\)/\2/' |sed s/./0/g`
   id=`curl -s -X POST --data "{\"id\":1, \"method\":\"call\", \"params\":[0,\"get_asset\",[\"$c\"]]}" $endpoint |awk -F '"' '{print $8}'`
   curl -X POST --data "{\"id\":1, \"method\":\"call\", \"params\":[0,\"publish_asset_feed\",[\"$account\",\"$c\",{ \"core_exchange_rate\":{\"quote\":{\"amount\": 1$quote_exp, \"asset_id\": \"1.3.0\" }, \"base\":{\"amount\": \"$base\", \"asset_id\": \"$id\" } } }, true]]}" $endpoint
   echo
done
