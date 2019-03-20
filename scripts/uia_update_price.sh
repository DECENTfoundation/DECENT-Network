#!/bin/bash

#Assumptions of this script:
# - the wallet is running, is unlocked, is listening on 127.0.0.1:8093
# - an updating account has the right to update the asset,
# - a funding account has sufficient balance to fund UIA/DCT pool/s,
# - necessary private accounts and private keys are imported in the wallet.

######################
# GENERAL PARAMETERS #
######################

#These parameters are adjustable as needed and define the behaviour of this script!

#MANDATORY PARAMETERS

WALLET_IP_AND_PORT="127.0.0.1:8093"

#general UIA data: its name and whether it is pegged to USD
UIA_NAME="RRR"
IS_UIA_PEGGED_TO_USD="false"

#set to true/false depending on whether asset price and/or asset pools should be updated
UPDATE_PRICE="true"
UPDATE_POOLS="true"

#pro API usage flag and key
#to use the live pro API of CoinMarketCap, the flag below has to be set to true
#and a pro API key needs to be specified
COINMARKETCAP_API_KEY="<insert_coinmarketcap_api_key_here>"

#PARAMETERS FOR PRICE UPDATE

#account to use for the price updates
UPDATING_ACCOUNT=""

#price diff threshold will be measured as 1 / PRICE_DIFF_THRESHOLD_QUOTIENT of the current quote
#PRICE_DIFF_THRESHOLD_QUOTIENT must be an integer
PRICE_DIFF_THRESHOLD_QUOTIENT=100

#PARAMETERS FOR POOL UPDATE

#account to use for the pool updates
FUNDING_ACCOUNT=""

#currency pool limits
#important: these limits can be integers
UIA_POOL_LIMIT=10
DCT_POOL_LIMIT=10

#thresholds for different pools to avoid very small pool updates
#pool and base-quote are updated if the amount missing is not within the acceptable threshold
#important: these limits can be integers
UIA_POOL_THRESHOLD=5
DCT_POOL_THRESHOLD=5

#we assume that balance limits are greater than pool limits!
#important: we also assume that these values are integers (floating-point operations not supported)!
UIA_BALANCE_LIMIT=100
DCT_BALANCE_LIMIT=100

#OTHER PARAMETERS

DIRECTORY=$(cd "$(dirname "${0}")"; echo "$(pwd)")
LOG_FILE_NAME=$DIRECTORY"/"$UIA_NAME"_update_log.txt"

#IDs of DCT and UIA for the CoinMarketCap API
#the CoinMarketCap ID for DCT is fixed below (to lower the number of API requests) and could also be obtained by the following call
#COINMARKETCAP_ID_DCT=`curl -s -H "X-CMC_PRO_API_KEY: <insert_coinmarketcap_api_key_here>" -X GET https://sandbox-api.coinmarketcap.com/v1/cryptocurrency/info?symbol="DCT"`
COINMARKETCAP_ID_DCT="1478"
#the CoinMarketCap ID for UIA is fixed below (to lower the number of API requests) and could also be obtained by the following call
#COINMARKETCAP_ID_UIA=`curl -s -H "X-CMC_PRO_API_KEY: <insert_coinmarketcap_api_key_here>" -X GET https://sandbox-api.coinmarketcap.com/v1/cryptocurrency/info?symbol="$UIA_NAME"`
COINMARKETCAP_ID_UIA=""
#the CoinGecko ID for UIA is fixed below
COINGECKO_ID_UIA=""

##############################
# GETTING THE GENERAL VALUES #
##############################

echo "=============================================" >> $LOG_FILE_NAME
echo >> $LOG_FILE_NAME

echo "Starting a UIA price / pool update for $UIA_NAME at $(date)..." >> $LOG_FILE_NAME

WALLET_IP_AND_PORT="http://"$WALLET_IP_AND_PORT"/rpc"

#determining whether the wallet is (un)locked
IS_LOCKED=`curl -s -X POST --data "{\"jsonrpc\": \"2.0\", \"method\": \"is_locked\", \"params\": [], \"id\": 1}" $WALLET_IP_AND_PORT | jq '.result'`
if [ ! "$IS_LOCKED" ]
then
    echo "Wallet is not running or listening at $WALLET_IP_AND_PORT." >> $LOG_FILE_NAME
    echo >> $LOG_FILE_NAME ; exit
elif [ "$IS_LOCKED" != "false" ]
then
    echo "Wallet is locked." >> $LOG_FILE_NAME
    echo >> $LOG_FILE_NAME ; exit
fi

#getting all the current metadata about the UIA and USD from the system
UIA=`curl -s -X POST --data "{\"jsonrpc\": \"2.0\", \"method\": \"get_asset\", \"params\": [\"$UIA_NAME\"], \"id\": 1}" $WALLET_IP_AND_PORT`
UIA_EXISTENCE_TEST=$(echo "$UIA" | jq '.result')
if [ "$UIA_EXISTENCE_TEST" == "null" ]
then
    echo "Asset $UIA_NAME does not exist." >> $LOG_FILE_NAME
    echo >> $LOG_FILE_NAME ; exit
fi

USD=`curl -s -X POST --data "{\"jsonrpc\": \"2.0\", \"method\": \"get_asset\", \"params\": [\"USD\"], \"id\": 1}" $WALLET_IP_AND_PORT`

#determining the asset IDs for DCT, USD and UIA tokens
UIA_ID=$(echo "$UIA" | jq '.result.id')
USD_ID=$(echo "$USD" | jq '.result.id')
DCT_ID='"1.3.0"'

#truncating the quotes from UIA_MAX_SUPPLY and UIA_DESCRIPTION due to conversion to uint64_t or preventing double quotation
UIA_DESCRIPTION=$(echo "$UIA" | jq -r '.result.description')
UIA_MAX_SUPPLY=$(echo "$UIA" | jq -r '.result.options.max_supply')
UIA_IS_EXCHANGEABLE=$(echo "$UIA" | jq '.result.options.is_exchangeable')

#getting the precision values for the token and for the USD
UIA_PRECISION=$(echo "$UIA" | jq '.result.precision')
USD_PRECISION=$(echo "$USD" | jq '.result.precision')
DCT_PRECISION=8

UIA_BALANCE_LIMIT=$(echo "$UIA_BALANCE_LIMIT * 10^($UIA_PRECISION)" | bc)
DCT_BALANCE_LIMIT=$(echo "$DCT_BALANCE_LIMIT * 10^($DCT_PRECISION)" | bc)

#################
# UPDATE PRICES #
#################

if [ $UPDATE_PRICE = "true" ]
then
    echo "Starting the price update..." >> $LOG_FILE_NAME

    #getting the current prices of UIA
    UIA_PRICE=$(echo "$UIA" | jq '.result.options.core_exchange_rate')

    #getting the current base and quotes stored in the blockchain
    OLD_UIA_BASE=$(echo "$UIA_PRICE" | jq -r '.base.amount')
    OLD_UIA_QUOTE=$(echo "$UIA_PRICE" | jq -r '.quote.amount')
    TMP=$(echo "$UIA_PRICE" | jq '.base.asset_id')
    if [ "$TMP" != "$DCT_ID" ]
    then
        TMP=$OLD_UIA_BASE
        OLD_UIA_BASE=$OLD_UIA_QUOTE
        OLD_UIA_QUOTE=$TMP
    fi
    echo "Current $UIA_NAME rate is: $OLD_UIA_BASE / $OLD_UIA_QUOTE, precision: $UIA_PRECISION" >> $LOG_FILE_NAME

    #conversion rates of 1 DCT into Satoshi
    AMOUNT_DCT=10000
    AMOUNT_DCT_SATOSHI=$(echo "$AMOUNT_DCT * 10^($DCT_PRECISION)" | bc)

    if [ "$OLD_UIA_BASE" -ne "$AMOUNT_DCT_SATOSHI" ]
    then
        echo "Critical error: the base or the quote of $UIA_NAME currently set on the blockchain has to be $AMOUNT_DCT_SATOSHI. Please adjust and try again." >> $LOG_FILE_NAME
        echo >> $LOG_FILE_NAME ; exit
    fi

    #taking the live rate from CoinMarketCap and parsing out the current USD price from the CoinMarketCap JSON
    CURRENT_USD_PRICE=`curl -s -H "X-CMC_PRO_API_KEY: $COINMARKETCAP_API_KEY" -X GET https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/latest?id=$COINMARKETCAP_ID_DCT | jq '.data' | jq '."'$COINMARKETCAP_ID_DCT'".quote.USD.price'`

    if [ "$CURRENT_USD_PRICE" = "null" ] || [ "$CURRENT_USD_PRICE" == 0 ] || [ "$CURRENT_USD_PRICE" = "" ]
    then
        echo "The CoinMarketCap price info for DCT / USD is not valid. Attempting to fall back on CoinGecko..." >> $LOG_FILE_NAME
        CURRENT_USD_PRICE=`curl -s -X GET "https://api.coingecko.com/api/v3/coins/decent" -H "accept: application/json" | jq '.market_data.current_price.usd'`
        if [ "$CURRENT_USD_PRICE" = "null" ] || [ "$CURRENT_USD_PRICE" == 0 ] || [ "$CURRENT_USD_PRICE" = "" ]
        then
            echo "The CoinGecko price info for DCT / USD is not valid. Please try again later." >> $LOG_FILE_NAME
            echo >> $LOG_FILE_NAME ; exit
        fi
    fi

    #computing the new USD base and quote
    NEW_USD_BASE=$AMOUNT_DCT_SATOSHI
    NEW_USD_QUOTE=$(echo "$CURRENT_USD_PRICE * 10^($USD_PRECISION) * $AMOUNT_DCT" | bc -l)

    if [ $IS_UIA_PEGGED_TO_USD = "true" ]
    then
        #UIA that is pegged to USD
        #recalculating the UIA base and quote on the basis of the live DCT-USD rate
        NEW_UIA_BASE=$NEW_USD_BASE
        NEW_UIA_QUOTE=$(echo "$NEW_USD_QUOTE * 10^($UIA_PRECISION-$USD_PRECISION)" | bc -l)
    else
        #UIA where we need to determine the price according to the markets
        COINMARKETCAP_INFO_UIA=`curl -s -H "X-CMC_PRO_API_KEY: $COINMARKETCAP_API_KEY" -X GET https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/latest?id=$COINMARKETCAP_ID_UIA`

        CURRENT_UIA_USD_PRICE=$(echo "$COINMARKETCAP_INFO_UIA" | jq '.data' | jq '."'$COINMARKETCAP_ID_UIA'".quote.USD.price')

        if [ "$CURRENT_UIA_USD_PRICE" = "null" ] || [ "$CURRENT_UIA_USD_PRICE" == 0 ] || [ "$CURRENT_UIA_USD_PRICE" = "" ]
        then
            echo "The CoinMarketCap price info for $UIA_NAME is not valid. Attempting to fall back on CoinGecko..." >> $LOG_FILE_NAME
            CURRENT_UIA_USD_PRICE=`curl -s -X GET "https://api.coingecko.com/api/v3/coins/$COINGECKO_ID_UIA" -H "accept: application/json" | jq '.market_data.current_price.usd'`
            if [ "$CURRENT_UIA_USD_PRICE" = "null" ] || [ "$CURRENT_UIA_USD_PRICE" == 0 ] || [ "$CURRENT_UIA_USD_PRICE" = "" ]
            then
                echo "The CoinGecko price info for $UIA_NAME is not valid. Please try again later." >> $LOG_FILE_NAME
                echo >> $LOG_FILE_NAME ; exit
            fi
        fi

        CURRENT_DCT_UIA_PRICE=$(echo "$CURRENT_USD_PRICE / $CURRENT_UIA_USD_PRICE" | bc -l)
        NEW_UIA_BASE=$NEW_USD_BASE
        NEW_UIA_QUOTE=$(echo "$CURRENT_DCT_UIA_PRICE * 10^($UIA_PRECISION) * $AMOUNT_DCT" | bc -l)
    fi

    echo "New live USD rate is: $NEW_USD_BASE / $NEW_USD_QUOTE, precision: $USD_PRECISION" >> $LOG_FILE_NAME
    echo "New computed $UIA_NAME rate is: $NEW_UIA_BASE / $NEW_UIA_QUOTE, precision: $UIA_PRECISION" >> $LOG_FILE_NAME

    #price diff between the old and new quote (in integer values with precision)
    PRICE_DIFF=$(echo "$OLD_UIA_QUOTE - $NEW_UIA_QUOTE" | bc | awk '{printf "%d", $0}')
    echo "The price difference (integer) computed is: $PRICE_DIFF, precision: $UIA_PRECISION" >> $LOG_FILE_NAME

    #the minimum price difference for an updates given as 1 / $PRICE_DIFF_THRESHOLD_QUOTIENT of the current UIA quote
    PRICE_DIFF_THRESHOLD=$(echo "$OLD_UIA_QUOTE / $PRICE_DIFF_THRESHOLD_QUOTIENT" | bc -l | awk '{printf "%d", $0}')
    echo "The threshold price difference (integer) to perform an update is 1 / $PRICE_DIFF_THRESHOLD_QUOTIENT of the current price, i.e.: $PRICE_DIFF_THRESHOLD, precision: $UIA_PRECISION" >> $LOG_FILE_NAME

    #comparison to check if the price change has surpassed the threshold
    if ([ "$PRICE_DIFF" -gt 0 ] && [ "$PRICE_DIFF" -gt "$PRICE_DIFF_THRESHOLD" ]) || ([ "$PRICE_DIFF" -lt 0 ] && [ "$PRICE_DIFF" -lt "-$PRICE_DIFF_THRESHOLD" ])
    then
        echo "The price difference is exceeding the threshold. Updating the price..." >> $LOG_FILE_NAME
        #truncating the new UIA quote to avoid problems with floating-point vs integer
        NEW_UIA_QUOTE=$(echo "$NEW_UIA_QUOTE" | awk '{printf "%d", $0}')

        UIA_UPDATE_RESULT=`curl -s -X POST --data "{\"jsonrpc\": \"2.0\", \"method\": \"update_user_issued_asset\", \"params\": [\"$UIA_NAME\",\"\",\"$UIA_DESCRIPTION\",$UIA_MAX_SUPPLY,{\"base\":{\"amount\":$NEW_UIA_BASE,\"asset_id\":\"1.3.0\"},\"quote\":{\"amount\":$NEW_UIA_QUOTE,\"asset_id\":$UIA_ID}},$UIA_IS_EXCHANGEABLE,true], \"id\": 1}" http://127.0.0.1:8093/rpc`
        RESPONSE_RESULT=$(echo "$UIA_UPDATE_RESULT" | jq '. | has("result")' )
        if [ "$RESPONSE_RESULT" = true ]
        then
            echo "Result for $UIA_NAME: $UIA_UPDATE_RESULT" >> $LOG_FILE_NAME
        else
            echo "Unspecified and unhandled error. Response message: $UIA_UPDATE_PRICE" ; exit
        fi
    else
        echo "No price update needed at the moment." >> $LOG_FILE_NAME
    fi

    echo >> $LOG_FILE_NAME
fi

################
# UPDATE POOLS #
################

if [ $UPDATE_POOLS = "true" ]
then
    echo "Starting the pools update..." >> $LOG_FILE_NAME

    #limits are in nominal units( e.g. in UIAs / DCTs, not in satoshis )
    UIA_POOL_LIMIT=$(echo "$UIA_POOL_LIMIT * 10^($UIA_PRECISION)" | bc -l | awk '{printf "%d", $0}')
    DCT_POOL_LIMIT=$(echo "$DCT_POOL_LIMIT * 10^($DCT_PRECISION)" | bc -l | awk '{printf "%d", $0}')

    #threshold for the pool difference
    UIA_POOL_THRESHOLD=$(echo "$UIA_POOL_THRESHOLD * 10^($UIA_PRECISION)" | bc -l | awk '{printf "%d", $0}')
    DCT_POOL_THRESHOLD=$(echo "$DCT_POOL_THRESHOLD * 10^($DCT_PRECISION)" | bc -l | awk '{printf "%d", $0}')

    echo "$UIA_NAME pool limit: $UIA_POOL_LIMIT, precision: $UIA_PRECISION" >> $LOG_FILE_NAME
    echo "DCT pool limit: $DCT_POOL_LIMIT, precision: $DCT_PRECISION" >> $LOG_FILE_NAME
    echo "$UIA_NAME pool threshold: $UIA_POOL_THRESHOLD, precision: $UIA_PRECISION" >> $LOG_FILE_NAME
    echo "DCT pool threshold: $DCT_POOL_THRESHOLD, precision: $DCT_PRECISION" >> $LOG_FILE_NAME

    #determining the current pool values
    UIA_DYN_DATA_ID=$(echo "$UIA" | jq '.result.dynamic_asset_data_id')
    UIA_POOLS=`curl -s -X POST --data "{\"jsonrpc\": \"2.0\", \"method\": \"get_object\", \"params\": [$UIA_DYN_DATA_ID], \"id\": 1}" $WALLET_IP_AND_PORT`
    UIA_POOL=$(echo "$UIA_POOLS" | jq '.result' | jq -r '.[0].asset_pool')
    DCT_POOL=$(echo "$UIA_POOLS" | jq '.result' | jq -r '.[0].core_pool')

    #pool difference in integer values with precision
    UIA_POOL_DIFF=$(echo "$UIA_POOL_LIMIT - $UIA_POOL" | bc)
    DCT_POOL_DIFF=$(echo "$DCT_POOL_LIMIT - $DCT_POOL" | bc)

    echo "Current $UIA_NAME pool difference: $UIA_POOL_DIFF, precision: $UIA_PRECISION" >> $LOG_FILE_NAME
    echo "Current DCT pool difference: $DCT_POOL_DIFF, precision: $DCT_PRECISION" >> $LOG_FILE_NAME

    #scaling the diff values down: "full" refers to the float representation of the value
    UIA_POOL_DIFF_SCALED_FULL=$(echo "$UIA_POOL_DIFF / 10^($UIA_PRECISION)" | bc -l | awk '{printf "%.'$UIA_PRECISION'f", $0}')
    DCT_POOL_DIFF_SCALED_FULL=$(echo "$DCT_POOL_DIFF / 10^($DCT_PRECISION)" | bc -l | awk '{printf "%.'$DCT_PRECISION'f", $0}')

    echo "Current $UIA_NAME pool difference (downscaled): $UIA_POOL_DIFF_SCALED_FULL" >> $LOG_FILE_NAME
    echo "Current DCT pool difference (downscaled): $DCT_POOL_DIFF_SCALED_FULL" >> $LOG_FILE_NAME

    #determining whether the current difference exceeds the threshold
    UIA_POOL_DIFF_SCALED=0
    DCT_POOL_DIFF_SCALED=0
    if [ "$UIA_POOL_DIFF" -gt "$UIA_POOL_THRESHOLD" ]
    then
        echo "Threshold for $UIA_NAME exceeded, pool chosen for funding..." >> $LOG_FILE_NAME
        UIA_POOL_DIFF_SCALED=$UIA_POOL_DIFF_SCALED_FULL
    else
        echo "Threshold for $UIA_NAME not exceeded." >> $LOG_FILE_NAME
        UIA_POOL_DIFF=0
    fi
    if [ "$DCT_POOL_DIFF" -gt "$DCT_POOL_THRESHOLD" ]
    then
        echo "Threshold for DCT exceeded, pool chosen for funding..." >> $LOG_FILE_NAME
        DCT_POOL_DIFF_SCALED=$DCT_POOL_DIFF_SCALED_FULL
    else
        echo "Threshold for DCT not exceeded." >> $LOG_FILE_NAME
        DCT_POOL_DIFF=0
    fi

    #getting the current balances of the account in DCT and UIA
    BALANCE=`curl -s -X POST --data "{\"jsonrpc\": \"2.0\", \"method\": \"list_account_balances\", \"params\": [\"$FUNDING_ACCOUNT\"], \"id\": 1}" $WALLET_IP_AND_PORT`
    NUM_OF_ASSETS=$(echo "$BALANCE" | jq '."result"' | jq 'length' )
    for ((i=0;i<$NUM_OF_ASSETS;i++))
    do
        BALANCE_ENTRY=$(echo "$BALANCE" | jq '.result' | jq --argjson INDEX $i '.[$INDEX]')
        BALANCE_ID=$(echo "$BALANCE_ENTRY" | jq '.asset_id')
        BALANCE_VALUE=$(echo "$BALANCE_ENTRY" | jq -r '.amount')

        if [ "$BALANCE_ID" == "$DCT_ID" ]
        then
            BALANCE_DCT=$BALANCE_VALUE
        elif [ "$BALANCE_ID" == "$UIA_ID" ]
        then
            BALANCE_UIA=$BALANCE_VALUE
        fi
    done

    #checking if the assets exist for the funding account
    if [ "$BALANCE_UIA" = "" ] || [ "$BALANCE_DCT" = "" ]
    then
        echo "Asset(s) not present in the funding accounts." >> $LOG_FILE_NAME
        echo >> $LOG_FILE_NAME ; exit
    fi

    echo "Current balance for $UIA_NAME: $BALANCE_UIA, precision: $UIA_PRECISION" >> $LOG_FILE_NAME
    echo "Current balance for DCT: $BALANCE_DCT, precision: $DCT_PRECISION" >> $LOG_FILE_NAME

    #we assume that balance limits are greater than pool limits
    if [ "$UIA_POOL_THRESHOLD" -gt "$UIA_POOL_LIMIT" ] || [ "$UIA_POOL_LIMIT" -gt "$UIA_BALANCE_LIMIT" ] || [ "$DCT_POOL_THRESHOLD" -gt "$DCT_POOL_LIMIT" ] || [ "$DCT_POOL_LIMIT" -gt "$DCT_BALANCE_LIMIT" ]
    then
        echo "Wrongly set variables. Balance limits should be greater than pool limits." >> $LOG_FILE_NAME
        echo >> $LOG_FILE_NAME ; exit
    fi

    #if we have less UIA funding than the pool limit
    if [ "$BALANCE_UIA" -lt "$UIA_POOL_LIMIT" ]
    then
        echo "Critical warning: the $UIA_NAME funding available is critically low!" >> $LOG_FILE_NAME
        if [ "$UIA_POOL_DIFF" -gt "$BALANCE_UIA" ]
        then
            echo "Insufficient $UIA_NAME balance!" >> $LOG_FILE_NAME
            echo >> $LOG_FILE_NAME ; exit
        fi
    #if we have less UIA funding than the balance limit
    elif [ "$BALANCE_UIA" -lt "$UIA_BALANCE_LIMIT" ]
    then
        echo "Warning: the $UIA_NAME funding available is low!" >> $LOG_FILE_NAME
    fi

    # if we have less DCT funding than the pool limit
    if [ "$BALANCE_DCT" -lt "$DCT_POOL_LIMIT" ]
    then
        echo "Critical warning: the DCT funding available is critically low!" >> $LOG_FILE_NAME
        if [ "$DCT_POOL_DIFF" -gt "$BALANCE_DCT" ]
        then
            echo "Insufficient DCT balance!" >> $LOG_FILE_NAME
            echo >> $LOG_FILE_NAME ; exit
        fi
    # if we have less DCT funding than the balance limit
    elif [ "$BALANCE_DCT" -lt "$DCT_BALANCE_LIMIT" ]
    then
        echo "Warning: the DCT funding available is low!" >> $LOG_FILE_NAME
    fi

    if [ "$UIA_POOL_DIFF" -gt 0 ] || [ "$DCT_POOL_DIFF" -gt 0 ]
    then
        echo "Funding..." >> $LOG_FILE_NAME
        echo "Funding amount in $UIA_NAME: $UIA_POOL_DIFF_SCALED" >> $LOG_FILE_NAME
        echo "Funding amount in DCT: $DCT_POOL_DIFF_SCALED" >> $LOG_FILE_NAME
        UIA_FUNDING_RESULT=`curl -s -X POST --data "{\"jsonrpc\": \"2.0\", \"method\": \"fund_asset_pools\", \"params\": [\"$FUNDING_ACCOUNT\",$UIA_POOL_DIFF_SCALED,\"$UIA_NAME\",$DCT_POOL_DIFF_SCALED,\"DCT\",true], \"id\": 1}" http://127.0.0.1:8093/rpc`
        echo "Result for $UIA_NAME: $UIA_FUNDING_RESULT" >> $LOG_FILE_NAME
        echo
    else
        echo "No pool funding needed." >> $LOG_FILE_NAME
    fi
    echo >> $LOG_FILE_NAME
fi

echo "Script finished successfully at $(date)." >> $LOG_FILE_NAME

exit
