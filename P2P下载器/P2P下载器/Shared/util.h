#pragma once
#include<iostream>
#include<vector>
#include<fstream>
#include<boost/filesystem.hpp>
#ifdef _WIN32
//windows头文件
#include<WS2tcpip.h>
#include<Iphlpapi.h>//获取网卡信息的头文件
#pragma comment(lib,"Iphlpapi.lib")//获取网卡信息接口的库文件包含
#pragma comment(lib,"ws2_32.lib")//windows下的socket库
#else
//linux头文件
#endif

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

//设计文件操作工具类，目的是为了让外部直接使用文件操作接口
//若后期文件操作有其他修改，则只需要修改文件操作工具即可，而不需要对原文进行改变
class FileUtil{
public:
	static bool Write(const std::string &name, const std::string &body, int64_t offset = 0){
		std::ofstream ofs(name);
		if (ofs.is_open() == false){
			std::cerr << "文件打开失败"<<std::endl;
			return false;
		}
		ofs.seekp(offset, std::ios::beg);//读写位置相对于起始位置开始偏移offset的偏移量
		ofs.write(&body[0], body.size());
		if (ofs.good() == false){
			std::cerr << "向文件写入数据失败" << std::endl; 
			return false;
		}
		ofs.close();
		return true;
	}
	static bool Read(const std::string &name, std::string *body){
		std::ifstream ifs(name);
		if (ifs.is_open() == false){
			std::cerr << "打开文件失败\n";
			return false;
		}
		int64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		ifs.read(&(*body)[0], filesize);
		/*if (ifs.good() == false){
			std::cerr << "读取文件失败\n";
			ifs.close();
			return false;
		}*/
		return true;
	}

};

class Adapter{
public:
	uint32_t _ip_addr;//网卡上的IP地址
	uint32_t _mask_addr;//玩会卡上的子网掩码信息
};

class AdapterUtil{
public:
#ifdef _WIN32
	//windows下的获取网卡信息实现
	static bool GetAllAdapter(std::vector<Adapter> *list){
		//PIP_ADAPTER_INFO结构体指针存储本机网卡信息
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();//开辟一块网卡信息结构空间
		uint64_t all_adapter_size = sizeof(IP_ADAPTER_INFO);
		// GetAdaptersInfo win下获取网卡信息的接口—网卡信息有可能有多个，因此传入一个指针
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		if (ret == ERROR_BUFFER_OVERFLOW){//缓冲区空间不足
			delete  p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];//重新给指针申请空间 
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//重新获取网卡信息
		}
		while (p_adapters){
			Adapter adapter;
			inet_pton(AF_INET,p_adapters->IpAddressList.IpAddress.String,&adapter._ip_addr);
			inet_pton(AF_INET,p_adapters->IpAddressList.IpMask.String,&adapter._mask_addr);
			if (adapter._ip_addr != 0){ //因为有些网卡并没有启用，导致IP地址为0；
				list->push_back(adapter);//将网卡信息添加到vector中返回给用户
				/*std::cout << "网卡名称：" << p_adapters->AdapterName << std::endl;
				std::cout << "网卡描述：" << p_adapters->Description << std::endl;
				std::cout << "IP地址：" << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "子网掩码：" << p_adapters->IpAddressList.IpMask.String << std::endl;
				std::cout << std::endl; */
				//continue;
			}
			
			p_adapters = p_adapters->Next;
		}
		return true;
	}
#endif
};