# gui_wallet

First setup 'BOOST_ROOT' and 'DECENT_ROOT' environmental variables corectly.
 1. 'BOOST_ROOT' should refer to the directory, where 'lib' and 'include' 
     directories for BOOST librarie and includes are located
 2. 'DECENT_ROOT' is the directory from where static libraries and headers
     will be found for compilation. In the case if this environmental variable
     is not set qt sets default value: '../../../../DECENT-Network'. Please have
     a look to the 'projects/gui_wallet/gui_wallet_qt/gui_wallet.pro' to see the
     usage of this variable
     
After environment is prepared, Use following command to compile the 'gui_wallet' application

$. scripts/build_gui_wallet.bat

# If there is an error, then please try following
  steps, to see where hapens error

1.  $cd projects/gui_wallet/gui_wallet_qt
2.  $qmake gui_wallet.pro
3.  $make

And then from UNIX like systems the GUI can be launched by
    $../../../sys/`lsb_release -c | cut -f 2`/bin/gui_wallet
    
    
