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
	uint32_t _ip_addr;//要配对的主机地址
	bool _pair_ret;//用于存放配对结果，配对成功—true，配对失败—false
};

class Client{
public:

	bool Start(){
		while (1){ 
			GetOnlineHost(); 
		}
		return true;
	}

	//主机配对线程入口函数
	void HostPair(Host *host){
		//1.组织http协议格式的请求数据
		//2.组织搭建一个TCP的客户端，将数据发送
		//3.等待服务器的回复，并进行解析
		//这整个过程只用第三方库httplib实现
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
	    httplib::Client cli(buf, P2P_PORT);
		auto rsp = cli.Get("/hostpair");
		if (rsp&&rsp->status == 200){
			host->_pair_ret = true;
		}
		return;
	} 

	//获取在线主机
	bool GetOnlineHost(){
		char ch='Y';//重新匹配，默认是进行匹配的，若已经匹配，Online主机不为空，就让用户自己选择
		if (!_online_host.empty()){
			std::cout << "是否重新查看在线主机(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}
		
		if (ch == 'Y'){
			//1.获取网卡信息，进而得到局域网中中所有IP地址列表
			std::cout << "开始主机匹配...\n";
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);
			std::vector<Host> host_list;//存放所有主机IP地址

			for (int i = 0; i < list.size(); i++){//得到所有的主机IP地址列表
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;
				uint32_t net = (ntohl(ip&mask));//计算网络号
				uint32_t max_host = (~ntohl(mask));//计算最大主机号
				for (int j = 1; j < (int32_t)max_host; j++){
					uint32_t host_ip = net + j;//这个主机IP的计算应该使用主机字节序的网络号和主机号
					Host host;
					host._ip_addr = htonl(host_ip);//转换为网络字节序
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}

			//对host_list中的主机创建线程进行配对
			std::cout << "正在主机匹配中，请稍后...\n";
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); i++){
				//2.逐个对IP地址列表中的主机发送配对请求
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
			}
			//3.若配对请求得到响应，则在对应主机位在线主机，将IP添加到——online_host列表中
			for (int i = 0; i < host_list.size(); i++){
				thr_list[i]->join();
				if (host_list[i]._pair_ret == true){
					_online_host.push_back(host_list[i]);
				}
				delete thr_list[i];
			}
		}

			//4.打印在线主机列表，使用户选择

		for (int i = 0; i < _online_host.size(); i++){
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr,buf, MAX_IPBUFFER);
			std::cout <<"\t"<< buf << std::endl;
		}
			std::cout << "请选择配对主机，获取共享文件列表：";
			fflush(stdout);
			std::string select_ip;
			std::cin >> select_ip;
			GetShareList(select_ip);//用户选择共享主机后，调用获取文件列表接口
			return true;
	}

	//获取文件列表
	bool GetShareList(const std::string &host_ip){
		//向服务端发送一个文件列表获取的请求
		//1.先发送请求2.得到响应后解析正文（文件名称）

		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200){
			std::cerr << "获取文件列表响应错误\n";
			return false;
		}
		std::cout << "打印文件列表\n";

		//打印正文—打印服务端响应的文件名称列表供用户选择
		std::cout << rsp->body << std::endl;
		std::cout << "\n请选择要下载的文件：";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		DownloadFile(host_ip, filename);
		return true;
	}
	//下载文件
	bool DownloadFile(const std::string &host_ip,const std::string &filename){
		//1.向服务端的发送文件下载请求
		//2.得到响应结果，响应结果中的body正文就是文件数据
		//3.创建文件，将文件写入到文件中，关闭文件
		std::string req_path = "/download" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200){
			std::cout << "下载文件，获取响应信息失败;" << rsp->status << std::endl;
			return false;
		}

		if (!boost::filesystem::exists(DOWNLOAD_PATH)){
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}

		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false){
			std::cerr << "文件下载失败" << std::endl;
			return false;
		}
		std::cout << "文件下载成功！\n";
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

	//获取文件列表—在主机上设置一个共享目录，这个目录下的文件都要共享给别人 
	static void ShareList(const httplib::Request &req, httplib::Response &rsp){
		//查看目录是否存在， 若不在则创建
		std::cout << "查看目录是否存在\n";
		if (!boost::filesystem::exists(SHARED_PATH)){
			boost::filesystem::create_directory(SHARED_PATH);
		}
		std::cout << "开始迭代查找\n";

		boost::filesystem::directory_iterator begin(SHARED_PATH);//实例化目录迭代器开始
		boost::filesystem::directory_iterator end;//实例化目录迭代器结尾
		for (; begin != end; ++begin){
			if (boost::filesystem::is_directory(begin->status())){
				continue;
			}
			std::string name = begin->path().filename().string();
			rsp.body += name + "\r\n";
		}
		std::cout << "已经发送\n";

		rsp.status = 200;
		return; 
	}
	static void DownLoad(const httplib::Request &req, httplib::Response &rsp){
		boost::filesystem::path req_path(req.path);//req.path-客户端请求得资源路径/download/filename.txt
		std::string name = req_path.filename().string();//只获取文件名称 filename.txt
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
