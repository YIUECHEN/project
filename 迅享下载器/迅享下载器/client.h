#pragma once
#include"util.h"
#include"httplib.h"
#include<boost/filesystem.hpp>//

#define P2P_PORT 9000
#define MAX_IPBUFFER 16
#define DOWNLOAD_PATH "./Download/"
#define MAX_RANGE (100*1024*1024)

struct Host{
	uint32_t _ip_addr;//Ҫ��Ե�������ַ
	bool _pair_ret;//���ڴ����Խ������Գɹ���true�����ʧ�ܡ�false
};

class Client{
public:

	bool Start(){
		while (1){
			GetOnlineHost();
		}
		return true;
	}

	//������Ե��߳���ں���
	void HostPair(Host *host){
		//1.��֯httpЭ���ʽ����������
		//2.��֯�һ��TCP�Ŀͻ��ˣ������ݷ���
		//3.�ȴ��������Ļظ��������н���
		//����������ֻ�õ�������httplibʵ��
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
		httplib::Client cli(buf, P2P_PORT);//ʵ�����ͻ��˶���
		auto rsp = cli.Get("/hostpair");//rsp��rsponse���͵�����ָ�룬�洢����˷��ص�����
		if (rsp&&rsp->status == 200){  
			host->_pair_ret = true;  //����������Խ��
		}
		return;
	}

	//��ȡ��������
	bool GetOnlineHost(){
		char ch = 'Y';//����ƥ�䣬Ĭ���ǽ���ƥ��ģ����Ѿ�ƥ�䣬Online������Ϊ�գ������û��Լ�ѡ��
		if (!_online_host.empty()){
			std::cout << "�Ƿ�����ƥ����������(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}

		if (ch == 'Y'){
			//1.��ȡ������Ϣ�������õ���������������IP��ַ�б�
			std::cout << "��ʼ��������ƥ��...\n";
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);
			std::vector<Host> host_list;//�����������IP��ַ

			for (int i = 0; i < list.size(); i++){//�õ����е�����IP��ַ�б�

				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				uint32_t net = (ntohl(ip&mask));//���������
				uint32_t max_host = (~ntohl(mask));//�������������

				for (int j = 1; j < (int32_t)max_host; j++){
					uint32_t host_ip = net + j;//�������IP�ļ���Ӧ��ʹ�������ֽ��������ź�������
					Host host;
					host._ip_addr = htonl(host_ip);//ת��Ϊ�����ֽ���
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}

			//��host_list�е����������߳̽������
			std::cout << "��������ƥ���У����Ժ�...\n";
			std::vector<std::thread*> thr_list(host_list.size());

			for (int i = 0; i < host_list.size(); i++){
			//2.�����IP��ַ�б��е����������������
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
			}

			//3.���������õ���Ӧ�����ڶ�Ӧ����λ������������IP��ӵ�����online_host�б���
			for (int i = 0; i < host_list.size(); i++){
				thr_list[i]->join();
				if (host_list[i]._pair_ret == true){
					_online_host.push_back(host_list[i]);
				}
				delete thr_list[i];
			}
		}

		//4.��ӡ���������б�ʹ�û�ѡ��

		for (int i = 0; i < _online_host.size(); i++){
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		std::cout << "��ѡ�������������ȡ�����ļ��б�";
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		GetShareList(select_ip);//�û�ѡ���������󣬵��û�ȡ�ļ��б�ӿ�
		return true;
	}

	//��ȡ�ļ��б�
	bool GetShareList(const std::string &host_ip){
		//�����˷���һ���ļ��б��ȡ������
		//1.�ȷ�������
		//2.�õ���Ӧ��������ģ��ļ����ƣ�

		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200){
			std::cerr << "��ȡ�ļ��б���Ӧ����\n";
			return false;
		}
		std::cout << "�����ص��ļ��б�" << std::endl;
		//��ӡ���ġ���ӡ�������Ӧ���ļ������б��û�ѡ��
		std::cout << rsp->body << std::endl;
		std::cout << "\n��ѡ��Ҫ���ص��ļ���";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		std::cout << "��ʼ�����ļ�..." << std::endl;
		//DownloadFile(host_ip, filename);
		RangeDownloadFile(host_ip, filename);
		return true;
	}
	//�����ļ�
	bool DownloadFile(const std::string &host_ip, const std::string &filename){
		//1.�����˵ķ����ļ���������
		//2.�õ���Ӧ�������Ӧ����е�body���ľ����ļ�����
		//3.�����ļ������ļ�д�뵽�ļ��У��ر��ļ�
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200){
			std::cout << "�����ļ�����ȡ��Ӧ��Ϣʧ��;" << rsp->status << std::endl;
			return false;
		}

		if (!boost::filesystem::exists(DOWNLOAD_PATH)){
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}

		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false){
			std::cerr << "�ļ�����ʧ��" << std::endl;
			return false;
		}
		std::cout << "�ļ����سɹ���\n";
		return true;
	}
	bool RangeDownloadFile(const std::string &host_ip, const std::string &filename){
		//1.�����˷���һ��HEAD���󣬵õ���Ӧ����ȡ�ļ���С
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200){
			std::cout << "��ȡ�ļ���С��Ϣʧ��\n";
			return false;
		}
		std::string clen = rsp->get_header_value("Content-Length");
		//int64_t filesize=StringUtil:;Str2Dig(clen);
		int64_t filesize = std::stol(clen);
		//2.���ݶ���Ŀ��С���ļ���������ֿ�
		if (filesize < MAX_RANGE){
			std::cout << "�ļ���С��ֱ������...\n";
			return DownloadFile(host_ip, filename);
		}
		std::cout << "�ļ��ϴ󣬷ֿ�����...\n";
		std::cout << filesize << std::endl;

		int range_count = 0;
		if ((filesize%MAX_RANGE) == 0) {
			range_count = filesize / MAX_RANGE;
		}
		else {
			range_count = (filesize / MAX_RANGE) + 1;
		}    
		int64_t range_start = 0, range_end = 0;
		for (int i = 0; i < range_count; i++){
			range_start = i*MAX_RANGE;
			if (i == (range_count - 1)){
				range_end = filesize-1;
			}
			else{
				range_end = (i + 1)*MAX_RANGE-1;
			}
			std::cout << "�ͻ�������ֿ�:" << range_start << "-" << range_end << std::endl;

			//3.ѭ������ֿ����ݣ�һ������ɹ�����������һ��
			std::stringstream tmp;
			tmp << "bytes=" << range_start << "-" << range_end;
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
			httplib::Headers header;
			header.insert(std::make_pair("Range", tmp.str()));
			auto rsp = cli.Get(req_path.c_str(), header);
			if (rsp == NULL || rsp->status != 206){
				std::cout <<  "�����ļ�����ʧ�ܣ�\n\n";
				return false;
			}
			std::string real_path = DOWNLOAD_PATH + filename;
			if (!boost::filesystem::exists(DOWNLOAD_PATH))
			{
				boost::filesystem::create_directory(DOWNLOAD_PATH);
			}
			FileUtil::Write(real_path, rsp->body, range_start);
		}
		std::cout << "�ļ����سɹ���\n\n";
		return true;
	}

private:
	std::vector<Host> _online_host;

};