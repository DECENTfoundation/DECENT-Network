Intro for new developers
------------------------
 
This is a quick introduction to get new developers up to speed on Decent.


Building Decent [![Build Status](https://travis-ci.com/DECENTfoundation/DECENT-Network.svg?token=xwFm8bxNLqiJV3NaNYgy&branch=develop)](https://travis-ci.com/DECENTfoundation/DECENT-Network)
---------------

### Installing prerequisites in Linux

For Ubuntu 18.04 LTS, execute in console:

     sudo apt-get update
     sudo apt-get install build-essential autotools-dev automake autoconf libtool make cmake checkinstall gcc g++ clang flex bison doxygen gettext git qt5-default libreadline-dev libcrypto++-dev libgmp-dev libssl-dev libncurses5-dev libboost-all-dev libcurl4-openssl-dev python-dev libicu-dev libbz2-dev

For Ubuntu 16.04 LTS, execute in console:

     sudo apt-get update
     sudo apt-get install build-essential autotools-dev automake autoconf libtool make cmake checkinstall realpath gcc g++ clang flex bison doxygen gettext git qt5-default libreadline-dev libcrypto++-dev libgmp-dev libssl-dev libncurses5-dev libboost-all-dev libcurl4-openssl-dev python-dev libicu-dev libbz2-dev

> Note, that the default versions of Boost and CMake installed in Ubuntu 16.04 LTS are too old and not supported. In order to install a supported ones, in addition to the common commands above, execute the following in console (in the same shell session, where you are going to build Decent itself):

    # Download and build Boost 1.65.1
     mkdir -p ~/dev/DECENTfoundation/DECENT-Network-third-party
     cd ~/dev/DECENTfoundation/DECENT-Network-third-party
     wget https://sourceforge.net/projects/boost/files/boost/1.65.1/boost_1_65_1.tar.gz
     tar xvf boost_1_65_1.tar.gz
     mkdir boost
     cd boost_1_65_1
     export BOOST_ROOT=$(realpath ../boost)
     ./bootstrap.sh --prefix=$BOOST_ROOT
     ./b2 install
     cd ..
     rm -rf boost_1_65_1 boost_1_65_1.tar.gz

    # Download and build Cmake 3.10.2
     cd ~/dev/DECENTfoundation/DECENT-Network-third-party
     wget https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz
     tar xvf cmake-3.10.2.tar.gz
     mkdir cmake
     cd cmake-3.10.2
     export CMAKE_ROOT=$(realpath ../cmake)
     ./configure --prefix=$CMAKE_ROOT
     make install
     export PATH=$CMAKE_ROOT/bin:$PATH
     cd ..
     rm -rf cmake-3.10.2 cmake-3.10.2.tar.gz

> At this point, CMake configure should find the Boost distribution in the exported `$BOOST_ROOT`.

For Fedora 28 or later, execute in console:

     sudo dnf clean metadata
     sudo dnf install automake autoconf libtool make cmake gcc flex bison doxygen gettext-devel git qt5-qtbase-devel readline-devel cryptopp-devel gmp-devel openssl-devel libcurl-devel ncurses-devel boost-devel boost-static

### Installing prerequisites in macOS

* Install Xcode and Command Line Tools as described in http://railsapps.github.io/xcode-command-line-tools.html
* Install Homebrew, see http://brew.sh

Then, execute in console:

    $ brew doctor
    $ brew tap homebrew/versions
    $ brew update
    $ brew install automake autoconf libtool cmake boost qt5 cryptopp doxygen byacc flex gettext git pbc gmp ipfs openssl readline

### Obtaining the sources, building, and installing Decent in Unix (macOS or Linux)

After all the prerequisites are installed, execute the following commands in console, in order to clone the repo, build, and install/stage Decent:

    # Clone the repo.
     mkdir -p ~/dev/DECENTfoundation
     cd ~/dev/DECENTfoundation
     git clone https://github.com/DECENTfoundation/DECENT-Network.git
     cd DECENT-Network
     git submodule update --init --recursive

    # Build and install Decent.
     mkdir -p ~/dev/DECENTfoundation/DECENT-Network-build
     cd ~/dev/DECENTfoundation/DECENT-Network-build
     cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Release ~/dev/DECENTfoundation/DECENT-Network
     cmake --build . --target all -- -j -l 3.0
     cmake --build . --target install

> Note that, in case of "Unix Makefiles" CMake generator, the last two commands are equivalent to:
> 
>     $ make -j -l 3.0
>     $ make install

By this time you should have Decent files installed at `~/dev/DECENTfoundation/DECENT-Network-build/artifacts/prefix` directory. You can specify any other custom install prefix for `cmake` during the initial configuration, for example, by adding `-D CMAKE_INSTALL_PREFIX=~/dev/DECENTfoundation/DECENT-Network-prefix` to the command line.

You can use any path instead of `~/dev/DECENTfoundation` in the steps above.

You can use Xcode, or any other CMake generator, and then, if it is an IDE generator, instead of building and installing via `cmake` in terminal, open the generated project/solution file in the corresponding IDE and perform `ALL_BUILD` and `INSTALL` (or `install`) actions from there.

### Installing prerequisites, obtaining the sources, building, and installing Decent in Windows

TODO


Starting Decent
---------------

> In the commands below, change `~/dev/DECENTfoundation/DECENT-Network-build/artifacts/prefix` to `~/dev/DECENTfoundation/DECENT-Network-prefix` or to any other install location, that you specified during initial configuration.

On first run `decentd` will create `.decent` in the home directory, if doesn't exist already.

    $ ~/dev/DECENTfoundation/DECENT-Network-build/artifacts/prefix/bin/decentd

Optionally, now press Ctrl-C to stop `decentd`. You can edit configuration in `~/.decent/data/decentd/config.ini`.

Then, run the decent daemon again:

    $ ~/dev/DECENTfoundation/DECENT-Network-build/artifacts/prefix/bin/decentd 
    
This will launch the decent daemon node with the default genesis. 

Then, in a separate console, start the command-line wallet by executing:

    $ cd ~/dev/DECENTfoundation/DECENT-Network-working-dir
    $ ~/dev/DECENTfoundation/DECENT-Network-build/artifacts/prefix/bin/cli_wallet

To set your initial password to `mypassword`, execute:

    >>> set_password mypassword
    >>> unlock mypassword

To import your account keys, execute:

    >>> import_key [name] [private_wif_key]


Decent daemon
--------------

The role of the decent daemon is to broadcast transactions, download blocks, and optionally sign them.

    $ ~/dev/DECENTfoundation/DECENT-Network-build/artifacts/prefix/bin/decentd --rpc-endpoint 127.0.0.1:8090 --enable-stale-production -w '"1.4.0"' 

Testing Decent
--------------

Seeder plugin is responsible for automatically announce seeder's capablity, downloading content, seeding it and distributing keys. In order to enable it follow these steps:
1. Generarate El-Gamal keys using cli_wallet command (first one is private, second one is public)

        generate_el_gamal_keys

2. Add parameters to the decent daemon

        --seeder [account-id] --seeder-private-key [private_wif_key] --content-private-key [el_gamal_private_key] --packages-path [path] --seeding-price [price] --free-space [free-space]
    
    where [account-id] is one of your accounts, [private_wif_key] corresponding active key, [el_gamal_private_key] is the generated El-Gamal key, [path] is a filesystem location with at least [space] Megabytes available, and [price] is publishing price per MB per day, in satoshis.


Using the API
-------------

We provide several different API's.  Each API has its own ID.
When running `decentd`, initially two API's are available:
API 0 provides read-only access to the database, while API 1 is
used to login and gain access to additional, restricted API's.

Here is an example using `wscat` package from `npm` for websockets:

    $ npm install -g wscat
    $ wscat -c ws://127.0.0.1:8090
    > {"id":1, "method":"call", "params":[0,"get_accounts",[["1.2.0"]]]}
    < {"id":1,"result":[{"id":"1.2.0","annotations":[],"membership_expiration_date":"1969-12-31T23:59:59","registrar":"1.2.0","referrer":"1.2.0","lifetime_referrer":"1.2.0","network_fee_percentage":2000,"lifetime_referrer_fee_percentage":8000,"referrer_rewards_percentage":0,"name":"committee-account","owner":{"weight_threshold":1,"account_auths":[],"key_auths":[],"address_auths":[]},"active":{"weight_threshold":6,"account_auths":[["1.2.5",1],["1.2.6",1],["1.2.7",1],["1.2.8",1],["1.2.9",1],["1.2.10",1],["1.2.11",1],["1.2.12",1],["1.2.13",1],["1.2.14",1]],"key_auths":[],"address_auths":[]},"options":{"memo_key":"GPH1111111111111111111111111111111114T1Anm","voting_account":"1.2.0","num_miner":0,"num_committee":0,"votes":[],"extensions":[]},"statistics":"2.7.0","whitelisting_accounts":[],"blacklisting_accounts":[]}]}

We can do the same thing using an HTTP client such as `curl` for API's which do not require login or other session state:

    $ curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0, "get_accounts", [["1.2.0"]]], "id": 1}' http://127.0.0.1:8090/rpc
    {"id":1,"result":[{"id":"1.2.0","annotations":[],"membership_expiration_date":"1969-12-31T23:59:59","registrar":"1.2.0","referrer":"1.2.0","lifetime_referrer":"1.2.0","network_fee_percentage":2000,"lifetime_referrer_fee_percentage":8000,"referrer_rewards_percentage":0,"name":"committee-account","owner":{"weight_threshold":1,"account_auths":[],"key_auths":[],"address_auths":[]},"active":{"weight_threshold":6,"account_auths":[["1.2.5",1],["1.2.6",1],["1.2.7",1],["1.2.8",1],["1.2.9",1],["1.2.10",1],["1.2.11",1],["1.2.12",1],["1.2.13",1],["1.2.14",1]],"key_auths":[],"address_auths":[]},"options":{"memo_key":"GPH1111111111111111111111111111111114T1Anm","voting_account":"1.2.0","num_miner":0,"num_committee":0,"votes":[],"extensions":[]},"statistics":"2.7.0","whitelisting_accounts":[],"blacklisting_accounts":[]}]}

API 0 is accessible using regular JSON-RPC:

    $ curl --data '{"jsonrpc": "2.0", "method": "get_accounts", "params": [["1.2.0"]], "id": 1}' http://127.0.0.1:8090/rpc


Accessing restricted API's
--------------------------

You can restrict API's to particular users by specifying an `api-access` file in `config.ini`.  Here is an example `apiaccess` file which allows
user `decent` with password `pwd` to access four different API's, while allowing any other user to access the three public API's
necessary to use the wallet:

    {
       "permission_map" :
       [
          [
             "decent",
             {
                "password_hash_b64" : "W/wGhp3F9QOPwyCCpAPSQTrRnoQJi7IrI98ttwCJwCE=",
                "password_salt_b64" : "8Bd7FkJHI/8=",
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
    `types.hpp`.

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
    publicly accessible.  It's fine if your `decentd` allows the general public to query
    the database or broadcast transactions (in fact, this is how the hosted web UI works).  It's
    less fine if your `decent` allows the general public to control which p2p nodes it's
    connecting to.  Therefore the API to add p2p connections needs to be set up with proper access
    controls.
