#define _CRT_SECURE_NO_WARNINGS


#include <boost/iostreams/device/mapped_file.hpp> 
#include <algorithm>  // for std::find
#include <iostream> 
#include <cstring>

#include <thread>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <queue>

#include <time.h>

#include <cstdlib> 

#include <iomanip>
#include <mutex>
#include <future>
#include <io.h>



using namespace std;
using namespace this_thread;
using namespace chrono;

string gen_random(const int len) {
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	std::string tmp_s;
	tmp_s.reserve(len);

	for (int i = 0; i < len; ++i) {
		tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	return tmp_s;
}

int create_files_string(int count, int l, string fi)
{
	ofstream outdata;
	for (int i = 0; i < count; i++)
	{
		outdata.open(fi + to_string(i) + ".txt");
		outdata << gen_random(l) << endl;
		outdata.close();
	}

	return 0;
}

int create_files_delim(int count, int delim, string fi)
{
	ofstream outdata;
	for (int i = 0; i < count; i++)
	{
		outdata.open(fi + to_string(i) + ".txt", std::ios_base::app);
		string tempstr = gen_random(delim);
		for (auto ch : tempstr)
			outdata << ch << endl;
		outdata.close();
	}


	return 0;
}

vector<string> split(string s, string delimiter) {
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	string token;
	vector<string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}


vector<string> parse(string s, string delimiter) {

	vector<string> res;

	std::size_t prev = 0, pos;
	while ((pos = s.find_first_of(delimiter, prev)) != std::string::npos)
	{
		if (pos > prev)
			res.push_back(s.substr(prev, pos - prev));
		prev = pos + 1;
	}
	if (prev < s.length())
		res.push_back(s.substr(prev, std::string::npos));


	return res;
}



mutex m;

vector<string> getDirFiles(string dirName)   // Сбор всех имен файлов в каталоге
{
	vector<string> fileNames;
	_finddata_t info;
	intptr_t handle = _findfirst((dirName + "\\*.*").c_str(), &info);
	if (handle == -1) return fileNames;
	do {
		if (info.attrib & _A_SUBDIR) continue;
		fileNames.push_back(string(dirName) + "\\" + info.name);
	} while (_findnext(handle, &info) == 0);
	_findclose(handle);
	return fileNames;
}



class Bar {

private:
	vector<string> _files;	//list of files
	queue<pair<string, pair<string, string>>> _rfiles;						//data of files
	queue<vector<string>> _out;									//what to write to out
	string out_dir;
public:
	int _out_size = 0;
	int temp_size = 0;
	int done = 0;
	Bar(string folder_in, string folder_out)
	{
		_files = getDirFiles(folder_in);
		out_dir = folder_out;
	}

	~Bar(){}

	void r()
	{
		std::chrono::time_point<std::chrono::high_resolution_clock> start_r, end_r;
		start_r = std::chrono::high_resolution_clock::now();
		for (string file : _files)
		{
			if (file != out_dir)
				get_data_from_file(_rfiles, file);
		}
		end_r = std::chrono::high_resolution_clock::now();
		std::chrono::duration<long double> elapsed_seconds_r = end_r - start_r;
		std::cout << "elapsed time r: " << elapsed_seconds_r.count() << "s\n";
	}

	void wr()
	{
		write_data_to_out(_rfiles);

	}

	void get_data_from_file(queue<pair<string, pair<string, string>>> &rfiles, string name)
	{
		boost::iostreams::mapped_file mmap(name, boost::iostreams::mapped_file::readonly);
		auto f = mmap.const_data();	//получаем адрес начала файла
		auto l = f + mmap.size();	//получаем адрес конца файла

		string delimiter = "\r\n";
		vector<string> v = split(f, delimiter);

		string temp = *v.begin();
		v.erase(v.begin());

		string s1;
		for (auto ch : v)
			s1 += ch;
		rfiles.push({ name, {temp, s1} });
		mmap.close();
		temp_size++;
	}

	void write_data_to_out(queue<pair<string, pair<string, string>>> &rfiles)
	{


		
		while ((_out_size < (_files.size() - 1)) || (temp_size > 0))
		{

			if (_rfiles.size() > 0)
			{
				m.lock();
				string n;
				pair<string, pair<string, string>> temp = rfiles.front();
				rfiles.pop();
				temp_size--;
				n = "[" + temp.first + "]:";
				vector<string> vout = parse(temp.second.first, temp.second.second);
				vout.insert(vout.begin(), n);

				cout << "get_id = " << get_id() << endl;
				_out.push(vout);
				_out_size++;
				m.unlock();

			}
		}
	}


	void print_to_file()
	{

		ofstream outdata;

		outdata.open(out_dir, std::ios_base::app);
		while ((done < (_files.size() - 1)) || (temp_size > 0))
		{
			while (!_out.empty())
			{
				for (string sstring : _out.front())
				{
					outdata << sstring << endl;
				}
				outdata << "\n";
				done++;
				_out.pop();
			}
		}

		outdata.close();

	}


};


int main(int argc, char *argv[])
{

	string source_folder = "C:\\Users\\Администратор\\source\\repos\\mthreads\\mthreads\\test";
	string dest_file = "C:\\Users\\Администратор\\source\\repos\\mthreads\\mthreads\\test\\out.txt";
	Bar test(source_folder, dest_file);
	//Bar test("D:\\files_folder", "D:\\files_folder\\out.txt");

/*	if (argc > 1)
	{
		if ((argc == 2) || ((string)argv[1] == "-h") || ((string)argv[1] == "-help") || ((string)argv[1] == "-?"))
		{
			cout << "Hello! It's help menu" << endl;
			cout << "Using: \n ./mthreads -h " << setw(20) << "Help menu" << endl;
			cout << " ./mthreads -create_files <count> <length of string> <count of delimetres> <folder_name> " << setw(20) << " Creating files in destination folder with set params" << endl;
			cout << " ./mthreads -transfer <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file" << endl;
			cout << " ./mthreads -thread 1 <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file using 1 thread!" << endl;
			cout << " ./mthreads -thread 3 <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file using 3 threads (reading + parsing + writing)!" << endl;

			return 0;
		}
		else
		{
			if ((string)argv[1] == "-create_files")
			{
				if (argc == 6)
				{
					std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
					start = std::chrono::high_resolution_clock::now();

					create_files_string(atoi(argv[2]), atoi(argv[3]), (string)argv[5]);
					create_files_delim(atoi(argv[2]), atoi(argv[4]), (string)argv[5]);

					cout << "DONE" << endl;

					end = std::chrono::high_resolution_clock::now();
					std::chrono::duration<long double> elapsed_seconds = end - start;
					std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

					return 0;
				}
				else
				{
					cout << "Hello! It's help menu" << endl;
					cout << "Using: \n ./mthreads -h " << setw(20) << "Help menu" << endl;
					cout << " ./mthreads -create_files <count> <length of string> <count of delimetres> <folder_name> " << setw(20) << " Creating files in destination folder with set params" << endl;
					cout << " ./mthreads -transfer <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file" << endl;
					cout << " ./mthreads -thread 1 <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file using 1 thread!" << endl;
					cout << " ./mthreads -thread 3 <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file using 3 threads (reading + parsing + writing)!" << endl;

					return 0;
				}

			}

			if ((string)argv[1] == "-transfer")
			{
				if (argc == 4)
				{
					source_folder = argv[2];
					dest_file = argv[3];
				}
				else
				{
					cout << "Hello! It's help menu" << endl;
					cout << "Using: \n ./mthreads -h " << setw(20) << "Help menu" << endl;
					cout << " ./mthreads -create_files <count> <length of string> <count of delimetres> <folder_name> " << setw(20) << " Creating files in destination folder with set params" << endl;
					cout << " ./mthreads -transfer <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file" << endl;
					cout << " ./mthreads -thread 1 <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file using 1 thread!" << endl;
					cout << " ./mthreads -thread 3 <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file using 3 threads (reading + parsing + writing)!" << endl;

					return 0;
				}
			}

			if ((string)argv[1] == "-thread")
			{
				if ((string)argv[1] == "1")
				{	
					std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
					start = std::chrono::high_resolution_clock::now();

					source_folder = argv[2];
					dest_file = argv[3];

					Bar thread1(source_folder, dest_file);

					thread1.r();
					thread1.wr();
					thread1.print_to_file();

					end = std::chrono::high_resolution_clock::now();
					std::chrono::duration<long double> elapsed_seconds = end - start;
					std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

					return 0;
				}
				else if ((string)argv[1] == "3")
				{
					std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
					start = std::chrono::high_resolution_clock::now();

					source_folder = argv[2];
					dest_file = argv[3];

					Bar thread3(source_folder, dest_file);


					std::thread t(&Bar::r, &thread3);
					t.detach();
					std::thread t1(&Bar::wr, &thread3);
					t1.detach();
					thread3.print_to_file();

					end = std::chrono::high_resolution_clock::now();
					std::chrono::duration<long double> elapsed_seconds = end - start;
					std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
					return 0;
				}
				else
				{
					cout << "Hello! It's help menu" << endl;
					cout << "Using: \n ./mthreads -h " << setw(20) << "Help menu" << endl;
					cout << " ./mthreads -create_files <count> <length of string> <count of delimetres> <folder_name> " << setw(20) << " Creating files in destination folder with set params" << endl;
					cout << " ./mthreads -transfer <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file" << endl;
					cout << " ./mthreads -thread 1 <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file using 1 thread!" << endl;
					cout << " ./mthreads -thread 3 <source_folder> <destination_file> " << setw(20) << " Parsing strings in files from source_folder and writing data to destionation_file using 3 threads (reading + parsing + writing)!" << endl;

					return 0;
				}
			}

		}
	}
*/	

	
	
	std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
	std:vector<future<void>> ThreadVector_pars;
	vector<future<void>> ThreadVector_write_to_file;

	start = std::chrono::high_resolution_clock::now();

	std::thread read(&Bar::r, &test);
	read.detach();

	for (int i = 0; i < std::thread::hardware_concurrency() * 2; i++)
	{
		ThreadVector_pars.push_back(async(&Bar::wr, &test));
	}




	test.print_to_file();
	


	for (auto &t1 : ThreadVector_pars)
	{
		t1.get();
	}
	

	


	end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<long double> elapsed_seconds = end - start;
	std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

	return 0;

}

