#pragma once
#include"util.h"
#include<thread>
#include"httplib.h"
#include<boost/filesystem.hpp>

#define P2P_PORT 9000
#define MAX_IPBUFFER 16
#define SHARED_PATH "./Shared/"
#define DOWNLOAD_PATH "./DownLoad/"

class Host{
public:
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

	//��������߳���ں���
	void HostPair(Host *host){
		//1.��֯httpЭ���ʽ����������
		//2.��֯�һ��TCP�Ŀͻ��ˣ������ݷ���
		//3.�ȴ��������Ļظ��������н���
		//����������ֻ�õ�������httplibʵ��
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
	    httplib::Client cli(buf, P2P_PORT);
		auto rsp = cli.Get("/hostpair");
		if (rsp&&rsp->status == 200){
			host->_pair_ret = true;
		}
		return;
	} 

	//��ȡ��������
	bool GetOnlineHost(){
		char ch='Y';//����ƥ�䣬Ĭ���ǽ���ƥ��ģ����Ѿ�ƥ�䣬Online������Ϊ�գ������û��Լ�ѡ��
		if (!_online_host.empty()){
			std::cout << "�Ƿ����²鿴��������(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}
		
		if (ch == 'Y'){
			//1.��ȡ������Ϣ�������õ���������������IP��ַ�б�
			std::cout << "��ʼ����ƥ��...\n";
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
			inet_ntop(AF_INET, &_online_host[i]._ip_addr,buf, MAX_IPBUFFER);
			std::cout <<"\t"<< buf << std::endl;
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
		//1.�ȷ�������2.�õ���Ӧ��������ģ��ļ����ƣ�

		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200){
			std::cerr << "��ȡ�ļ��б���Ӧ����\n";
			return false;
		}
		std::cout << "��ӡ�ļ��б�\n";

		//��ӡ���ġ���ӡ�������Ӧ���ļ������б��û�ѡ��
		std::cout << rsp->body << std::endl;
		std::cout << "\n��ѡ��Ҫ���ص��ļ���";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		DownloadFile(host_ip, filename);
		return true;
	}
	//�����ļ�
	bool DownloadFile(const std::string &host_ip,const std::string &filename){
		//1.�����˵ķ����ļ���������
		//2.�õ���Ӧ�������Ӧ����е�body���ľ����ļ�����
		//3.�����ļ������ļ�д�뵽�ļ��У��ر��ļ�
		std::string req_path = "/download" + filename;
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
private:
	std::vector<Host> _online_host;

};
 
class Server{
public:
	bool Start(){
		_srv.Get("/hostpair",HostPair);
		_srv.Get("/list", ShareList);
		_srv.Get("/download.*", DownLoad);
		_srv.listen("0.0.0.0", P2P_PORT);

		return true;
	}
private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp){
		rsp.status = 200;
		return;
	}

	//��ȡ�ļ��б�������������һ������Ŀ¼�����Ŀ¼�µ��ļ���Ҫ��������� 
	static void ShareList(const httplib::Request &req, httplib::Response &rsp){
		//�鿴Ŀ¼�Ƿ���ڣ� �������򴴽�
		std::cout << "�鿴Ŀ¼�Ƿ����\n";
		if (!boost::filesystem::exists(SHARED_PATH)){
			boost::filesystem::create_directory(SHARED_PATH);
		}
		std::cout << "��ʼ��������\n";

		boost::filesystem::directory_iterator begin(SHARED_PATH);//ʵ����Ŀ¼��������ʼ
		boost::filesystem::directory_iterator end;//ʵ����Ŀ¼��������β
		for (; begin != end; ++begin){
			if (boost::filesystem::is_directory(begin->status())){
				continue;
			}
			std::string name = begin->path().filename().string();
			rsp.body += name + "\r\n";
		}
		std::cout << "�Ѿ�����\n";

		rsp.status = 200;
		return; 
	}
	static void DownLoad(const httplib::Request &req, httplib::Response &rsp){
		boost::filesystem::path req_path(req.path);//req.path-�ͻ����������Դ·��/download/filename.txt
		std::string name = req_path.filename().string();//ֻ��ȡ�ļ����� filename.txt
		std::string realpath = SHARED_PATH  + name;
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath)){
			rsp.status  = 404;
			return;
		}
		if (FileUtil::Read(realpath, &rsp.body) == false){
			rsp.status = 500;
			return;
		}
		return;
	}

private:
	httplib::Server _srv;
};
