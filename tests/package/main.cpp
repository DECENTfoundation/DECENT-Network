
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/crypto/sha512.hpp>

#include <graphene/app/api.hpp>

#include <graphene/package/package.hpp>
#include <boost/filesystem.hpp>

#ifndef WIN32
#include <csignal>
#endif

using namespace graphene::app;
using namespace graphene::package;

using namespace std;
namespace bpo = boost::program_options;


int main( int argc, char** argv )
{
	try
	{
		bpo::options_description cli_options("Graphene package test");
		cli_options.add_options()
			("help,h", "Print this help message and exit.")
			("content", bpo::value<boost::filesystem::path>(), "Directory or file to use as package content")
			("samples", bpo::value<boost::filesystem::path>()->default_value(""), "Directory containing samples")

			("package", bpo::value<boost::filesystem::path>(), "Package directory")
			("output", bpo::value<boost::filesystem::path>(), "Output directory")
			
			("key", bpo::value<string>(), "key to encrypt/decrypt package")
			("extract,e", bpo::value<bool>(), "If true program will extract content of package to output")
			;

		bpo::variables_map options;

		try
		{
			boost::program_options::store( boost::program_options::parse_command_line(argc, argv, cli_options), options );
		}
		catch (const boost::program_options::error& e)
		{
			std::cerr << "package_test:	error parsing command line: " << e.what() << "\n";
			return 1;
		}

		if( options.count("help") )
		{
			std::cout << cli_options << "\n";
			return 0;
		}

		boost::filesystem::path contentPath, samplesPath, packagePath, outputPath;

		if( options.count("content") ) {
			contentPath = options["content"].as<boost::filesystem::path>();
			if( contentPath.is_relative() )
				contentPath = boost::filesystem::current_path() / contentPath;
		}

		if( options.count("samples") ) {
			samplesPath = options["samples"].as<boost::filesystem::path>();
			if( samplesPath.is_relative() )
				samplesPath = boost::filesystem::current_path() / samplesPath;
		}

		if( options.count("package") ) {
			packagePath = options["package"].as<boost::filesystem::path>();
			if( packagePath.is_relative() )
				packagePath = boost::filesystem::current_path() / packagePath;
		}

		if( options.count("output") ) {
			outputPath = options["output"].as<boost::filesystem::path>();
			if( outputPath.is_relative() )
				outputPath = boost::filesystem::current_path() / outputPath;
		}

		string key;
		if( options.count("key") ) {
			key = options["key"].as<string>();
		}

		bool extract = false;
		if( options.count("extract") ) {
			extract = options["extract"].as<bool>();
		}

		if (extract) {
			cout << "Extracting package..." << endl;
		} else {
			cout << "Creating package..." << endl;

			PackageManager pacman(contentPath, samplesPath, fc::sha512(key));
			string error;
			if (pacman.createPackage(packagePath, &error)) {
				cout << "Package created " << pacman.getHash() << endl;
			} else {
				cout << "Failed to create package: " << error << endl;
			}
		}
		
		cout << contentPath << endl;
		cout << samplesPath << endl;
		cout << key << endl;
		cout << extract << endl;

	}
	catch ( const fc::exception& e )
	{
		std::cout << e.to_detail_string() << "\n";
		return 1;
	}
	return 0;
}


