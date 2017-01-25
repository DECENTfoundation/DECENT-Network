Intro for new developers
------------------------

This is a quick introduction to get new developers up to speed on Decent.


Building Decent [![Build Status](https://travis-ci.com/DECENTfoundation/DECENT-Network.svg?token=xwFm8bxNLqiJV3NaNYgy&branch=develop)](https://travis-ci.com/DECENTfoundation/DECENT-Network)
---------------

### Installing all prerequisites in Linux

For Ubuntu 16.04 LTS or later, execute in console:

    $ sudo apt-get update
    $ sudo apt-get install build-essential autotools-dev automake autoconf libtool make cmake gcc g++ clang flex bison doxygen gettext git google-perftools libgoogle-perftools-dev libreadline-dev libcrypto++-dev libgmp-dev libdb-dev libdb++-dev libssl-dev libncurses5-dev libboost-all-dev

> Note, that the default version of Boost installed in Ubuntu 16.10 is not supported. In order to install a supported one, execute in console:
> 
>     $ sudo apt-get remove libboost-all-dev
>     $ sudo apt-get autoremove
>     $ sudo apt-get install libboost1.60-all-dev


For Fedora 25 or later, execute in console:

    $ sudo yum makecache
    $ sudo yum install automake autoconf libtool make cmake gcc clang flex bison doxygen gettext-devel git google-perftools google-perftools-devel readline-devel cryptopp-devel gmp-devel libdb-devel libdb-cxx-devel openssl-devel ncurses-devel boost-devel boost-static


### Installing all prerequisites in macOS

* Install Xcode and Command Line Tools, see http://railsapps.github.io/xcode-command-line-tools.html
* Install Homebrew, see http://brew.sh

Then, execute in console:

    $ brew tap homebrew/versions
    $ brew update
    $ brew install automake autoconf libtool cmake berkeley-db boost160 cryptopp libtorrent-rasterbar doxygen byacc flex gettext git pbc gmp ipfs openssl readline gperftools


### Obtaining the sources, building, and installing Decent in Unix (macOS or Linux)

After all the prerequisites are installed, execute the following commands in console, in order to clone the repo, build, and install/stage Decent:

    # Clone the repo.
    $ mkdir -p ~/dev/DECENTfoundation
    $ cd ~/dev/DECENTfoundation
    $ git clone git://github.com/DECENTfoundation/DECENT-Network.git
    $ cd DECENT-Network
    $ git submodule update --init --recursive

    # Build and install Decent.
    $ mkdir -p ~/dev/DECENTfoundation/build/DECENT-Network
    $ cd ~/dev/DECENTfoundation/build/DECENT-Network
    $ cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug ~/dev/DECENTfoundation/DECENT-Network
    $ cmake --build . --target all -- -j -l 3.0
    $ cmake --build . --target install

By this time you should have Decent files installed at `~/dev/DECENTfoundation/build/DECENT-Network/artifacts/prefix` or `~/dev/DECENTfoundation/build/DECENT-Network/install` directory.

You can use any path instead of `~/dev/DECENTfoundation` directory in the steps above.

> Note that, in case of "Unix Makefiles" CMake generator, the last two commands are equivalent to:
> 
>     $ make -j -l 3.0
>     $ make install


You can use Xcode, or any other CMake generator, and then, if it is an IDE generator, instead of building and installing via `cmake` in terminal, open the generated project/solution file in the corresponding IDE and perform `ALL_BUILD` and `INSTALL` or `install` actions from there.

### Installing all prerequisites, obtaining the sources, building, and installing Decent in Windows

TODO


Starting Decent
---------------

On first run `witness_node` will create `witness_node_data_dir` in the current working directory, if doesn't exist already.

    $ mkdir -p ~/dev/DECENTfoundation/working_dir
    $ cd ~/dev/DECENTfoundation/working_dir
    $ ~/dev/DECENTfoundation/build/artifacts/prefix/bin/witness_node

Now press Ctrl-C to stop `witness_node`.

Edit `~/dev/DECENTfoundation/working_dir/witness_node_data_dir/config.ini` to contain the following lines:

    seed-node = 185.8.165.21:33142
    rpc-endpoint = 127.0.0.1:8090

Then, run the witness node again:

    $ cd ~/dev/DECENTfoundation/working_dir
    $ ~/dev/DECENTfoundation/build/artifacts/prefix/bin/witness_node --genesis-json ~/dev/DECENTfoundation/DECENT-Network/genesis.json

This will launch the witness node with the default genesis. 

Then, in a separate console, start the command-line wallet by executing:

    $ cd ~/dev/DECENTfoundation/working_dir
    $ ~/dev/DECENTfoundation/build/artifacts/prefix/bin/cli_wallet

To set your initial password to `password`, execute:

    >>> set_password password
    >>> unlock password

To import your account keys, execute:

    >>> import_key [name] [private_wif_key]

The list of the test accounts is here:

    suggest_brain_key 
    {
      "brain_priv_key": "BACCAE CARDOON BENI RIPSACK SORRA ARDUOUS WEEDERY UPCHOKE APERT ENZYME ARTISAN JAPERY RAFFIA MIJL PLEROME CONCHY",
      "wif_priv_key": "5Kgs83SJvdTVmeRYBAE1WNyp8FLWpxYf1hsX5oejpqNvC7RLuwU",
      "pub_key": "DCT7MQp1bzYqHEBifH2YuTy35ZYiNkP8eJJsVBVRG585xChJDc25F"
    }
    vazgen2


    suggest_brain_key 
    {
      "brain_priv_key": "HUGSOME SHELLED COADAPT CYCAD SLINE BIFARA BEAD FIPPLE NUDITY EPOPTIC LABROSE SHINDIG OWL EGGHEAD VALLAR RESNUB",
      "wif_priv_key": "5K1LBKuR77tJLfAwNRWrJ5okCD4xfkUR7JABTVBhaxZoE9tqsPD",
      "pub_key": "DCT6vuWZiVB5w6LWhRhoVLjvvrDpxw5CwuWQNu1oSeeDCybRAtDQT"
    }
    artazor


    suggest_brain_key 
    {
      "brain_priv_key": "ASTEISM DOGWOOD CUMBRE HERBIST INULASE SQUOZE SILEX PARAPH PIGGING SHIPLAP ENCRISP MYCELIA LEISURE ENDEVIL LATROBE DUCKERY",
      "wif_priv_key": "5KZC42A8XHSSLE26xqAymkZiyqjfvhK9wNbQnrWQfyzfMeXceUV",
      "pub_key": "DCT8KvRax7aDCdsdFLmdQrJ8BxdoK8hduQQyudC33Zf1pu4q67db2"
    }
    davit

A list of CLI wallet commands is available [here](https://github.com/cryptonomex/graphene/blob/master/libraries/wallet/include/graphene/wallet/wallet.hpp).


Witness node
------------

The role of the witness node is to broadcast transactions, download blocks, and optionally sign them.

    $ ~/dev/DECENTfoundation/build/artifacts/prefix/bin/witness_node --rpc-endpoint 127.0.0.1:8090 --enable-stale-production -w '"1.6.0"' '"1.6.1"' '"1.6.2"' '"1.6.3"' '"1.6.4"' '"1.6.5"' '"1.6.6"' '"1.6.7"' '"1.6.8"' '"1.6.9"' '"1.6.10"' '"1.6.11"' '"1.6.12"' '"1.6.13"' '"1.6.14"' '"1.6.15"' '"1.6.16"' '"1.6.17"' '"1.6.18"' '"1.6.19"' '"1.6.20"' '"1.6.21"' '"1.6.22"' '"1.6.23"' '"1.6.24"' '"1.6.25"' '"1.6.26"' '"1.6.27"' '"1.6.28"' '"1.6.29"' '"1.6.30"' '"1.6.31"' '"1.6.32"' '"1.6.33"' '"1.6.34"' '"1.6.35"' '"1.6.36"' '"1.6.37"' '"1.6.38"' '"1.6.39"' '"1.6.40"' '"1.6.41"' '"1.6.42"' '"1.6.43"' '"1.6.44"' '"1.6.45"' '"1.6.46"' '"1.6.47"' '"1.6.48"' '"1.6.49"' '"1.6.50"' '"1.6.51"' '"1.6.52"' '"1.6.53"' '"1.6.54"' '"1.6.55"' '"1.6.56"' '"1.6.57"' '"1.6.58"' '"1.6.59"' '"1.6.60"' '"1.6.61"' '"1.6.62"' '"1.6.63"' '"1.6.64"' '"1.6.65"' '"1.6.66"' '"1.6.67"' '"1.6.68"' '"1.6.69"' '"1.6.70"' '"1.6.71"' '"1.6.72"' '"1.6.73"' '"1.6.74"' '"1.6.75"' '"1.6.76"' '"1.6.77"' '"1.6.78"' '"1.6.79"' '"1.6.80"' '"1.6.81"' '"1.6.82"' '"1.6.83"' '"1.6.84"' '"1.6.85"' '"1.6.86"' '"1.6.87"' '"1.6.88"' '"1.6.89"' '"1.6.90"' '"1.6.91"' '"1.6.92"' '"1.6.93"' '"1.6.94"' '"1.6.95"' '"1.6.96"' '"1.6.97"' '"1.6.98"' '"1.6.99"' '"1.6.100"'


Testing Decent
--------------

TODO


### Coverage testing

Check how much code is covered by unit tests, using gcov/lcov (see http://ltp.sourceforge.net/coverage/lcov.php ).

    $ mkdir -p ~/dev/DECENTfoundation/build
    $ cd ~/dev/DECENTfoundation/build
    $ cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug -D ENABLE_COVERAGE_TESTING=TRUE ~/dev/DECENTfoundation/DECENT-Network
    $ cmake --build . --target all -- -j
    $ cmake --build . --target install
    
    $ lcov --capture --initial --directory . --output-file base.info --no-external
    $ libraries/contrib/fc/bloom_test
    $ libraries/contrib/fc/task_cancel_test
    $ libraries/contrib/fc/api
    $ libraries/contrib/fc/blind
    $ libraries/contrib/fc/ecc_test test
    $ libraries/contrib/fc/real128_test
    $ libraries/contrib/fc/lzma_test README.md
    $ libraries/contrib/fc/ntp_test
    $ tests/intense_test
    $ tests/app_test
    $ tests/chain_bench
    $ tests/chain_test
    $ tests/performance_test
    $ lcov --capture --directory . --output-file test.info --no-external
    $ lcov --add-tracefile base.info --add-tracefile test.info --output-file total.info
    $ lcov -o interesting.info -r total.info libraries/contrib/fc/vendor/\* libraries/contrib/fc/tests/\* tests/\*
    $ mkdir -p lcov
    $ genhtml interesting.info --output-directory lcov --prefix `pwd`

Now open `lcov/index.html` in a browser.


### Unit testing

We use the Boost.Test unit test framework for unit testing.  Most unit
tests reside in the `chain_test` build target.


### Running specific tests

`$ tests/chain_tests -t block_tests/name_of_test`


Using the API
-------------

We provide several different API's.  Each API has its own ID.
When running `witness_node`, initially two API's are available:
API 0 provides read-only access to the database, while API 1 is
used to login and gain access to additional, restricted API's.

Here is an example using `wscat` package from `npm` for websockets:

    $ npm install -g wscat
    $ wscat -c ws://127.0.0.1:8090
    > {"id":1, "method":"call", "params":[0,"get_accounts",[["1.2.0"]]]}
    < {"id":1,"result":[{"id":"1.2.0","annotations":[],"membership_expiration_date":"1969-12-31T23:59:59","registrar":"1.2.0","referrer":"1.2.0","lifetime_referrer":"1.2.0","network_fee_percentage":2000,"lifetime_referrer_fee_percentage":8000,"referrer_rewards_percentage":0,"name":"committee-account","owner":{"weight_threshold":1,"account_auths":[],"key_auths":[],"address_auths":[]},"active":{"weight_threshold":6,"account_auths":[["1.2.5",1],["1.2.6",1],["1.2.7",1],["1.2.8",1],["1.2.9",1],["1.2.10",1],["1.2.11",1],["1.2.12",1],["1.2.13",1],["1.2.14",1]],"key_auths":[],"address_auths":[]},"options":{"memo_key":"GPH1111111111111111111111111111111114T1Anm","voting_account":"1.2.0","num_witness":0,"num_committee":0,"votes":[],"extensions":[]},"statistics":"2.7.0","whitelisting_accounts":[],"blacklisting_accounts":[]}]}

We can do the same thing using an HTTP client such as `curl` for API's which do not require login or other session state:

    $ curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0, "get_accounts", [["1.2.0"]]], "id": 1}' http://127.0.0.1:8090/rpc
    {"id":1,"result":[{"id":"1.2.0","annotations":[],"membership_expiration_date":"1969-12-31T23:59:59","registrar":"1.2.0","referrer":"1.2.0","lifetime_referrer":"1.2.0","network_fee_percentage":2000,"lifetime_referrer_fee_percentage":8000,"referrer_rewards_percentage":0,"name":"committee-account","owner":{"weight_threshold":1,"account_auths":[],"key_auths":[],"address_auths":[]},"active":{"weight_threshold":6,"account_auths":[["1.2.5",1],["1.2.6",1],["1.2.7",1],["1.2.8",1],["1.2.9",1],["1.2.10",1],["1.2.11",1],["1.2.12",1],["1.2.13",1],["1.2.14",1]],"key_auths":[],"address_auths":[]},"options":{"memo_key":"GPH1111111111111111111111111111111114T1Anm","voting_account":"1.2.0","num_witness":0,"num_committee":0,"votes":[],"extensions":[]},"statistics":"2.7.0","whitelisting_accounts":[],"blacklisting_accounts":[]}]}

API 0 is accessible using regular JSON-RPC:

    $ curl --data '{"jsonrpc": "2.0", "method": "get_accounts", "params": [["1.2.0"]], "id": 1}' http://127.0.0.1:8090/rpc


Accessing restricted API's
--------------------------

You can restrict API's to particular users by specifying an `apiaccess` file in `config.ini`.  Here is an example `apiaccess` file which allows
user `bytemaster` with password `supersecret` to access four different API's, while allowing any other user to access the three public API's
necessary to use the wallet:

    {
       "permission_map" :
       [
          [
             "bytemaster",
             {
                "password_hash_b64" : "9e9GF7ooXVb9k4BoSfNIPTelXeGOZ5DrgOYMj94elaY=",
                "password_salt_b64" : "INDdM6iCi/8=",
                "allowed_apis" : ["database_api", "network_broadcast_api", "history_api", "network_node_api"]
             }
          ],
          [
             "*",
             {
                "password_hash_b64" : "*",
                "password_salt_b64" : "*",
                "allowed_apis" : ["database_api", "network_broadcast_api", "history_api"]
             }
          ]
       ]
    }

Passwords are stored in `base64` as salted `sha256` hashes.  A simple Python script, `saltpass.py` is avaliable to obtain hash and salt values from a password.
A single asterisk `"*"` may be specified as username or password hash to accept any value.

With the above configuration, here is an example of how to call `add_node` from the `network_node` API:

    {"id":1, "method":"call", "params":[1,"login",["bytemaster", "supersecret"]]}
    {"id":2, "method":"call", "params":[1,"network_node",[]]}
    {"id":3, "method":"call", "params":[2,"add_node",["127.0.0.1:9090"]]}

Note, the call to `network_node` is necessary to obtain the correct API identifier for the network API.  It is not guaranteed that the network API identifier will always be `2`.

Since the `network_node` API requires login, it is only accessible over the websocket RPC.  Our `doxygen` documentation contains the most up-to-date information
about API's for the [witness node](https://bitshares.github.io/doxygen/namespacegraphene_1_1app.html) and the
[wallet](https://bitshares.github.io/doxygen/classgraphene_1_1wallet_1_1wallet__api.html).
If you want information which is not available from an API, it might be available
from the [database](https://bitshares.github.io/doxygen/classgraphene_1_1chain_1_1database.html);
it is fairly simple to write API methods to expose database methods.


Running private testnet
-----------------------

See the [documentation](https://github.com/cryptonomex/graphene/wiki/private-testnet) if you want to run a private testnet.


Questions
---------

- Is there a way to generate help with parameter names and method descriptions?

    Yes. Documentation of the code base, including APIs, can be generated using Doxygen. Simply run `doxygen` in this directory.

    If both Doxygen and perl are available in your build environment, the CLI wallet's `help` and `gethelp`
    commands will display help generated from the doxygen documentation.

    If your CLI wallet's `help` command displays descriptions without parameter names like
        `signed_transaction transfer(string, string, string, string, string, bool)`
    it means CMake was unable to find Doxygen or perl during configuration.  If found, the
    output should look like this:
        `signed_transaction transfer(string from, string to, string amount, string asset_symbol, string memo, bool broadcast)`

- Is there a way to allow external program to drive `cli_wallet` via websocket, JSONRPC, or HTTP?

    Yes. External programs may connect to the CLI wallet and make its calls over a websockets API. To do this, run the wallet in
    server mode, i.e. `cli_wallet -s "127.0.0.1:9999"` and then have the external program connect to it over the specified port
    (in this example, port 9999).

- Is there a way to access methods which require login over HTTP?

    No.  Login is inherently a stateful process (logging in changes what the server will do for certain requests, that's kind
    of the point of having it).  If you need to track state across HTTP RPC calls, you must maintain a session across multiple
    connections.  This is a famous source of security vulnerabilities for HTTP applications.  Additionally, HTTP is not really
    designed for "server push" notifications, and we would have to figure out a way to queue notifications for a polling client.

    Websockets solves all these problems.  If you need to access Graphene's stateful methods, you need to use Websockets.

- What is the meaning of `a.b.c` numbers?

    The first number specifies the *space*.  Space 1 is for protocol objects, 2 is for implementation objects.
    Protocol space objects can appear on the wire, for example in the binary form of transactions.
    Implementation space objects cannot appear on the wire and solely exist for implementation
    purposes, such as optimization or internal bookkeeping.

    The second number specifies the *type*.  The type of the object determines what fields it has.  For a
    complete list of type ID's, see `enum object_type` and `enum impl_object_type` in
    [types.hpp](https://github.com/cryptonomex/graphene/blob/master/libraries/chain/include/graphene/chain/protocol/types.hpp).

    The third number specifies the *instance*.  The instance of the object is different for each individual
    object.

- The answer to the previous question was really confusing.  Can you make it clearer?

    All account ID's are of the form `1.2.x`.  If you were the 9735th account to be registered,
    your account's ID will be `1.2.9735`.  Account `0` is special (it's the "committee account,"
    which is controlled by the committee members and has a few abilities and restrictions other accounts
    do not).

    All asset ID's are of the form `1.3.x`.  If you were the 29th asset to be registered,
    your asset's ID will be `1.3.29`.  Asset `0` is special (it's BTS, which is considered the "core asset").

    The first and second number together identify the kind of thing you're talking about (`1.2` for accounts,
    `1.3` for assets).  The third number identifies the particular thing.

- How do I get the `network_add_nodes` command to work?  Why is it so complicated?

    You need to follow the instructions in the "Accessing restricted API's" section to
    allow a username/password access to the `network_node` API.  Then you need
    to pass the username/password to the `cli_wallet` on the command line or in a config file.

    It's set up this way so that the default configuration is secure even if the RPC port is
    publicly accessible.  It's fine if your `witness_node` allows the general public to query
    the database or broadcast transactions (in fact, this is how the hosted web UI works).  It's
    less fine if your `witness_node` allows the general public to control which p2p nodes it's
    connecting to.  Therefore the API to add p2p connections needs to be set up with proper access
    controls.
