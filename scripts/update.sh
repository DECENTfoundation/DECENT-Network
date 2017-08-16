echo "please vote phinx!"
cd ~/dev/DECENTfoundation/DECENT-Network
git pull
cd ~/dev/DECENTfoundation/DECENT-Network-build
cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug ~/dev/DECENTfoundation/DECENT-Network
cmake --build . --target all -- -j -l 3.0
cmake --build . --target install
cd ~/dev/DECENTfoundation/DECENT-Network-build/artifacts/prefix/bin/
rm -rf ~/decent
mkdir ~/decent
cp ~/dev/DECENTfoundation/DECENT-Network-build/artifacts/prefix/bin/* ~/decent/
cd ~/decent
ls
./decentd --replay-blockchain
echo "all updated!"

