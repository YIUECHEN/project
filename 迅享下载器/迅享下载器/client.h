#pragma once
#include"util.h"
#include"httplib.h"
#include<boost/filesystem.hpp>//

#define P2P_PORT 9000
#define MAX_IPBUFFER 16
#define DOWNLOAD_PATH "./Download/"
#define MAX_RANGE (100*1024*1024)

struct Host{
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

	//主机配对的线程入口函数
	void HostPair(Host *host){
		//1.组织http协议格式的请求数据
		//2.组织搭建一个TCP的客户端，将数据发送
		//3.等待服务器的回复，并进行解析
		//这整个过程只用第三方库httplib实现
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
		httplib::Client cli(buf, P2P_PORT);//实例化客户端对象
		auto rsp = cli.Get("/hostpair");//rsp是rsponse类型的智能指针，存储服务端返回的内容
		if (rsp&&rsp->status == 200){  
			host->_pair_ret = true;  //重置主机配对结果
		}
		return;
	}

	//获取在线主机
	bool GetOnlineHost(){
		char ch = 'Y';//重新匹配，默认是进行匹配的，若已经匹配，Online主机不为空，就让用户自己选择
		if (!_online_host.empty()){
			std::cout << "是否重新匹配在线主机(Y/N):";
			fflush(stdout);
			std::cin >> ch;
		}

		if (ch == 'Y'){
			//1.获取网卡信息，进而得到局域网中中所有IP地址列表
			std::cout << "开始主机查找匹配...\n";
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
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
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
		//1.先发送请求
		//2.得到响应后解析正文（文件名称）

		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("/list");
		if (rsp == NULL || rsp->status != 200){
			std::cerr << "获取文件列表响应错误\n";
			return false;
		}
		std::cout << "可下载的文件列表" << std::endl;
		//打印正文—打印服务端响应的文件名称列表供用户选择
		std::cout << rsp->body << std::endl;
		std::cout << "\n请选择要下载的文件：";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		std::cout << "开始下载文件..." << std::endl;
		//DownloadFile(host_ip, filename);
		RangeDownloadFile(host_ip, filename);
		return true;
	}
	//下载文件
	bool DownloadFile(const std::string &host_ip, const std::string &filename){
		//1.向服务端的发送文件下载请求
		//2.得到响应结果，响应结果中的body正文就是文件数据
		//3.创建文件，将文件写入到文件中，关闭文件
		std::string req_path = "/download/" + filename;
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
	bool RangeDownloadFile(const std::string &host_ip, const std::string &filename){
		//1.向服务端发送一个HEAD请求，得到响应，获取文件大小
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200){
			std::cout << "获取文件大小信息失败\n";
			return false;
		}
		std::string clen = rsp->get_header_value("Content-Length");
		//int64_t filesize=StringUtil:;Str2Dig(clen);
		int64_t filesize = std::stol(clen);
		//2.根据定义的块大小对文件进行区间分块
		if (filesize < MAX_RANGE){
			std::cout << "文件较小，直接下载...\n";
			return DownloadFile(host_ip, filename);
		}
		std::cout << "文件较大，分块下载...\n";
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
			std::cout << "客户端请求分块:" << range_start << "-" << range_end << std::endl;

			//3.循环请求分块数据，一块请求成功了在下载下一块
			std::stringstream tmp;
			tmp << "bytes=" << range_start << "-" << range_end;
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
			httplib::Headers header;
			header.insert(std::make_pair("Range", tmp.str()));
			auto rsp = cli.Get(req_path.c_str(), header);
			if (rsp == NULL || rsp->status != 206){
				std::cout <<  "区间文件下载失败！\n\n";
				return false;
			}
			std::string real_path = DOWNLOAD_PATH + filename;
			if (!boost::filesystem::exists(DOWNLOAD_PATH))
			{
				boost::filesystem::create_directory(DOWNLOAD_PATH);
			}
			FileUtil::Write(real_path, rsp->body, range_start);
		}
		std::cout << "文件下载成功！\n\n";
		return true;
	}

private:
	std::vector<Host> _online_host;

};