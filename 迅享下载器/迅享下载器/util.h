#pragma once
#include<iostream>
#include<vector>
#include<fstream>
#include<sstream>

#include<boost/filesystem.hpp>
#ifdef _WIN32
//windows头文件
#include<WS2tcpip.h>
#include<Iphlpapi.h>//获取网卡信息的头文件
#pragma comment(lib,"Iphlpapi.lib") //获取网卡信息接口的库文件包含  GetAdaptersInfo()
#pragma comment(lib,"ws2_32.lib")//windows下的socket库  inet_pton
#else
//linux头文件
#endif

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;


#define P2P_PORT 9000
#define MAX_IPBUFFER 16

class StringUtil{
	static int64_t Str2Dig(const std::string &num){
		std::stringstream tmp;
		tmp << num;
		int64_t res;
		tmp >> res;
		return res;

	}

};

//设计文件操作工具类，目的是为了让外部直接使用文件操作接口
//若后期文件操作有其他修改，则只需要修改文件操作工具即可，而不需要对原文进行改变
class FileUtil{
public:
	static int64_t GetFileSize(const std::string &name){
	return boost::filesystem::file_size(name);
	}
	static bool Write(const std::string &name, const std::string &body, int64_t offset = 0){
		//std::ofstream ofs(name，std::ios::binary);
		//if (ofs.is_open() == false){
		//	std::cerr << "文件打开失败"<<std::endl;
		//	return false;
		//}
		//ofs.seekp(offset, std::ios::beg);//读写位置相对于起始位置开始偏移offset的偏移量
		//ofs.write(&body[0], body.size());
		//if (ofs.good() == false){
		//	std::cerr << "向文件写入数据失败" << std::endl; 
		//	return false;
		//}
		//ofs.close();
		//return true;
		FILE *fp = NULL;

		//在当前目录下打开文件， 只允许进行“读”操作，并使fp指向该文件
		fopen_s(&fp, name.c_str(), "wb+");//“wb”只写打开或建立一个二进制文件，只允许写数据
		if (fp == NULL){
			std::cout << "打开文件失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);//是把位置指针移到离文件首offset个字节处
		auto ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size()){
			std::cout << "向文件写入数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
	static bool Read(const std::string &name, std::string *body){
		
		std::ifstream ifs(name,std::ios::binary);
		if (ifs.is_open() == false){
			std::cerr << "打开文件失败\n";
			return false;
		}
		uint64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		ifs.read(&(*body)[0], filesize);
		ifs.close();
		

		/*uint64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "读取文件数据:" << name << "size:" << filesize << "\n";
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "打开文件数据失败\n";
			return false;
		}
		size_t ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize)
		{
			std::cerr << "读取文件失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);*/

		return true;
	}
	static bool ReadRange(const std::string &name, std::string *body, int64_t len, int64_t offset){
		body->resize(len);
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL){
			std::cerr << "打开文件失败！\n";
			fclose(fp);
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		auto ret = fread(&(*body)[0], 1, len, fp);
		if (ret != len){
			std::cerr  << "从文件读取数据失败！\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
	static bool GetRange(const std::string& range_str, int64_t* start, int64_t* end)
	{
		size_t pos1 = range_str.find('-');
		size_t pos2 = range_str.find('=');
		*start = std::atol(range_str.substr(pos2 + 1, pos1 - pos2 - 1).c_str());
		//std::cout << "range_str.substr(pos1 + 1, pos1 - pos2 - 1):" << range_str.substr(pos1 + 1, pos1 - pos2 - 1) << std::endl;
		*end = std::atol(range_str.substr(pos1 + 1).c_str());
		//std::cout << "range_str.substr(pos1 + 1):" << range_str.substr(pos1 + 1) << std::endl;
		return true;
	}
};

class Adapter{//不同系统，给出网卡信息不同，这个接口用于统一
public:
	uint32_t _ip_addr;//网卡上的IP地址
	uint32_t _mask_addr;//网卡上的子网掩码信息
};

class AdapterUtil{
public:
#ifdef _WIN32
	//windows下的获取网卡信息实现
	static bool GetAllAdapter(std::vector<Adapter> *list){
		//PIP_ADAPTER_INFO结构体指针 存储本机网卡信息
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();//开辟一块网卡信息结构空间
		unsigned long all_adapter_size = sizeof(IP_ADAPTER_INFO);

		// GetAdaptersInfo win下获取网卡信息的接口—网卡信息有可能有多个，因此传入一个指针
		//all_adapter_size,即是一个输入值也是一个输出值，空间大小不够就会返回实际网卡信息需要的大小
		int ret = GetAdaptersInfo(p_adapters, &all_adapter_size); 
		if (ret == ERROR_BUFFER_OVERFLOW){//缓冲区空间不足
			delete  p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];//重新给指针申请空间 
			GetAdaptersInfo(p_adapters, &all_adapter_size);//重新获取网卡信息
		}
		auto p = p_adapters;//记录开始位置
		while (p_adapters){

			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);

			if (adapter._ip_addr != 0){ //因为有些网卡并没有启用，导致IP地址为0；

				list->push_back(adapter);//将网卡信息添加到vector中返回给用户

			}

			p_adapters = p_adapters->Next;
		}
		delete p;//释放
		return true;
	}
#else
	bool GetAllAdapter(std::vector<Adapter> *list);//Linux下获取网卡信息实现

#endif
};