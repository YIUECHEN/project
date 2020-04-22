#pragma once
#include<iostream>
#include<vector>
#include<fstream>
#include<sstream>

#include<boost/filesystem.hpp>
#ifdef _WIN32
//windowsͷ�ļ�
#include<WS2tcpip.h>
#include<Iphlpapi.h>//��ȡ������Ϣ��ͷ�ļ�
#pragma comment(lib,"Iphlpapi.lib") //��ȡ������Ϣ�ӿڵĿ��ļ�����  GetAdaptersInfo()
#pragma comment(lib,"ws2_32.lib")//windows�µ�socket��  inet_pton
#else
//linuxͷ�ļ�
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

//����ļ����������࣬Ŀ����Ϊ�����ⲿֱ��ʹ���ļ������ӿ�
//�������ļ������������޸ģ���ֻ��Ҫ�޸��ļ��������߼��ɣ�������Ҫ��ԭ�Ľ��иı�
class FileUtil{
public:
	static int64_t GetFileSize(const std::string &name){
	return boost::filesystem::file_size(name);
	}
	static bool Write(const std::string &name, const std::string &body, int64_t offset = 0){
		//std::ofstream ofs(name��std::ios::binary);
		//if (ofs.is_open() == false){
		//	std::cerr << "�ļ���ʧ��"<<std::endl;
		//	return false;
		//}
		//ofs.seekp(offset, std::ios::beg);//��дλ���������ʼλ�ÿ�ʼƫ��offset��ƫ����
		//ofs.write(&body[0], body.size());
		//if (ofs.good() == false){
		//	std::cerr << "���ļ�д������ʧ��" << std::endl; 
		//	return false;
		//}
		//ofs.close();
		//return true;
		FILE *fp = NULL;

		//�ڵ�ǰĿ¼�´��ļ��� ֻ������С�������������ʹfpָ����ļ�
		fopen_s(&fp, name.c_str(), "wb+");//��wb��ֻд�򿪻���һ���������ļ���ֻ����д����
		if (fp == NULL){
			std::cout << "���ļ�ʧ��\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);//�ǰ�λ��ָ���Ƶ����ļ���offset���ֽڴ�
		auto ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size()){
			std::cout << "���ļ�д������ʧ��\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
	static bool Read(const std::string &name, std::string *body){
		
		std::ifstream ifs(name,std::ios::binary);
		if (ifs.is_open() == false){
			std::cerr << "���ļ�ʧ��\n";
			return false;
		}
		uint64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		ifs.read(&(*body)[0], filesize);
		ifs.close();
		

		/*uint64_t filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "��ȡ�ļ�����:" << name << "size:" << filesize << "\n";
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL)
		{
			std::cerr << "���ļ�����ʧ��\n";
			return false;
		}
		size_t ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize)
		{
			std::cerr << "��ȡ�ļ�ʧ��\n";
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
			std::cerr << "���ļ�ʧ�ܣ�\n";
			fclose(fp);
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		auto ret = fread(&(*body)[0], 1, len, fp);
		if (ret != len){
			std::cerr  << "���ļ���ȡ����ʧ�ܣ�\n";
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

class Adapter{//��ͬϵͳ������������Ϣ��ͬ������ӿ�����ͳһ
public:
	uint32_t _ip_addr;//�����ϵ�IP��ַ
	uint32_t _mask_addr;//�����ϵ�����������Ϣ
};

class AdapterUtil{
public:
#ifdef _WIN32
	//windows�µĻ�ȡ������Ϣʵ��
	static bool GetAllAdapter(std::vector<Adapter> *list){
		//PIP_ADAPTER_INFO�ṹ��ָ�� �洢����������Ϣ
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();//����һ��������Ϣ�ṹ�ռ�
		unsigned long all_adapter_size = sizeof(IP_ADAPTER_INFO);

		// GetAdaptersInfo win�»�ȡ������Ϣ�Ľӿڡ�������Ϣ�п����ж������˴���һ��ָ��
		//all_adapter_size,����һ������ֵҲ��һ�����ֵ���ռ��С�����ͻ᷵��ʵ��������Ϣ��Ҫ�Ĵ�С
		int ret = GetAdaptersInfo(p_adapters, &all_adapter_size); 
		if (ret == ERROR_BUFFER_OVERFLOW){//�������ռ䲻��
			delete  p_adapters;
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];//���¸�ָ������ռ� 
			GetAdaptersInfo(p_adapters, &all_adapter_size);//���»�ȡ������Ϣ
		}
		auto p = p_adapters;//��¼��ʼλ��
		while (p_adapters){

			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);

			if (adapter._ip_addr != 0){ //��Ϊ��Щ������û�����ã�����IP��ַΪ0��

				list->push_back(adapter);//��������Ϣ��ӵ�vector�з��ظ��û�

			}

			p_adapters = p_adapters->Next;
		}
		delete p;//�ͷ�
		return true;
	}
#else
	bool GetAllAdapter(std::vector<Adapter> *list);//Linux�»�ȡ������Ϣʵ��

#endif
};